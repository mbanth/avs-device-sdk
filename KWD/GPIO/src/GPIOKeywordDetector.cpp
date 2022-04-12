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

/// Keyword string
static const std::string KEYWORD_STRING = "alexa";

/// The number of hertz per kilohertz.
static const size_t HERTZ_PER_KILOHERTZ = 1000;

/// The timeout to use for read calls to the SharedDataStream.
const std::chrono::milliseconds TIMEOUT_FOR_READ_CALLS = std::chrono::milliseconds(1000);

/// The GPIO KW compatible AVS sample rate of 16 kHz.
static const unsigned int GPIO_COMPATIBLE_SAMPLE_RATE = 16000;

/// The GPIO KW compatible bits per sample of 16.
static const unsigned int GPIO_COMPATIBLE_SAMPLE_SIZE_IN_BITS = 16;

/// The GPIO KW compatible number of channels, which is 1.
static const unsigned int GPIO_COMPATIBLE_NUM_CHANNELS = 1;

/// The GPIO KW compatible audio encoding of LPCM.
static const avsCommon::utils::AudioFormat::Encoding GPIO_COMPATIBLE_ENCODING =
    avsCommon::utils::AudioFormat::Encoding::LPCM;

/// The GPIO KW compatible endianness which is little endian.
static const avsCommon::utils::AudioFormat::Endianness GPIO_COMPATIBLE_ENDIANNESS =
    avsCommon::utils::AudioFormat::Endianness::LITTLE;

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
 * Read a specific index from the payload of the USB control message
 *
 * @param payload The data returned via control message
 * @param start_index The index in the payload to start reading from
 * @return value stored in payload
 */
uint64_t readIndex(uint8_t* payload, int start_index) {
    uint64_t value = 0;
    for (int i=start_index; i<8+start_index; i++) {
        // Shift the byte by the right number of bits
        value += payload[i] << ((8-(i-start_index)-1)*8);
    }
    return value;
}

/**
 * Open the I2C port connected to the device
 *
 * @return file descriptor with the connected device
 */
uint8_t openI2CDevice() {
    int rc = 0;
    int fd = -1;
    // Open port for reading and writing
    if ((fd = open(DEVNAME, O_RDWR)) < 0) {
        ACSDK_ERROR(LX("openI2CDeviceFailed")
                    .d("reason", "openFailed"));
        perror( "" );
        return -1;
    }
    // Set the port options and set the address of the device we wish to speak to
    if ((rc = ioctl(fd, I2C_SLAVE, I2C_ADDRESS)) < 0) {
        ACSDK_ERROR(LX("openI2CDeviceFailed")
                    .d("reason", "setI2CConfigurationFailed"));
        perror( "" );
        return -1;
    }

    ACSDK_INFO(LX("openI2CDeviceSuccess").d("port", I2C_ADDRESS));

    return fd;
}

/**
 * Checks to see if an @c avsCommon::utils::AudioFormat is compatible with GPIO KW.
 *
 * @param audioFormat The audio format to check.
 * @return @c true if the audio format is compatible with GPIO KW and @c false otherwise.
 */
