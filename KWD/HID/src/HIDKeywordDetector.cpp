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
#include <fcntl.h>
#include <unistd.h>
#include <chrono>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "HID/HIDKeywordDetector.h"

namespace alexaClientSDK {
namespace kwd {

using namespace avsCommon::utils::logger;

/// String to identify log entries originating from this file.
static const std::string TAG("HIDKeywordDetector");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Keyword string
static const std::string KEYWORD_STRING = "alexa";

/// The number of hertz per kilohertz.
static const size_t HERTZ_PER_KILOHERTZ = 1000;

/// The timeout to use for read calls to the SharedDataStream.
const std::chrono::milliseconds TIMEOUT_FOR_READ_CALLS = std::chrono::milliseconds(1000);

/// The HID KW compatible AVS sample rate of 16 kHz.
static const unsigned int HID_COMPATIBLE_SAMPLE_RATE = 16000;

/// The HID KW compatible bits per sample of 16.
static const unsigned int HID_COMPATIBLE_SAMPLE_SIZE_IN_BITS = 16;

/// The HID KW compatible number of channels, which is 1.
static const unsigned int HID_COMPATIBLE_NUM_CHANNELS = 1;

/// The HID KW compatible audio encoding of LPCM.
static const avsCommon::utils::AudioFormat::Encoding HID_COMPATIBLE_ENCODING =
    avsCommon::utils::AudioFormat::Encoding::LPCM;

/// The HID KW compatible endianness which is little endian.
static const avsCommon::utils::AudioFormat::Endianness HID_COMPATIBLE_ENDIANNESS =
    avsCommon::utils::AudioFormat::Endianness::LITTLE;

/// HID keycode to monitor:
static const char * HID_KEY_CODE = "KEY_T";

/// HID device path

static const char * HID_DEVICE_PATH =  "/dev/input/event0";

/// USB Product ID of XMOS device
static const int USB_VENDOR_ID = 0x20B1;

/// USB Product ID of XMOS device
static const int USB_PRODUCT_ID = 0x18;

/// USB timeout for control transfer
static const int USB_TIMEOUT_MS = 500;

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
 * Search for an USB device, open the connection and return the correct handlers
 *
 * @param evdev The device handler necessary for reading HID events
 * @param devh  The device handler necessary for sending control commands
 * @return 0 if device is found and handlers correctly set
 */
uint8_t openUSBDevice(libevdev** evdev, libusb_device_handle** devh) {
    int rc = 1;
    libusb_device **devs = NULL;
    libusb_device *dev = NULL;

    ACSDK_INFO(LX("openUSBDeviceOngoing")
               .d("HIDDevicePath", HID_DEVICE_PATH)
               .d("USBVendorID", USB_VENDOR_ID)
               .d("USBProductID", USB_PRODUCT_ID));

    // Find USB device for reading HID events
    int fd;
    fd = open(HID_DEVICE_PATH, O_RDONLY|O_NONBLOCK);
    rc = libevdev_new_from_fd(fd, evdev);
    if (rc < 0) {
        ACSDK_ERROR(LX("openUSBDeviceFailed")
                        .d("reason", "initialiseLibevdevFailed")
                        .d("error", strerror(-rc)));
        return -1;
    }

    // Find USB device for sending control commands
    int ret = libusb_init(NULL);
    if (ret < 0) {
        ACSDK_ERROR(LX("openUSBDeviceFailed").d("reason", "initialiseLibUsbFailed"));
        return -1;
    }

    int num_dev = libusb_get_device_list(NULL, &devs);

    for (int i = 0; i < num_dev; i++) {
        struct libusb_device_descriptor desc;
        libusb_get_device_descriptor(devs[i], &desc);
        if (desc.idVendor == USB_VENDOR_ID && desc.idProduct == USB_PRODUCT_ID) {
          dev = devs[i];
          break;
        }
    }

    if (dev == NULL) {
        ACSDK_ERROR(LX("openUSBDeviceFailed").d("reason", "UsbDeviceNotFound"));
        return -1;
    }

    if (libusb_open(dev, devh) < 0) {
        ACSDK_ERROR(LX("openUSBDeviceFailed").d("reason", "UsbDeviceNotOpened"));
        return -1;
    }

    libusb_free_device_list(devs, 1);
    ACSDK_INFO(LX("openUSBDeviceSuccess").d("reason", "UsbDeviceOpened"));
    return 0;
}


/**
 * Checks to see if an @c avsCommon::utils::AudioFormat is compatible with HID KW.
 *
 * @param audioFormat The audio format to check.
 * @return @c true if the audio format is compatible with HID KW and @c false otherwise.
 */
static bool isAudioFormatCompatibleWithHIDKW(avsCommon::utils::AudioFormat audioFormat) {
    if (HID_COMPATIBLE_ENCODING != audioFormat.encoding) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithHIDKWFailed")
                        .d("reason", "incompatibleEncoding")
                        .d("gpiowwEncoding", HID_COMPATIBLE_ENCODING)
                        .d("encoding", audioFormat.encoding));
        return false;
    }
    if (HID_COMPATIBLE_ENDIANNESS != audioFormat.endianness) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithHIDKWFailed")
                        .d("reason", "incompatibleEndianess")
                        .d("gpiowwEndianness", HID_COMPATIBLE_ENDIANNESS)
                        .d("endianness", audioFormat.endianness));
        return false;
    }
    if (HID_COMPATIBLE_SAMPLE_RATE != audioFormat.sampleRateHz) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithHIDKWFailed")
                        .d("reason", "incompatibleSampleRate")
                        .d("gpiowwSampleRate", HID_COMPATIBLE_SAMPLE_RATE)
                        .d("sampleRate", audioFormat.sampleRateHz));
        return false;
    }
    if (HID_COMPATIBLE_SAMPLE_SIZE_IN_BITS != audioFormat.sampleSizeInBits) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithHIDKWFailed")
                        .d("reason", "incompatibleSampleSizeInBits")
                        .d("gpiowwSampleSizeInBits", HID_COMPATIBLE_SAMPLE_SIZE_IN_BITS)
                        .d("sampleSizeInBits", audioFormat.sampleSizeInBits));
        return false;
    }
    if (HID_COMPATIBLE_NUM_CHANNELS != audioFormat.numChannels) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithHIDKWFailed")
                        .d("reason", "incompatibleNumChannels")
                        .d("gpiowwNumChannels", HID_COMPATIBLE_NUM_CHANNELS)
                        .d("numChannels", audioFormat.numChannels));
        return false;
    }
    return true;
}

