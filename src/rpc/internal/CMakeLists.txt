include(GoogleTest)
file(GLOB_RECURSE src_rpc_internal ${PROJECT_SOURCE_DIR}/src/rpc/internal *.cpp *.h *.cc)
list(FILTER src_rpc_internal EXCLUDE REGEX "Test.cpp$")
message("${src_rpc_internal}")

add_library(rpc_internal STATIC
        ${src_rpc_internal}
        ) 
