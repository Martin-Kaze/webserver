cmake_minimum_required(VERSION 3.20)
project(webserver CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(server server.cpp)
target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(server PRIVATE -fsanitize=address,undefined)
    target_link_options(server PRIVATE -fsanitize=address,undefined)
endif()
