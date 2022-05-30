include(GoogleTest)
file(GLOB_RECURSE src_nr *.cpp *.h *.cc)
list(FILTER src_nr EXCLUDE REGEX "Test.cpp$")

add_library(nr STATIC ${src_nr})