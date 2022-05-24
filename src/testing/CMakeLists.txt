add_library(testing STATIC main.h main.cpp)
add_dependencies(testing tinyRPC)
target_link_libraries(testing tinyRPC) 