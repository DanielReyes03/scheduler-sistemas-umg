cmake_minimum_required(VERSION 3.29)
project(scheduler_sistemas_umg)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(scheduler_sistemas_umg
    main.cpp
    main.h)
