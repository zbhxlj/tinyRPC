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