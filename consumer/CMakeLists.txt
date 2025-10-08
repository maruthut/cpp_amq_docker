cmake_minimum_required(VERSION 3.10)
project(Consumer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Add executable
add_executable(consumer main.cpp)

# Link libraries
target_link_libraries(consumer ${CMAKE_THREAD_LIBS_INIT})