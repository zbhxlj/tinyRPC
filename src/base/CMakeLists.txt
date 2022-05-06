enable_testing()

# set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/bin/src)

add_executable(SpinLockTest thread/SpinLockTest.cpp)
target_include_directories(SpinLockTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(SpinLockTest
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest_main.a
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest.a
        pthread
        )

include(GoogleTest)
gtest_discover_tests(SpinLockTest)