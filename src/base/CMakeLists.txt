include(GoogleTest)

file(GLOB_RECURSE src_base ./**.cpp)
list(REMOVE_ITEM src_base ./**Test.cpp)

add_library(base STATIC ${src_base})

#SpinLockTest
add_executable(SpinLockTest thread/SpinLockTest.cpp)
target_include_directories(SpinLockTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(SpinLockTest ${libcommon})
gtest_discover_tests(SpinLockTest)

#ErasedPtrTest
add_executable(ErasedPtrTest ErasedPtrTest.cpp)
target_include_directories(ErasedPtrTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ErasedPtrTest ${libcommon})

gtest_discover_tests(ErasedPtrTest)

#ScopedDeferredTest
add_executable(ScopedDeferredTest ScopedDeferredTest.cpp)
target_include_directories(ScopedDeferredTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ScopedDeferredTest ${libcommon})

gtest_discover_tests(ScopedDeferredTest)

#DoublyLinkedListTest
add_executable(DoublyLinkedListTest DoublyLinkedListTest.cpp)
target_include_directories(DoublyLinkedListTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(DoublyLinkedListTest ${libcommon})

gtest_discover_tests(DoublyLinkedListTest)