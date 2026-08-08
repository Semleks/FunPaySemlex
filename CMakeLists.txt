cmake_minimum_required(VERSION 4.2)
project(FunPaySemlex)

set(CMAKE_CXX_STANDARD 20)

add_executable(FunPaySemlex main.cpp
        src/Config.cpp
        include/Config.h
        src/SystemUtils.cpp
        include/SystemUtils.h)

target_include_directories(FunPaySemlex PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)