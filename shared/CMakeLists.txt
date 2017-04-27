cmake_minimum_required(VERSION 3.6)
project(shared)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        packet.cpp
        packet.h)

add_executable(shared ${SOURCE_FILES})