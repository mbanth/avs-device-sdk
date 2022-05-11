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

#ifndef ALEXA_CLIENT_SDK_KWD_GPIO_INCLUDE_GPIO_GPIOKEYWORDDETECTOR_H_
#define ALEXA_CLIENT_SDK_KWD_GPIO_INCLUDE_GPIO_GPIOKEYWORDDETECTOR_H_

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

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

// A specialization of a KeyWordEngine, where a trigger comes from GPIO
class XMOSKeywordDetector : public AbstractKeywordDetector {

        private:
    /**
     * Initializes the stream reader, sets up the GPIO, and kicks off a thread to begin processing data from
     * the stream. This function should only be called once with each new @c GPIOKeywordDetector.
     *
     * @return @c true if the engine was initialized properly and @c false otherwise.
     */
    virtual bool init() = 0;

    /// The main function that reads data and feeds it into the engine.
    virtual void detectionLoop() = 0;
    /// The main function that reads data and feeds it into the engine.
    virtual void readAudioLoop() = 0;

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

    /// The file descriptor to access I2C port
    int m_fileDescriptor;

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
