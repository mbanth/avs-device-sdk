add_definitions("-DACSDK_LOG_MODULE=XMOS")
add_library(XMOS
    XMOSKeywordDetector.cpp)

target_include_directories(XMOS PUBLIC
        "${XMOS_SOURCE_DIR}/include")

target_link_libraries(XMOS
    acsdkKWDImplementations
    acsdkKWDInterfaces
    AVSCommon)

# install target
asdk_install()
