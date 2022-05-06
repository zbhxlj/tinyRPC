cmake_minimum_required(VERSION 3.21)

project(tinyRPC)

set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(unittest)
add_subdirectory(src/base)

#add_executable (myapp main.cpp)
#target_link_libraries (myapp glog::glog)
