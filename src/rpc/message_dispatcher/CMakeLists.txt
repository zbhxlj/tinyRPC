include(GoogleTest)
file(GLOB_RECURSE src_msg_dispatch *.cpp *.h *.cc)
list(FILTER src_msg_dispatch EXCLUDE REGEX "Test.cpp$")

add_library(msg_dispatch STATIC ${src_msg_dispatch})
add_dependencies(msg_dispatch nr lb)
target_link_libraries(msg_dispatch nr lb)   