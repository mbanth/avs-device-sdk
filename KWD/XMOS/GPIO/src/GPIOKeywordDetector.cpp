// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1
/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <memory>
#include <wiringPi.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <AVSCommon/Utils/Logger/Logger.h>
#include "GPIO/GPIOKeywordDetector.h"

namespace alexaClientSDK {
namespace kwd {

using namespace avsCommon::utils::logger;

/// String to identify log entries originating from this file.
static const std::string TAG("GPIOKeywordDetector");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// GPIO pin to monitor:
// Wiring Pi pin 2 which corresponds to Physical/Board pin 13 and GPIO/BCM pin 27
static const int GPIO_PIN = 2;

/// The device name of the I2C port connected to the device.
static const char *DEVNAME = "/dev/i2c-1";

/// The address of the I2C port connected to the device.
static const unsigned char I2C_ADDRESS = 0x2C;

/// The maximum size in bytes of the I2C transaction
static const int I2C_TRANSACTION_MAX_BYTES = 256;

/// The resource ID of the XMOS control command.
static const int CONTROL_RESOURCE_ID = 0xE0;

/// The command ID of the XMOS control command.
static const int CONTROL_CMD_ID = 0xAF;

/// The lenght of the payload of the XMOS control command
/// one control byte plus 3 uint64_t values
static const int CONTROL_CMD_PAYLOAD_LEN = 25;

/**
 * Open the I2C port connected to the device
 *
 * @return file descriptor with the connected device
 */
uint8_t GPIOKeywordDetector::openDevice() {
    int rc = 0;
    m_fileDescriptor = -1;

    setenv("WIRINGPI_GPIOMEM", "1", 1);
    if (wiringPiSetup() < 0) {
        ACSDK_ERROR(LX("openDeviceFailed").d("reason", "wiringPiSetup failed"));
        return false;
    }
    pinMode(GPIO_PIN, INPUT);

    // Open port for reading and writing
    if ((m_fileDescriptor = open(DEVNAME, O_RDWR)) < 0) {
        ACSDK_ERROR(LX("openDeviceFailed")
                    .d("reason", "openFailed"));
        perror( "" );
        return -1;
    }
    // Set the port options and set the address of the device we wish to speak to
    if ((rc = ioctl(m_fileDescriptor, I2C_SLAVE, I2C_ADDRESS)) < 0) {
        ACSDK_ERROR(LX("openDeviceFailed")
                    .d("reason", "setI2CConfigurationFailed"));
        perror( "" );
        return -1;
    }

    ACSDK_INFO(LX("openDeviceSuccess").d("port", I2C_ADDRESS));

    return 0;
}

std::unique_ptr<GPIOKeywordDetector> GPIOKeywordDetector::create(
        std::shared_ptr<AudioInputStream> stream,
        avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
        std::chrono::milliseconds msToPushPerIteration)  {

    if (!stream) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullStream"));
        return nullptr;
    }

    // TODO: ACSDK-249 - Investigate cpu usage of converting bytes between endianness and if it's not too much, do it.
    if (isByteswappingRequired(audioFormat)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "endianMismatch"));
        return nullptr;
    }

    std::unique_ptr<GPIOKeywordDetector> detector(new GPIOKeywordDetector(
        stream, keyWordObservers, keyWordDetectorStateObservers, audioFormat));

    if (!detector->init()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initDetectorFailed"));
        return nullptr;
    }

    return detector;
}

GPIOKeywordDetector::GPIOKeywordDetector(
    std::shared_ptr<AudioInputStream> stream,
    std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
    std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
    avsCommon::utils::AudioFormat audioFormat,
    std::chrono::milliseconds msToPushPerIteration) :
        XMOSKeywordDetector(stream, keyWordObservers, keyWordDetectorStateObservers, audioFormat, msToPushPerIteration) {
}

GPIOKeywordDetector::~GPIOKeywordDetector() {
}

