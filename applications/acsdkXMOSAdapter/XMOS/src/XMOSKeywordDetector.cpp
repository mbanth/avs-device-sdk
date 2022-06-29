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

#include "XMOS/XMOSKeywordDetector.h"

namespace alexaClientSDK {
namespace kwd {

/// String to identify log entries originating from this file.
static const std::string TAG("XMOSKeywordDetector");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

XMOSKeywordDetector::~XMOSKeywordDetector() {
    m_isShuttingDown = true;
    if (m_detectionThread.joinable())
        m_detectionThread.join();
    if (m_readAudioThread.joinable())
        m_readAudioThread.join();
}

XMOSKeywordDetector::XMOSKeywordDetector(
    std::shared_ptr<AudioInputStream> stream,
    const std::shared_ptr<acsdkKWDInterfaces::KeywordNotifierInterface> keywordNotifier,
    const std::shared_ptr<acsdkKWDInterfaces::KeywordDetectorStateNotifierInterface> KeywordDetectorStateNotifier,
    avsCommon::utils::AudioFormat audioFormat,
    std::chrono::milliseconds msToPushPerIteration):
        AbstractKeywordDetector(keywordNotifier, KeywordDetectorStateNotifier),
        m_stream{stream},
        m_maxSamplesPerPush((audioFormat.sampleRateHz / HERTZ_PER_KILOHERTZ) * msToPushPerIteration.count()) {
}

bool XMOSKeywordDetector::init() {
    if (!openDevice()) {
        ACSDK_ERROR(LX("initFailed").d("reason", "openDeviceFailed"));
        return false;
    }

    m_streamReader = m_stream->createReader(AudioInputStream::Reader::Policy::BLOCKING);
    if (!m_streamReader) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createStreamReaderFailed"));
        return false;
    }

    m_isShuttingDown = false;
    m_readAudioThread = std::thread(&XMOSKeywordDetector::readAudioLoop, this);
    return true;
}

void XMOSKeywordDetector::readAudioLoop() {
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
        if (didErrorOccur) {
            m_isShuttingDown = true;
        }
    }
}

uint64_t XMOSKeywordDetector::readIndex(uint8_t* payload, int start_index) {
    uint64_t u64value = 0;
    // convert array of bytes into uint64_t value
    memcpy(&u64value, &payload[start_index], sizeof(uint64_t));
    // swap bytes of uint64_t value
    u64value = __bswap_64(u64value);
    return u64value;
}

}  // namespace kwd
}  // namespace alexaClientSDK
