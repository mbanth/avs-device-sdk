add_definitions("-DACSDK_LOG_MODULE=HIDKeywordDetector")

add_library(HID SHARED
        HIDKeywordDetector.cpp)

target_include_directories(HID PUBLIC
    "${KWD_SOURCE_DIR}/include"
    "${XMOS_SOURCE_DIR}/include"
    "${HID_SOURCE_DIR}/include")

target_link_libraries(HID KWD AVSCommon evdev usb-1.0)

# install target
asdk_install()
