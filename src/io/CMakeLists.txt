include(GoogleTest)
file(GLOB_RECURSE src_io ${PROJECT_SOURCE_DIR}/src/io *.cpp *.h *.cc)
list(FILTER src_io EXCLUDE REGEX "Test.cpp$")
message("${src_io}")

add_library(io STATIC
        ${src_io}
        ) 

add_dependencies(io fiber base)

add_executable(ReadAtMostTest detail/ReadAtMostTest.cpp)
target_include_directories(ReadAtMostTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ReadAtMostTest
        io
        base
        ${libcommon}
        )
                
gtest_discover_tests(ReadAtMostTest)

add_executable(WatchDogTest detail/WatchDogTest.cpp)
target_include_directories(WatchDogTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(WatchDogTest tinyRPC testing)
                
gtest_discover_tests(WatchDogTest)

add_executable(WritingBufferListTest detail/WritingBufferListTest.cpp)
target_include_directories(WritingBufferListTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(WritingBufferListTest tinyRPC testing)
                
gtest_discover_tests(WritingBufferListTest)

add_executable(AcceptorTest native/AcceptorTest.cpp)
target_include_directories(AcceptorTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(AcceptorTest tinyRPC testing)
                
gtest_discover_tests(AcceptorTest)

add_executable(StreamConnectionTest native/StreamConnectionTest.cpp)
target_include_directories(StreamConnectionTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(StreamConnectionTest tinyRPC testing)
                
gtest_discover_tests(StreamConnectionTest)

add_executable(SocketTest util/SocketTest.cpp)
target_include_directories(SocketTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(SocketTest tinyRPC testing)
                
gtest_discover_tests(SocketTest)