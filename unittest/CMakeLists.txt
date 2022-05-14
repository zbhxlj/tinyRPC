add_executable(GlogTest GlogTest.cpp)
target_include_directories(GlogTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(GlogTest
        ${PROJECT_SOURCE_DIR}/lib/glog/libglog.a
        ${PROJECT_SOURCE_DIR}/lib/gflags/libgflags.a
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest_main.a
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest.a
        ${PROJECT_SOURCE_DIR}/lib/libunwind/libunwind.so
        pthread
        )

include(GoogleTest)
gtest_discover_tests(GlogTest)