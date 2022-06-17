add_definitions("-DACSDK_LOG_MODULE=HIDKeywordDetector")

find_package(PkgConfig)
pkg_check_modules(libevdev REQUIRED libevdev)
find_package(PkgConfig)
pkg_check_modules(libusb-1.0 REQUIRED libusb-1.0)

add_library(HID SHARED
        HIDKeywordDetector.cpp)

target_include_directories(HID PUBLIC
    "${KWD_SOURCE_DIR}/include"
    "${XMOS_SOURCE_DIR}/include"
    "${HID_SOURCE_DIR}/include")

target_link_libraries(HID KWD AVSCommon evdev usb-1.0)

# install target
asdk_install()
