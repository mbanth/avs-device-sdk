add_definitions("-DACSDK_LOG_MODULE=XMOSKeywordDetector")
add_library(XMOS SHARED
    XMOSKeywordDetector.cpp)

include_directories(XMOS "${XMOS_SOURCE_DIR}/include")
target_link_libraries(XMOS KWD AVSCommon)

# install target
asdk_install()
