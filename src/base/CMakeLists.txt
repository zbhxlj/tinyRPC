include(GoogleTest)

#SpinLockTest
add_executable(SpinLockTest thread/SpinLockTest.cpp)
target_include_directories(SpinLockTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(SpinLockTest
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest_main.a
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest.a
        pthread
        )
gtest_discover_tests(SpinLockTest)

#ErasedPtrTest
add_executable(ErasedPtrTest ErasedPtrTest.cpp)
target_include_directories(ErasedPtrTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ErasedPtrTest
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest_main.a
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest.a
        pthread
        )

gtest_discover_tests(ErasedPtrTest)

#ScopedDeferredTest
add_executable(ScopedDeferredTest ScopedDeferredTest.cpp)
target_include_directories(ScopedDeferredTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ScopedDeferredTest
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest_main.a
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest.a
        pthread
        )

gtest_discover_tests(ScopedDeferredTest)

#DoublyLinkedListTest
add_executable(DoublyLinkedListTest DoublyLinkedListTest.cpp)
target_include_directories(DoublyLinkedListTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(DoublyLinkedListTest
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest_main.a
        ${PROJECT_SOURCE_DIR}/lib/googletest/libgtest.a
        ${PROJECT_SOURCE_DIR}/lib/glog/libglog.a
        ${PROJECT_SOURCE_DIR}/lib/libunwind/libunwind.so
        pthread
        )

gtest_discover_tests(DoublyLinkedListTest)