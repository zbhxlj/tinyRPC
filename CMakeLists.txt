cmake_minimum_required(VERSION 3.21)

project(tinyRPC)

set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Werror -Wunused -Wno-sign-compare -g -O2 -Wno-comment  -Wno-nonnull-compare
        -Wno-deprecated-declarations) 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# set(Protobuf_SRC_ROOT_FOLDER ${PROJECT_SOURCE_DIR}/include/google/protobuf)
# set(Protobuf_USE_STATIC_LIBS ON)
# set(Protobuf_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)
# set(Protobuf_LIBRARIES ${PROJECT_SOURCE_DIR}/lib/protobuf-3.4.1/libprotobuf.a)
# set(Protobuf_PROTOC_EXECUTABLE ${PROJECT_SOURCE_DIR}/scripts/protoc)

# include_directories(${Protobuf_INCLUDE_DIRS})

FIND_PACKAGE(protobuf REQUIRED)

set(libcommon 
        ${PROJECT_SOURCE_DIR}/lib/fmt/libfmt.a
        ${PROJECT_SOURCE_DIR}/lib/glog/libglog.a
        ${PROJECT_SOURCE_DIR}/lib/gflags/libgflags.a
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest_main.a
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest.a
        ${PROJECT_SOURCE_DIR}/lib/libunwind/libunwind.so
        pthread
)

enable_testing()

add_subdirectory(src/base)
add_subdirectory(src/fiber)
add_subdirectory(src/io)
add_subdirectory(src/rpc)
add_subdirectory(src/testing)
add_subdirectory(src/init)
add_subdirectory(src)
