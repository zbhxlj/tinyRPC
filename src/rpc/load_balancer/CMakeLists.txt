include(GoogleTest)
file(GLOB_RECURSE src_lb *.cpp *.h *.cc)
list(FILTER src_lb EXCLUDE REGEX "Test.cpp$")

add_library(lb STATIC ${src_lb})