static bool isAudioFormatCompatibleWithGPIOKW(avsCommon::utils::AudioFormat audioFormat) {
    if (GPIO_COMPATIBLE_ENCODING != audioFormat.encoding) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithGPIOKWFailed")
                        .d("reason", "incompatibleEncoding")
                        .d("gpioKWEncoding", GPIO_COMPATIBLE_ENCODING)
                        .d("encoding", audioFormat.encoding));
        return false;
    }
    if (GPIO_COMPATIBLE_ENDIANNESS != audioFormat.endianness) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithGPIOKWFailed")
                        .d("reason", "incompatibleEndianess")
                        .d("gpioKWEndianness", GPIO_COMPATIBLE_ENDIANNESS)
                        .d("endianness", audioFormat.endianness));
        return false;
    }
    if (GPIO_COMPATIBLE_SAMPLE_RATE != audioFormat.sampleRateHz) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithGPIOKWFailed")
                        .d("reason", "incompatibleSampleRate")
                        .d("gpioKWSampleRate", GPIO_COMPATIBLE_SAMPLE_RATE)
                        .d("sampleRate", audioFormat.sampleRateHz));
        return false;
    }
    if (GPIO_COMPATIBLE_SAMPLE_SIZE_IN_BITS != audioFormat.sampleSizeInBits) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithGPIOKWFailed")
                        .d("reason", "incompatibleSampleSizeInBits")
                        .d("gpioKWSampleSizeInBits", GPIO_COMPATIBLE_SAMPLE_SIZE_IN_BITS)
                        .d("sampleSizeInBits", audioFormat.sampleSizeInBits));
        return false;
    }
    if (GPIO_COMPATIBLE_NUM_CHANNELS != audioFormat.numChannels) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithGPIOKWFailed")
                        .d("reason", "incompatibleNumChannels")
                        .d("gpioKWNumChannels", GPIO_COMPATIBLE_NUM_CHANNELS)
                        .d("numChannels", audioFormat.numChannels));
        return false;
    }
    return true;
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

    if (!isAudioFormatCompatibleWithGPIOKW(audioFormat)) {
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
        AbstractKeywordDetector(keyWordObservers, keyWordDetectorStateObservers),
        m_stream{stream},
        m_maxSamplesPerPush((audioFormat.sampleRateHz / HERTZ_PER_KILOHERTZ) * msToPushPerIteration.count()) {
}

GPIOKeywordDetector::~GPIOKeywordDetector() {
    m_isShuttingDown = true;
    if (m_detectionThread.joinable())
        m_detectionThread.join();
    if (m_readAudioThread.joinable())
        m_readAudioThread.join();
}

bool GPIOKeywordDetector::init() {
    setenv("WIRINGPI_GPIOMEM", "1", 1);
    if (wiringPiSetup() < 0) {
        ACSDK_ERROR(LX("initFailed").d("reason", "wiringPiSetup failed"));
        return false;
    }
    pinMode(GPIO_PIN, INPUT);

    if ((m_fileDescriptor = openI2CDevice())<0) {
        ACSDK_ERROR(LX("detectionLoopFailed").d("reason", "openI2CDeviceFailed"));
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

void GPIOKeywordDetector::readAudioLoop() {
    std::vector<int16_t> audioDataToPush(m_maxSamplesPerPush);
    bool didErrorOccur = false;

    while (!m_isShuttingDown) {
        readFromStream(
            m_streamReader,
            m_stream,
            audioDataToPush.data(),
            audioDataToPush.size(),
            TIMEOUT_FOR_READ_CALLS,
            &didErrorOccur);
    }
}

void GPIOKeywordDetector::detectionLoop() {
    m_beginIndexOfStreamReader = m_streamReader->tell();
    notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
    std::vector<int16_t> audioDataToPush(m_maxSamplesPerPush);
    int oldGpioValue = HIGH;

    std::chrono::steady_clock::time_point prev_time;
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

    while (!m_isShuttingDown) {
        auto current_index = m_streamReader->tell();

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
            uint64_t current_device_index = readIndex(payload, 1);
            uint64_t begin_device_index = readIndex(payload, 9);
            uint64_t end_device_index = readIndex(payload, 17);
            auto begin_server_index = current_index - (current_device_index - begin_device_index);

            // Send information to the server
            notifyKeyWordObservers(
                m_stream,
                KEYWORD_STRING,
                begin_server_index,
                current_index);
            ACSDK_DEBUG0(LX("detectionLoopIndexes").d("hostCurrentIndex", current_index)
                         .d("deviceCurrentIndex", current_device_index)
                         .d("deviceKWEndIndex", end_device_index)
                         .d("deviceKWBeginIndex", begin_device_index)
                         .d("serverKWEndIndex", current_index)
                         .d("serverKWBeginIndex", begin_server_index));
        }
        oldGpioValue = gpioValue;
    }
    m_streamReader->close();
}

}  // namespace kwd
}  // namespace alexaClientSDK
