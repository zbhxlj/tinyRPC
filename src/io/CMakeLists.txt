file(GLOB_RECURSE src_io ${PROJECT_SOURCE_DIR}/src/io *.cpp *.h *.cc)
list(FILTER src_io EXCLUDE REGEX "Test.cpp$")
message("${src_io}")

add_library(io STATIC
        ${src_io}
        ) 

add_dependencies(fiber fiber base)