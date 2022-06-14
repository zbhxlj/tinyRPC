include(GoogleTest)
file(GLOB_RECURSE src_plugin *.cpp *.h *.cc)
add_library(plugin STATIC ${src_plugin})

include_directories(${Protobuf_INCLUDE_DIRS})
target_link_libraries(plugin ${Protobuf_LIBRARIES})