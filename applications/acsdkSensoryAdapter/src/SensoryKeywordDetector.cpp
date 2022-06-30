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
#define SNSR_RC_OK 0
#include <memory>

#include <acsdkKWDImplementations/KWDNotifierFactories.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "Sensory/SensoryKeywordDetector.h"

namespace alexaClientSDK {
namespace kwd {

using namespace avsCommon::utils::logger;

/// String to identify log entries originating from this file.
static const std::string TAG("SensoryKeywordDetector");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The number of hertz per kilohertz.
static const size_t HERTZ_PER_KILOHERTZ = 1000;

/// The timeout to use for read calls to the SharedDataStream.
const std::chrono::milliseconds TIMEOUT_FOR_READ_CALLS = std::chrono::milliseconds(1000);

/// The Sensory compatible AVS sample rate of 16 kHz.
static const unsigned int SENSORY_COMPATIBLE_SAMPLE_RATE = 16000;

/// The Sensory compatible bits per sample of 16.
static const unsigned int SENSORY_COMPATIBLE_SAMPLE_SIZE_IN_BITS = 16;

/// The Sensory compatible number of channels, which is 1.
static const unsigned int SENSORY_COMPATIBLE_NUM_CHANNELS = 1;

/// The Sensory compatible audio encoding of LPCM.
static const avsCommon::utils::AudioFormat::Encoding SENSORY_COMPATIBLE_ENCODING =
    avsCommon::utils::AudioFormat::Encoding::LPCM;

/// The Sensory compatible endianness which is little endian.
static const avsCommon::utils::AudioFormat::Endianness SENSORY_COMPATIBLE_ENDIANNESS =
    avsCommon::utils::AudioFormat::Endianness::LITTLE;

/**
 * Checks to see if an @c avsCommon::utils::AudioFormat is compatible with Sensory.
 *
 * @param audioFormat The audio format to check.
 * @return @c true if the audio format is compatible with Sensory and @c false otherwise.
 */
static bool isAudioFormatCompatibleWithSensory(avsCommon::utils::AudioFormat audioFormat) {
    if (SENSORY_COMPATIBLE_ENCODING != audioFormat.encoding) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithSensoryFailed")
                        .d("reason", "incompatibleEncoding")
                        .d("sensoryEncoding", SENSORY_COMPATIBLE_ENCODING)
                        .d("encoding", audioFormat.encoding));
        return false;
    }
    if (SENSORY_COMPATIBLE_ENDIANNESS != audioFormat.endianness) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithSensoryFailed")
                        .d("reason", "incompatibleEndianess")
                        .d("sensoryEndianness", SENSORY_COMPATIBLE_ENDIANNESS)
                        .d("endianness", audioFormat.endianness));
        return false;
    }
    if (SENSORY_COMPATIBLE_SAMPLE_RATE != audioFormat.sampleRateHz) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithSensoryFailed")
                        .d("reason", "incompatibleSampleRate")
                        .d("sensorySampleRate", SENSORY_COMPATIBLE_SAMPLE_RATE)
                        .d("sampleRate", audioFormat.sampleRateHz));
        return false;
    }
    if (SENSORY_COMPATIBLE_SAMPLE_SIZE_IN_BITS != audioFormat.sampleSizeInBits) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithSensoryFailed")
                        .d("reason", "incompatibleSampleSizeInBits")
                        .d("sensorySampleSizeInBits", SENSORY_COMPATIBLE_SAMPLE_SIZE_IN_BITS)
                        .d("sampleSizeInBits", audioFormat.sampleSizeInBits));
        return false;
    }
    if (SENSORY_COMPATIBLE_NUM_CHANNELS != audioFormat.numChannels) {
        ACSDK_ERROR(LX("isAudioFormatCompatibleWithSensoryFailed")
                        .d("reason", "incompatibleNumChannels")
                        .d("sensoryNumChannels", SENSORY_COMPATIBLE_NUM_CHANNELS)
                        .d("numChannels", audioFormat.numChannels));
        return false;
    }
    return true;
}

#define SNSR_RES_BEGIN_SAMPLE 1
SnsrRC SensoryKeywordDetector::keyWordDetectedCallback(SnsrSession s, const char* key, void* userData) {
    SensoryKeywordDetector* engine = static_cast<SensoryKeywordDetector*>(userData);
    //SnsrRC result;
    const char* keyword = "";
    double begin = 0;
    double end = 0;
/*
    result = snsrGetDouble(s, SNSR_RES_BEGIN_SAMPLE, &begin);
    if (result != SNSR_RC_OK) {
        ACSDK_ERROR(LX("keyWordDetectedCallbackFailed")
                        .d("reason", "invalidBeginIndex")
                        .d("error", getSensoryDetails(s, result)));
        return result;
    }

    result = snsrGetDouble(s, SNSR_RES_END_SAMPLE, &end);
    if (result != SNSR_RC_OK) {
        ACSDK_ERROR(LX("keyWordDetectedCallbackFailed")
                        .d("reason", "invalidEndIndex")
                        .d("error", getSensoryDetails(s, result)));
        return result;
    }

    result = snsrGetString(s, SNSR_RES_TEXT, &keyword);
    if (result != SNSR_RC_OK) {
        ACSDK_ERROR(LX("keyWordDetectedCallbackFailed")
                        .d("reason", "keywordRetrievalFailure")
                        .d("error", getSensoryDetails(s, result)));
        return result;
    }
*/
    engine->notifyKeyWordObservers(
        engine->m_stream,
        keyword,
        engine->m_beginIndexOfStreamReader + begin,
        engine->m_beginIndexOfStreamReader + end);
    return SNSR_RC_OK;
}

