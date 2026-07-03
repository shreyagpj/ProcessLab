cmake_minimum_required(VERSION 3.10)
project(os_sim)

set(CMAKE_CXX_STANDARD 17)

add_executable(os_sim
    src/main.cpp
    src/Memory.cpp
    src/Scheduler.cpp
    src/OS.cpp
    src/tests.cpp
)

target_include_directories(os_sim PRIVATE src)
