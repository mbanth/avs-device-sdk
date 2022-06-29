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

#include <acsdkKWDImplementations/KWDNotifierFactories.h>
#include <acsdkManufactory/ComponentAccumulator.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <Sensory/SensoryKeywordDetector.h>

#include "acsdkKWD/KWDComponent.h"

namespace alexaClientSDK {
namespace acsdkKWD {

/// String to identify log entries originating from this file.
static const std::string TAG{"SensoryKWDComponent"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The Sensory Config values from AlexaClientSDKConfig.json
static const std::string SAMPLE_APP_CONFIG_ROOT_KEY("sampleApp");
static const std::string SENSORY_CONFIG_ROOT_KEY("sensory");
static const std::string SENSORY_MODEL_FILE_PATH("modelFilePath");
#ifdef SENSORY_OP_POINT
static const std::string SENSORY_SNSR_OPERATING_POINT("snsrOperatingPoint");
#endif // SENSORY_OP_POINT
static std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector> createAbstractKeywordDetector(
    const std::shared_ptr<avsCommon::avs::AudioInputStream>& stream,
    const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
    std::shared_ptr<acsdkKWDInterfaces::KeywordNotifierInterface> keywordNotifier,
    std::shared_ptr<acsdkKWDInterfaces::KeywordDetectorStateNotifierInterface> keywordDetectorStateNotifier) {
    std::string modelFilePath;
#ifdef SENSORY_OP_POINT
    int snsrOperatingPoint;
#endif // SENSORY_OP_POINT
    auto config = avsCommon::utils::configuration::ConfigurationNode::getRoot()[SAMPLE_APP_CONFIG_ROOT_KEY]
                                                                               [SENSORY_CONFIG_ROOT_KEY];
    if (config) {
        config.getString(SENSORY_MODEL_FILE_PATH, &modelFilePath);
#ifdef SENSORY_OP_POINT
        config.getUint32(SENSORY_SNSR_OPERATING_POINT, &snsrOperatingPoint);
#endif // SENSORY_OP_POINT
    }
    if (modelFilePath.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyModelFilePath"));
        return nullptr;
    }
#ifdef SENSORY_OP_POINT
    if (snsrOperatingPoint==0) {
        ACSDK_ERROR(LX("createFailed").d("reason", "zeroSnsrOperatingPoint"));
        return nullptr;
    }
#endif // SENSORY_OP_POINT

    return kwd::SensoryKeywordDetector::create(
        stream, audioFormat, keywordNotifier, keywordDetectorStateNotifier, modelFilePath \
#ifdef SENSORY_OP_POINT
        , snsrOperatingPoint
#endif // SENSORY_OP_POINT
        );
};

KWDComponent getComponent() {
    return acsdkManufactory::ComponentAccumulator<>()
        .addRetainedFactory(createAbstractKeywordDetector)
        .addRetainedFactory(acsdkKWDImplementations::KWDNotifierFactories::createKeywordDetectorStateNotifier)
        .addRetainedFactory(acsdkKWDImplementations::KWDNotifierFactories::createKeywordNotifier);
}

}  // namespace acsdkKWD
}  // namespace alexaClientSDK
