cmake_minimum_required(VERSION 3.21)
project(leptjson)

set(CMAKE_CXX_STANDARD 14)

set(BUILD_PATH ${CMAKE_SOURCE_DIR}/build) # 自定义路径变量

set(EXECUTABLE_OUTPUT_PATH ${BUILD_PATH}) # EXE

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILD_PATH}) # LIB

add_library(leptjson leptjson.cpp)

add_executable(test test.cpp)

target_link_libraries(test leptjson)