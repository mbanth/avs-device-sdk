add_library(KeywordDetectorProvider SHARED
    KeywordDetectorProvider.cpp)

target_include_directories(KeywordDetectorProvider PUBLIC
	"${KeywordDetectorProvider_SOURCE_DIR}/include")

target_link_libraries(KeywordDetectorProvider KWD AVSCommon)

if(SENSORY_KEY_WORD_DETECTOR)
    target_link_libraries(KeywordDetectorProvider SENSORY)
endif()

if(HID_KEY_WORD_DETECTOR)
    target_link_libraries(KeywordDetectorProvider XMOS HID)
endif()

if(GPIO_KEY_WORD_DETECTOR)
    target_link_libraries(KeywordDetectorProvider XMOS GPIO)
endif()

# install target
asdk_install()
