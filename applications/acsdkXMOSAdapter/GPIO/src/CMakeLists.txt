add_definitions("-DACSDK_LOG_MODULE=GPIOKeywordDetector")

add_library(GPIO
    GPIOKeywordDetector.cpp)

target_include_directories(GPIO PUBLIC
    "${GPIO_SOURCE_DIR}/../XMOS/include"
    "${GPIO_SOURCE_DIR}/include")

target_link_libraries(GPIO
    XMOS
    acsdkKWDImplementations
    acsdkKWDInterfaces
    AVSCommon
    wiringPi)

# install target
asdk_install()
