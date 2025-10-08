cmake_minimum_required(VERSION 3.10)
project(Producer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Add executable
add_executable(producer main.cpp)

# Link libraries
target_link_libraries(producer ${CMAKE_THREAD_LIBS_INIT})