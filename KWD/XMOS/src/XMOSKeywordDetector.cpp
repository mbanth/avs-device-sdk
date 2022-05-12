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

XMOSKeywordDetector::XMOSKeywordDetector(
    std::shared_ptr<AudioInputStream> stream,
    std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
    std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
    avsCommon::utils::AudioFormat audioFormat,
    std::chrono::milliseconds msToPushPerIteration) :
        AbstractKeywordDetector(keyWordObservers, keyWordDetectorStateObservers),
        m_stream{stream},
        m_maxSamplesPerPush((audioFormat.sampleRateHz / HERTZ_PER_KILOHERTZ) * msToPushPerIteration.count()) {
}

XMOSKeywordDetector::~XMOSKeywordDetector() {
    m_isShuttingDown = true;
    if (m_detectionThread.joinable())
        m_detectionThread.join();
    if (m_readAudioThread.joinable())
        m_readAudioThread.join();
}

/**
 * Read a specific index from the payload of the USB control message
 *
 * @param payload The data returned via control message
 * @param start_index The index in the payload to start reading from
 * @return value stored in payload
 */
uint64_t readIndex(uint8_t* payload, int start_index) {
    uint64_t u64value = 0;
    // convert array of bytes into uint64_t value
    memcpy(&u64value, &payload[start_index], sizeof(uint64_t));
    // swap bytes of uint64_t value
    u64value = __bswap_64(u64value);
    return u64value;
}

}  // namespace kwd
}  // namespace alexaClientSDK
