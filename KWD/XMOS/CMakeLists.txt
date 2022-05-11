cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(KWD LANGUAGES CXX)

if(HID_KEY_WORD_DETECTOR)
        add_subdirectory("HID")
endif()
if(GPIO_KEY_WORD_DETECTOR)
        add_subdirectory("GPIO"))
endif()
