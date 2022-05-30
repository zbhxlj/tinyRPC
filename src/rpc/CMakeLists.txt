# include(GoogleTest)
# file(GLOB_RECURSE src_rpc ${PROJECT_SOURCE_DIR}/src/rpc *.cpp *.h *.cc)
# list(FILTER src_rpc EXCLUDE REGEX "Test.cpp$")
# message("${src_rpc}")

# add_library(rpc STATIC
#         ${src_rpc}
#         ) 

# add_dependencies(rpc io fiber base)

add_subdirectory(internal)
add_subdirectory(load_balancer)
add_subdirectory(name_resolver)
add_subdirectory(message_dispatcher)
add_subdirectory(protocol/protobuf)