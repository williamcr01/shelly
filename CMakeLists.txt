cmake_minimum_required(VERSION 3.20)

project(shelly LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(shelly
    src/main.cpp
    src/command.cpp
    src/executor.cpp
)

target_include_directories(shelly PRIVATE
    include
)

target_compile_options(shelly PRIVATE
    -Wall
    -Wextra
    -pedantic
)