std::unique_ptr<HIDKeywordDetector> HIDKeywordDetector::create(
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

    if (!isAudioFormatCompatibleWithHIDKW(audioFormat)) {
        return nullptr;
    }

    std::unique_ptr<HIDKeywordDetector> detector(new HIDKeywordDetector(
        stream, keyWordObservers, keyWordDetectorStateObservers, audioFormat));

    if (!detector->init()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initDetectorFailed"));
        return nullptr;
    }

    return detector;
}

HIDKeywordDetector::HIDKeywordDetector(
    std::shared_ptr<AudioInputStream> stream,
    std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
    std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
    avsCommon::utils::AudioFormat audioFormat,
    std::chrono::milliseconds msToPushPerIteration) :
        AbstractKeywordDetector(keyWordObservers, keyWordDetectorStateObservers),
        m_stream{stream},
        m_maxSamplesPerPush((audioFormat.sampleRateHz / HERTZ_PER_KILOHERTZ) * msToPushPerIteration.count()) {
}

HIDKeywordDetector::~HIDKeywordDetector() {
    m_isShuttingDown = true;
    if (m_detectionThread.joinable())
        m_detectionThread.join();
    if (m_readAudioThread.joinable())
        m_readAudioThread.join();

}

bool HIDKeywordDetector::init() {
    m_streamReader = m_stream->createReader(AudioInputStream::Reader::Policy::BLOCKING);
    if (!m_streamReader) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createStreamReaderFailed"));
        return false;
    }

    if (openUSBDevice(&m_evdev, &m_devh) != 0) {
        return false;
    }

    m_isShuttingDown = false;
    m_readAudioThread = std::thread(&HIDKeywordDetector::readAudioLoop, this);
    m_detectionThread = std::thread(&HIDKeywordDetector::detectionLoop, this);
    return true;
}

void HIDKeywordDetector::readAudioLoop() {
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

void HIDKeywordDetector::detectionLoop() {
    notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
    int rc = 1;

    std::chrono::steady_clock::time_point prev_time;
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

    while (!m_isShuttingDown) {
        auto current_index = m_streamReader->tell();
        struct input_event ev;
        rc = libevdev_next_event(m_evdev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        // wait for HID_KEY_CODE true event
        if (rc == 0 && strcmp(libevdev_event_type_get_name(ev.type), "EV_KEY")==0 && \
            strcmp(libevdev_event_code_get_name(ev.type, ev.code), HID_KEY_CODE)==0 && \
            ev.value == 1)
        {
            current_index = m_streamReader->tell();

            std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();
            ACSDK_DEBUG0(LX("detectionLoopHIDevent").d("absoluteElapsedTime (ms)", std::chrono::duration_cast<std::chrono::milliseconds> (current_time - start_time).count()));

            // Check if this is not the first HID event
            if (prev_time != std::chrono::steady_clock::time_point()) {
                ACSDK_DEBUG0(LX("detectionLoopHIDevent").d("elapsedTimeFromPreviousEvent (ms)", std::chrono::duration_cast<std::chrono::milliseconds> (current_time - prev_time).count()));
            }
            prev_time = current_time;

            // Retrieve device indexes using control message via USB
            unsigned char payload[64];

            uint8_t cmd_ret = 1;

            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
            while (cmd_ret!=0) {
                rc = libusb_control_transfer(m_devh,
                    LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                    0, CONTROL_CMD_ID, CONTROL_RESOURCE_ID, payload, CONTROL_CMD_PAYLOAD_LEN, USB_TIMEOUT_MS);

                cmd_ret = payload[0];
            }
            if (rc != CONTROL_CMD_PAYLOAD_LEN) {
                 ACSDK_ERROR(LX("detectionLoopControlCommand").d("reason", "USBControlTransferFailed"));
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
    }
}

}  // namespace kwd
}  // namespace alexaClientSDK
