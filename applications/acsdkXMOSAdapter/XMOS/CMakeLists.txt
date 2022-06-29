cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(XMOS LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

# set(TARGET_KWD_LIB "acsdkXMOSAdapter" PARENT_SCOPE)

add_subdirectory("src")
