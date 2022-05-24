file(GLOB_RECURSE src_testing ${PROJECT_SOURCE_DIR}/src/testing *.cpp *.h *.cc)
add_library(testing STATIC ${src_testing})
add_dependencies(testing tinyRPC)
target_link_libraries(testing tinyRPC) 