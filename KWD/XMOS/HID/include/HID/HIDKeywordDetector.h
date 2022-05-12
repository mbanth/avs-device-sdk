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

#ifndef ALEXA_CLIENT_SDK_KWD_HID_INCLUDE_HID_HIDKEYWORDDETECTOR_H_
#define ALEXA_CLIENT_SDK_KWD_HID_INCLUDE_HID_HIDKEYWORDDETECTOR_H_

#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <libusb-1.0/libusb.h>

#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>

#include "XMOS/XMOSKeywordDetector.h"

namespace alexaClientSDK {
namespace kwd {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

// A specialization of a KeyWordEngine, where a trigger comes from HID
class HIDKeywordDetector : public XMOSKeywordDetector {
public:
    /**
     * Creates a @c HIDKeywordDetector.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param msToPushPerIteration The amount of data in milliseconds to push to the cloud  at a time. This was the amount used by
     * Sensory in example code.
     * @return A new @c HIDKeywordDetector, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<HIDKeywordDetector> create(
        std::shared_ptr<AudioInputStream> stream,
        avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

    /**
     * Destructor.
     */
    ~HIDKeywordDetector();

private:
    /**
     * Constructor.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param msToPushPerIteration The amount of data in milliseconds to push to the cloud  at a time. This was the amount used by
     * Sensory in example code.
     */
    HIDKeywordDetector(
        std::shared_ptr<AudioInputStream> stream,
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
        avsCommon::utils::AudioFormat audioFormat,
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

    bool init();

    uint8_t openDevice();

    /// The main function that reads data and feeds it into the engine.
    void detectionLoop();

    /// The device handler necessary for reading HID events
    struct libevdev *m_evdev;

    //The device handler necessary for sending control commands
    libusb_device_handle *m_devh;
};

}  // namespace kwd
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_KWD_HID_INCLUDE_HID_HIDKEYWORDDETECTOR_H_
