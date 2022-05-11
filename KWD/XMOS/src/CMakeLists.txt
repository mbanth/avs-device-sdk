add_definitions("-DACSDK_LOG_MODULE=xmosKeywordDetector")

include_directories(XMOS "${XMOS_SOURCE_DIR}/include")
target_link_libraries(XMOS KWDu AVSCommon)

# install target
asdk_install()
