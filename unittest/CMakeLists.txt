enable_testing()

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/bin/unittest)

add_executable(glog_test glog_test.cpp)
target_include_directories(glog_test PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(glog_test
        ${PROJECT_SOURCE_DIR}/lib/glog/libglog.a
        ${PROJECT_SOURCE_DIR}/lib/gflags/libgflags.a
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest_main.a
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest.a
        ${PROJECT_SOURCE_DIR}/lib/libunwind/libunwind.so
        pthread
        )

include(GoogleTest)
gtest_discover_tests(glog_test)