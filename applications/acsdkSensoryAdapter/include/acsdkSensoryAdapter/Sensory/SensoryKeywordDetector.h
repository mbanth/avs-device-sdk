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

#ifndef ACSDKSENSORYADAPTER_SENSORY_SENSORYKEYWORDDETECTOR_H_
#define ACSDKSENSORYADAPTER_SENSORY_SENSORYKEYWORDDETECTOR_H_

#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>

#include <acsdkKWDImplementations/AbstractKeywordDetector.h>
#include <acsdkKWDInterfaces/KeywordDetectorStateNotifierInterface.h>
#include <acsdkKWDInterfaces/KeywordNotifierInterface.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/Utils/AudioFormat.h>

//#include "snsr.h"

namespace alexaClientSDK {
namespace kwd {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
#define SnsrSession int
#define SnsrRC int
class SensoryKeywordDetector : public acsdkKWDImplementations::AbstractKeywordDetector {
public:
    /**
     * Creates a @c SensoryKeywordDetector. Requires that the AlexaClientSDKConfig.json has a modelFilePath value 
     * and a snsrOperatingPoint (XMOS-only feature) under sampleApp
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordNotifier The object with which to notifiy observers of keyword detections.
     * @param KeyWordDetectorStateNotifier The object with which to notify observers of state changes in the engine.
     * @param modelFilePath The path to the model file.
     * @param snsrOperatingPoint The operating point of the SNSR (XMOS-only feature).
     * @param msToPushPerIteration The amount of data in milliseconds to push to Sensory at a time. Smaller sizes will
     * lead to less delay but more CPU usage. Additionally, larger amounts of data fed into the engine per iteration
     * might lead longer delays before receiving keyword detection events. This has been defaulted to 10 milliseconds
     * as it is a good trade off between CPU usage and recognition delay. Additionally, this was the amount used by
     * Sensory in example code.
     * @return A new @c SensoryKeywordDetector, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<SensoryKeywordDetector> create(
        const std::shared_ptr<AudioInputStream> stream,
        const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
        std::shared_ptr<acsdkKWDInterfaces::KeywordNotifierInterface> keyWordNotifier,
        std::shared_ptr<acsdkKWDInterfaces::KeywordDetectorStateNotifierInterface> KeyWordDetectorStateNotifier,
        const std::string& modelFilePath,
#ifdef SENSORY_OP_POINT
        const uint32_t& snsrOperatingPoint,
#endif // SENSORY_OP_POINT
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

    /**
     * @deprecated
     * Creates a @c SensoryKeywordDetector. Requires that the AlexaClientSDKConfig.json has a modelFilePath value 
     * and a snsrOperatingPoint (XMOS-only feature) under sampleApp
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param modelFilePath The path to the model file.
     * @param snsrOperatingPoint The operating point of the SNSR (XMOS-only feature).
     * @param msToPushPerIteration The amount of data in milliseconds to push to Sensory at a time. Smaller sizes will
     * lead to less delay but more CPU usage. Additionally, larger amounts of data fed into the engine per iteration
     * might lead longer delays before receiving keyword detection events. This has been defaulted to 10 milliseconds
     * as it is a good trade off between CPU usage and recognition delay. Additionally, this was the amount used by
     * Sensory in example code.
     * @return A new @c SensoryKeywordDetector, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<SensoryKeywordDetector> create(
        const std::shared_ptr<AudioInputStream> stream,
        avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
        const std::string& modelFilePath,
#ifdef SENSORY_OP_POINT
        const uint32_t& snsrOperatingPoint,
#endif // SENSORY_OP_POINT
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

    /**
     * Destructor.
     */
    ~SensoryKeywordDetector() override;

private:
    /**
     * Constructor.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keywordNotifier The object with which to notify observers of keyword detections.
     * @param KeywordDetectorStateNotifier The object with which to notify observers of state changes in the engine.
     * @param msToPushPerIteration The amount of data in milliseconds to push to Sensory at a time. Smaller sizes will
     * lead to less delay but more CPU usage. Additionally, larger amounts of data fed into the engine per iteration
     * might lead longer delays before receiving keyword detection events. This has been defaulted to 10 milliseconds
     * as it is a good trade off between CPU usage and recognition delay. Additionally, this was the amount used by
     * Sensory in example code.
     */
    SensoryKeywordDetector(
        std::shared_ptr<AudioInputStream> stream,
        const std::shared_ptr<acsdkKWDInterfaces::KeywordNotifierInterface> keywordNotifier,
        const std::shared_ptr<acsdkKWDInterfaces::KeywordDetectorStateNotifierInterface> KeywordDetectorStateNotifier,
        avsCommon::utils::AudioFormat audioFormat,
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

    /**
     * Initializes the stream reader, sets up the Sensory engine, and kicks off a thread to begin processing data from
     * the stream. This function should only be called once with each new @c SensoryKeywordDetector.
     *
     * @param modelFilePath The path to the model file.
     * @param snsrOperatingPoint The operating point of the SNSR. (XMOS-only feature).
     * @return @c true if the engine was initialized properly and @c false otherwise.
     */
    bool init(const std::string& modelFilePath
#ifdef SENSORY_OP_POINT
        , const uint32_t& snsrOperatingPoint
#endif // SENSORY_OP_POINT
        );

    /**
     * Sets up the runtime settings for a @c SnsrSession. This includes setting the callback handler and setting the
     * @c SNSR_AUTO_FLUSH setting.
     *
     * @param session The SnsrSession to set up runtime settings for.
     * @return @c true if everything succeeded and @c false otherwise.
     */
    bool setUpRuntimeSettings(SnsrSession* session
#ifdef SENSORY_OP_POINT
        , const uint32_t snsrOperatingPoint
#endif // SENSORY_OP_POINT
        );

    /// The main function that reads data and feeds it into the engine.
    void detectionLoop();

    /**
     * The callback that Sensory will issue to notify of keyword detections.
     *
     * @param s The @c Sensory session handle.
     * @param key The name of the callback setting.
     * @param userData A pointer to the user data to pass along to the callback.
     * @return @c SNSR_RC_OK if everything was processed properly, and a different error code otherwise.
     */
    static SnsrRC keyWordDetectedCallback(SnsrSession s, const char* key, void* userData);

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

    /// Internal thread that reads audio from the buffer and feeds it to the Sensory engine.
    std::thread m_detectionThread;

    /// The Sensory handle.
    SnsrSession m_session;

    /**
     * The max number of samples to push into the underlying engine per iteration. This will be determined based on the
     * sampling rate of the audio data passed in.
     */
    const size_t m_maxSamplesPerPush;
};

}  // namespace kwd
}  // namespace alexaClientSDK

#endif  // ACSDKSENSORYADAPTER_SENSORY_SENSORYKEYWORDDETECTOR_H_
