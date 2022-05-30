include(GoogleTest)
file(GLOB_RECURSE src_pb *.cpp *.h *.cc)
list(FILTER src_pb EXCLUDE REGEX "Test.cpp$")

add_library(pb STATIC ${src_pb})