// Deprecated create method.
std::unique_ptr<SensoryKeywordDetector> SensoryKeywordDetector::create(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    avsCommon::utils::AudioFormat audioFormat,
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>
        keyWordDetectorStateObservers,
    const std::string& modelFilePath,
    const uint32_t& snsrOperatingPoint,
    std::chrono::milliseconds msToPushPerIteration) {
    // Create Notifiers to be used instead of the observers.
    auto keywordNotifier = acsdkKWDImplementations::KWDNotifierFactories::createKeywordNotifier();
    for (auto kwObserver : keyWordObservers) {
        keywordNotifier->addObserver(kwObserver);
    }

    auto keywordDetectorStateNotifier =
        acsdkKWDImplementations::KWDNotifierFactories::createKeywordDetectorStateNotifier();
    for (auto kwdStateObserver : keyWordDetectorStateObservers) {
        keywordDetectorStateNotifier->addObserver(kwdStateObserver);
    }

    return create(
        stream,
        std::make_shared<avsCommon::utils::AudioFormat>(audioFormat),
        keywordNotifier,
        keywordDetectorStateNotifier,
        modelFilePath,
	snsrOperatingPoint,
        msToPushPerIteration);
}

std::unique_ptr<SensoryKeywordDetector> SensoryKeywordDetector::create(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
    const std::shared_ptr<acsdkKWDInterfaces::KeywordNotifierInterface> keywordNotifier,
    const std::shared_ptr<acsdkKWDInterfaces::KeywordDetectorStateNotifierInterface> keywordDetectorStateNotifier,
    const std::string& modelFilePath,
    const uint32_t& snsrOperatingPoint,
    std::chrono::milliseconds msToPushPerIteration) {
    if (!stream) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullStream"));
        return nullptr;
    }

    // TODO: ACSDK-249 - Investigate cpu usage of converting bytes between endianness and if it's not too much, do it.
    if (isByteswappingRequired(*audioFormat)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "endianMismatch"));
        return nullptr;
    }

    if (!isAudioFormatCompatibleWithSensory(*audioFormat)) {
        return nullptr;
    }

    std::unique_ptr<SensoryKeywordDetector> detector(new SensoryKeywordDetector(
        stream, keywordNotifier, keywordDetectorStateNotifier, *audioFormat, msToPushPerIteration));
    if (!detector->init(modelFilePath, snsrOperatingPoint)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initDetectorFailed"));
        return nullptr;
    }

    return detector;
}

SensoryKeywordDetector::~SensoryKeywordDetector() {
    m_isShuttingDown = true;
    if (m_detectionThread.joinable()) {
        m_detectionThread.join();
    }
    //snsrRelease(m_session);
}

SensoryKeywordDetector::SensoryKeywordDetector(
    std::shared_ptr<AudioInputStream> stream,
    std::shared_ptr<acsdkKWDInterfaces::KeywordNotifierInterface> keywordNotifier,
    std::shared_ptr<acsdkKWDInterfaces::KeywordDetectorStateNotifierInterface> keywordDetectorStateNotifier,
    avsCommon::utils::AudioFormat audioFormat,
    std::chrono::milliseconds msToPushPerIteration) :
        acsdkKWDImplementations::AbstractKeywordDetector(keywordNotifier, keywordDetectorStateNotifier),
        m_stream{stream},
        m_session{nullptr},
        m_maxSamplesPerPush((audioFormat.sampleRateHz / HERTZ_PER_KILOHERTZ) * msToPushPerIteration.count()) {
}

bool SensoryKeywordDetector::init(const std::string& modelFilePath,  const uint32_t& snsrOperatingPoint)    {
    m_streamReader = m_stream->createReader(AudioInputStream::Reader::Policy::BLOCKING);
    if (!m_streamReader) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createStreamReaderFailed"));
        return false;
    }
    m_isShuttingDown = false;
    m_detectionThread = std::thread(&SensoryKeywordDetector::detectionLoop, this);
    return true;
}

bool SensoryKeywordDetector::setUpRuntimeSettings(SnsrSession* session) {
    if (!session) {
        ACSDK_ERROR(LX("setUpRuntimeSettingsFailed").d("reason", "nullSession"));
        return false;
    }
    return true;
}

void SensoryKeywordDetector::detectionLoop() {
    m_beginIndexOfStreamReader = m_streamReader->tell();
    notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
    std::vector<int16_t> audioDataToPush(m_maxSamplesPerPush);
    //SnsrRC result;
    while (!m_isShuttingDown) {
        bool didErrorOccur = false;
        auto wordsRead = readFromStream(
            m_streamReader,
            m_stream,
            audioDataToPush.data(),
            audioDataToPush.size(),
            TIMEOUT_FOR_READ_CALLS,
            &didErrorOccur);
        if (didErrorOccur) {
            /*
             * Note that this does not include the overrun condition, which the base class handles by instructing the
             * reader to seek to BEFORE_WRITER.
             */
            break;
        } else if (wordsRead == AudioInputStream::Reader::Error::OVERRUN) {
            /*
             * Updating reference point of Reader so that new indices that get emitted to keyWordObservers can be
             * relative to it.
             */
            m_beginIndexOfStreamReader = m_streamReader->tell();

        }
        // Reset return code for next round
        //snsrClearRC(m_session);
	
    }
    m_streamReader->close();
}

}  // namespace kwd
}  // namespace alexaClientSDK
