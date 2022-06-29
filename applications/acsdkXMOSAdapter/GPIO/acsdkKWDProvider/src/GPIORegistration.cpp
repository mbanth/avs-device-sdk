/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions
 * set forth in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED,
 * OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 * A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include <acsdkKWDImplementations/AbstractKeywordDetector.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <GPIO/GPIOKeywordDetector.h>

#include "KWDProvider/KeywordDetectorProvider.h"

/**
 * @deprecated
 * This registration file is not needed if an application uses the manufactory to build its components. For example,
 * the SDK Preview App does not use this registration file any longer.
 *
 * However, for applications that have yet to transition to using the manufactory, this file is provided so those
 * applications can continue to use Keyword Detection (for example, the backwards-compatible SDK Sample App does use
 * this file).
 */
namespace alexaClientSDK {
namespace kwd {

std::unique_ptr<acsdkKWDImplementations::AbstractKeywordDetector> createGPIOKWDAdapter(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    avsCommon::utils::AudioFormat audioFormat,
    std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
    std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers) {

    return kwd::GPIOKeywordDetector::create(
        stream, audioFormat, keyWordObservers, keyWordDetectorStateObservers);
}

/// The registration object to register the XMOS GPIO adapter's creation method.
static const KeywordDetectorProvider::KWDRegistration g_gpioAdapterRegistration(createGPIOKWDAdapter);

}  // namespace kwd
}  // namespace alexaClientSDK
