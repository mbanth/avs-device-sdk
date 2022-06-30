add_definitions("-DACSDK_LOG_MODULE=HIDKeywordDetector")

find_package(PkgConfig)
pkg_check_modules(libevdev REQUIRED libevdev)
find_package(PkgConfig)
pkg_check_modules(libusb-1.0 REQUIRED libusb-1.0)

add_library(HID
        HIDKeywordDetector.cpp)

target_include_directories(HID PUBLIC
    "${HID_SOURCE_DIR}/../XMOS/include"
    "${HID_SOURCE_DIR}/include")

target_link_libraries(HID
    XMOS
    acsdkKWDImplementations
    acsdkKWDInterfaces
    AVSCommon
    evdev
    usb-1.0)

# install target
asdk_install()
