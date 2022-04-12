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

#include "KWD/AbstractKeywordDetector.h"

namespace alexaClientSDK {
namespace kwd {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

// A specialization of a KeyWordEngine, where a trigger comes from HID
class HIDKeywordDetector : public AbstractKeywordDetector {
public:
    /**
     * Creates a @c HIDKeywordDetector.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param msToPushPerIteration The amount of data in milliseconds to push to HID KW at a time. This has been defaulted to 10 milliseconds
     * as it is a good trade off between CPU usage and recognition delay. Additionally, this was the amount used by
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
     * @param msToPushPerIteration The amount of data in milliseconds to push to HID KW at a time.  This has been defaulted to 10 milliseconds
     * as it is a good trade off between CPU usage and recognition delay.
     */
    HIDKeywordDetector(
        std::shared_ptr<AudioInputStream> stream,
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
        avsCommon::utils::AudioFormat audioFormat,
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

    /**
     * Initializes the stream reader, sets up the HID, and kicks off a thread to begin processing data from
     * the stream. This function should only be called once with each new @c HIDKeywordDetector.
     *
     * @return @c true if the engine was initialized properly and @c false otherwise.
     */
    bool init();

    /// The function that updates the audio stream.
    void readAudioLoop();

    /// The function that waits for HID events and notifies the server
    void detectionLoop();

    /// Indicates whether the internal main loop should keep running.
    std::atomic<bool> m_isShuttingDown;

    /// The stream of audio data.
    const std::shared_ptr<avsCommon::avs::AudioInputStream> m_stream;

    /// The reader that will be used to read audio data from the stream.
    std::shared_ptr<avsCommon::avs::AudioInputStream::Reader> m_streamReader;

    /**
     * This serves as a reference point used when notifying observers of keyword detection indices since Sensory has no
     * way of specifying a start index.
     */
    avsCommon::avs::AudioInputStream::Index m_beginIndexOfStreamReader;


    /// The device handler necessary for reading HID events
    struct libevdev *m_evdev;

    //The device handler necessary for sending control commands
    libusb_device_handle *m_devh;

    /// Internal thread that read audio samples
    std::thread m_readAudioThread;

    /// Internal thread that monitors HID.
    std::thread m_detectionThread;

    /**
     * The max number of samples to push into the underlying engine per iteration. This will be determined based on the
     * sampling rate of the audio data passed in.
     */
    const size_t m_maxSamplesPerPush;
};

}  // namespace kwd
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_KWD_HID_INCLUDE_HID_HIDKEYWORDDETECTOR_H_
