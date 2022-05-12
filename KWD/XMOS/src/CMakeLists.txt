add_definitions("-DACSDK_LOG_MODULE=XMOSKeywordDetector")
add_library(XMOS SHARED
    XMOSKeywordDetector.cpp)

target_include_directories(XMOS PUBLIC
    "${KWD_SOURCE_DIR}/include"
    "${XMOS_SOURCE_DIR}/include")

target_link_libraries(XMOS KWD AVSCommon)

# install target
asdk_install()
