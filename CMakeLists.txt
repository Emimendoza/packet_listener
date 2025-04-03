cmake_minimum_required(VERSION 3.30)
project(PacketCapturer)

set(CMAKE_CXX_STANDARD 23)

find_package(PkgConfig REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(PacketCapturer ${SOURCES}
        )
