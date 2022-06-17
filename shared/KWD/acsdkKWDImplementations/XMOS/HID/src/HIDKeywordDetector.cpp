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
#include <dirent.h>

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

/// HID keycode to monitor:
static const char * HID_KEY_CODE = "KEY_T";

/// HID device directory path
static const std::string HID_DEVICE_DIR_PATH =  "/dev/input/";

/// HID device name
static const std::string HID_DEVICE_NAME = "XMOS XVF3615 Voice Processor Keyboard";

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

/// The length of the payload of the XMOS control command
/// one control byte plus 3 uint64_t values
static const int CONTROL_CMD_PAYLOAD_LEN = 25;

bool HIDKeywordDetector::openDevice() {
    int rc = 1;
    libusb_device **devs = NULL;
    libusb_device *dev = NULL;

    ACSDK_INFO(LX("openDeviceOngoing")
               .d("HIDDeviceName", HID_DEVICE_NAME)
               .d("USBVendorID", USB_VENDOR_ID)
               .d("USBProductID", USB_PRODUCT_ID));

    // Find USB device for reading HID events
    int fd = -1;
    DIR *dir;
    struct dirent *entry;

    //search all files in directory
    dir = opendir(HID_DEVICE_DIR_PATH.c_str()); 
    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            std::string file_path(entry->d_name);
            fd = open((HID_DEVICE_DIR_PATH+file_path).c_str(), O_RDONLY|O_NONBLOCK);

            // Do not check if command is successful, as the entries reported by readdir() are not all devices
            rc = libevdev_new_from_fd(fd, &m_evdev);
            if (!rc) {
                if (libevdev_get_name(m_evdev)==HID_DEVICE_NAME) {
                    ACSDK_INFO(LX("openDeviceSuccess").d("reason", "Found HID device").d("path", HID_DEVICE_DIR_PATH+file_path));
                    break;
                }
            }
            fd = -1;
        }
        closedir(dir); //close directory
    }
    if (fd==-1) {
        ACSDK_ERROR(LX("openDeviceFailed").d("reason", "HidDeviceNotFound"));
    }
    // Find USB device for sending control commands
    int ret = libusb_init(NULL);
    if (ret < 0) {
        ACSDK_ERROR(LX("openDeviceFailed").d("reason", "initialiseLibUsbFailed"));
        return false;
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
        ACSDK_ERROR(LX("openDeviceFailed").d("reason", "UsbDeviceNotFound"));
        return false;
    }

    if (libusb_open(dev, &m_devh) < 0) {
        ACSDK_ERROR(LX("openDeviceFailed").d("reason", "UsbDeviceNotOpened"));
        return false;
    }

    libusb_free_device_list(devs, 1);
    ACSDK_INFO(LX("openDeviceSuccess").d("reason", "UsbDeviceOpened"));
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
        XMOSKeywordDetector(stream, keyWordObservers, keyWordDetectorStateObservers, audioFormat, msToPushPerIteration) {
}

HIDKeywordDetector::~HIDKeywordDetector() {
}


bool HIDKeywordDetector::init() {
    if (XMOSKeywordDetector::init()) {
        m_detectionThread = std::thread(&HIDKeywordDetector::detectionLoop, this);
        return true;
    }
    return false;
}

void HIDKeywordDetector::detectionLoop() {
    notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
    int rc = 1;

    std::chrono::steady_clock::time_point prev_time;
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

    while (!m_isShuttingDown) {
        auto currentIndex = m_streamReader->tell();
        struct input_event ev;
        rc = libevdev_next_event(m_evdev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        // wait for HID_KEY_CODE true event
        if (rc == 0 && strcmp(libevdev_event_type_get_name(ev.type), "EV_KEY")==0 && \
            strcmp(libevdev_event_code_get_name(ev.type, ev.code), HID_KEY_CODE)==0 && \
            ev.value == 1)
        {
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
    }
}

}  // namespace kwd
}  // namespace alexaClientSDK
