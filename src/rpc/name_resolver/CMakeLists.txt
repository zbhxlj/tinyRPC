include(GoogleTest)
file(GLOB_RECURSE src_nr *.cpp *.h *.cc)
list(FILTER src_nr EXCLUDE REGEX "Test.cpp$")

add_library(nr STATIC ${src_nr})

#NameResolverImplTest
add_executable(NameResolverImplTest NameResolverImplTest.cpp)
target_include_directories(NameResolverImplTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(NameResolverImplTest nr io fiber base ${libcommon})
gtest_discover_tests(NameResolverImplTest)

#DomainNameResolverTest
add_executable(DomainNameResolverTest util/DomainNameResolverTest.cpp)
target_include_directories(DomainNameResolverTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(DomainNameResolverTest nr io fiber base ${libcommon})
gtest_discover_tests(DomainNameResolverTest)

#ListTest
add_executable(ListTest ListTest.cpp)
target_include_directories(ListTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ListTest nr io fiber base ${libcommon})
gtest_discover_tests(ListTest)