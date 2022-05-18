// Copyright (c) 2022 XMOS LIMITED. This Software is subject to the terms of the
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

#ifndef ALEXA_CLIENT_SDK_KWD_XMOS_INCLUDE_XMOS_XMOSKEYWORDDETECTOR_H_
#define ALEXA_CLIENT_SDK_KWD_XMOS_INCLUDE_XMOS_XMOSKEYWORDDETECTOR_H_

#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>
#include <wiringPi.h>

#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>

#include "KWD/AbstractKeywordDetector.h"

namespace alexaClientSDK {
namespace kwd {

/// Keyword string
static const std::string KEYWORD_STRING = "alexa";

/// The number of hertz per kilohertz.
static const size_t HERTZ_PER_KILOHERTZ = 1000;

/// The timeout to use for read calls to the SharedDataStream.
const std::chrono::milliseconds TIMEOUT_FOR_READ_CALLS = std::chrono::milliseconds(1000);

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

// A specialization of a KeyWordEngine, where a trigger comes from GPIO
class XMOSKeywordDetector : public AbstractKeywordDetector {

protected:

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
    XMOSKeywordDetector(
        std::shared_ptr<AudioInputStream> stream,
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
        avsCommon::utils::AudioFormat audioFormat,
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

    /**
     * Destructor.
     */
    ~XMOSKeywordDetector();

    /**
     * Initializes the stream reader, sets up the connection to the device, and kicks off thread to begin reading 
     * the audio stream. This function should only be called once with each new @c XMOSKeywordDetector.
     *
     * @return @c true if the engine was initialized properly and @c false otherwise.
     */
    bool init();

    /// Function to establish a connection with an XMOS device
    virtual bool openDevice() = 0; 

    /// The main function that reads data and feeds it into the engine.
    virtual void detectionLoop() = 0;

    /// The main function that reads data and feeds it into the engine.
    void readAudioLoop();

    /**
     * Read a specific index from the payload of the USB control message
     *
     * @param payload The data returned via control message
     * @param start_index The index in the payload to start reading from
     * @return value stored in payload
    */
    uint64_t readIndex(uint8_t* payload, int start_index);

    /// Indicates whether the internal main loop should keep running.
    std::atomic<bool> m_isShuttingDown;

    /// The stream of audio data.
    const std::shared_ptr<avsCommon::avs::AudioInputStream> m_stream;

    /// The reader that will be used to read audio data from the stream.
    std::shared_ptr<avsCommon::avs::AudioInputStream::Reader> m_streamReader;

    /// Internal thread that read audio samples
    std::thread m_readAudioThread;

    /// Internal thread that monitors GPIO pin.
    std::thread m_detectionThread;

    /**
     * The max number of samples to push into the underlying engine per iteration. This will be determined based on the
     * sampling rate of the audio data passed in.
     */
    const size_t m_maxSamplesPerPush;
};

}  // namespace kwd
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_KWD_GPIO_INCLUDE_GPIO_GPIOKEYWORDDETECTOR_H_
