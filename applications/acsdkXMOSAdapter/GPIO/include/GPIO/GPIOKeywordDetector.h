// Copyright (c) 2021-2022 XMOS LIMITED. This Software is subject to the terms of the
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

#ifndef ALEXA_CLIENT_SDK_KWD_XMOS_GPIO_INCLUDE_GPIO_GPIOKEYWORDDETECTOR_H_
#define ALEXA_CLIENT_SDK_KWD_XMOS_GPIO_INCLUDE_GPIO_GPIOKEYWORDDETECTOR_H_

#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>
#include <wiringPi.h>

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

// A specialization of a KeyWordEngine, where a trigger comes from GPIO
class GPIOKeywordDetector : public XMOSKeywordDetector {
public:
    /**
     * Creates a @c GPIOKeywordDetector.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param msToPushPerIteration The amount of data in milliseconds to push to the cloud  at a time. This was the amount used by
     * Sensory in example code.
     * @return A new @c GPIOKeywordDetector, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<GPIOKeywordDetector> create(
        std::shared_ptr<AudioInputStream> stream,
        avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

    /**
     * Destructor.
     */
    ~GPIOKeywordDetector();

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
    GPIOKeywordDetector(
        std::shared_ptr<AudioInputStream> stream,
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
        avsCommon::utils::AudioFormat audioFormat,
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

    /**
     * Initializes the stream reader, sets up the GPIO pin, and kicks off threads to begin processing data from
     * the stream. This function should only be called once with each new @c GPIOKeywordDetector.
     *
     * @return @c true if the engine was initialized properly and @c false otherwise.
     */
    bool init();

    /**
     * Open the I2C port connected to the device
     *
     * @return @c true if file descriptor for the connected device is correctly set
    */
    bool openDevice();

    /// Re-declaration of base class member function
    void detectionLoop();

    /// The file descriptor to access I2C port
    int m_fileDescriptor;

};
}  // namespace kwd
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_KWD_GPIO_INCLUDE_GPIO_GPIOKEYWORDDETECTOR_H_