bool GPIOKeywordDetector::init() {
    if (openDevice()<0) {
        ACSDK_ERROR(LX("initFailed").d("reason", "openDeviceFailed"));
        return false;
    }

    m_streamReader = m_stream->createReader(AudioInputStream::Reader::Policy::BLOCKING);
    if (!m_streamReader) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createStreamReaderFailed"));
        return false;
    }

    m_isShuttingDown = false;
    m_readAudioThread = std::thread(&GPIOKeywordDetector::readAudioLoop, this);
    m_detectionThread = std::thread(&GPIOKeywordDetector::detectionLoop, this);
    return true;
}

void GPIOKeywordDetector::detectionLoop() {
    notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
    int oldGpioValue = HIGH;

    std::chrono::steady_clock::time_point prev_time;
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

    while (!m_isShuttingDown) {
        auto currentIndex = m_streamReader->tell();

        // Read gpio value
        int gpioValue = digitalRead(GPIO_PIN);

        // Check if GPIO pin is changing from high to low
        if (gpioValue == LOW && oldGpioValue == HIGH)
        {
            std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();
            ACSDK_DEBUG0(LX("detectionLoopGPIOevent").d("absoluteElapsedTime (ms)", std::chrono::duration_cast<std::chrono::milliseconds> (current_time - start_time).count()));

            // Check if this is not the first HID event
            if (prev_time != std::chrono::steady_clock::time_point()) {
                ACSDK_DEBUG0(LX("detectionLoopGPIOevent").d("elapsedTimeFromPreviousEvent (ms)", std::chrono::duration_cast<std::chrono::milliseconds> (current_time - prev_time).count()));
            }
            prev_time = current_time;

            // Retrieve device indexes using control message via USB
            unsigned char payload[64];

            uint8_t cmd_ret = 1;

            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
            int rc = 0;
            while (cmd_ret!=0) {
                // Do a repeated start (write followed by read with no stop bit)
                unsigned char read_hdr[I2C_TRANSACTION_MAX_BYTES];
                read_hdr[0] = CONTROL_RESOURCE_ID;
                read_hdr[1] = CONTROL_CMD_ID;
                read_hdr[2] = (uint8_t)CONTROL_CMD_PAYLOAD_LEN;

                struct i2c_msg rdwr_msgs[2] = {
                    {  // Start address
                        .addr = I2C_ADDRESS,
                        .flags = 0, // write
                        .len = 3, // this is always 3
                        .buf = read_hdr
                    },
                    { // Read buffer
                        .addr = I2C_ADDRESS,
                        .flags = I2C_M_RD, // read
                        .len = CONTROL_CMD_PAYLOAD_LEN,
                        .buf = payload
                    }
                };

                struct i2c_rdwr_ioctl_data rdwr_data = {
                    .msgs = rdwr_msgs,
                    .nmsgs = 2
                };

                rc = ioctl( m_fileDescriptor, I2C_RDWR, &rdwr_data );

                if ( rc < 0 ) {
                    ACSDK_ERROR(LX("detectionLoopControlCommandFailed").d("reason", rc));
                    perror( "" );
                }

                cmd_ret = payload[0];
            }
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            ACSDK_DEBUG0(LX("detectionLoopControlCommand").d("time (us)", std::chrono::duration_cast<std::chrono::microseconds> (end - begin).count() ));

            // Read indexes
            uint64_t currentDeviceIndex = readIndex(payload, 1);
            uint64_t beginKWDeviceIndex = readIndex(payload, 9);
            uint64_t endKWDeviceIndex = readIndex(payload, 17);
            auto beginKWServerIndex = currentIndex - (currentDeviceIndex - beginKWDeviceIndex);

            // Send information to the server
            notifyKeyWordObservers(
                m_stream,
                KEYWORD_STRING,
                beginKWServerIndex,
                currentIndex);
            ACSDK_DEBUG0(LX("detectionLoopIndexes").d("hostCurrentIndex", currentIndex)
                         .d("deviceCurrentIndex", currentDeviceIndex)
                         .d("deviceKWEndIndex", endKWDeviceIndex)
                         .d("deviceKWBeginIndex", beginKWDeviceIndex)
                         .d("serverKWEndIndex", currentIndex)
                         .d("serverKWBeginIndex", beginKWServerIndex));
        }
        oldGpioValue = gpioValue;
    }
    m_streamReader->close();
}

}  // namespace kwd
}  // namespace alexaClientSDK
