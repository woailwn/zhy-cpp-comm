cmake_minimum_required(VERSION 3.10)
project(zhy_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB APP_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../app/*.cpp")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(zhy_test
    zhy_test.cpp
    ${APP_SRCS}
)

target_link_libraries(zhy_test pthread)
