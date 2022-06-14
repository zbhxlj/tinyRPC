include(GoogleTest)

#add_subdirectory(plugin)

file(GLOB src_pb *.cpp *.h *.cc)
list(FILTER src_pb EXCLUDE REGEX "Test.cpp$")

add_library(pb STATIC ${src_pb})



# #ServiceMethodLocatorTest
# add_executable(ServiceMethodLocatorTest ServiceMethodLocatorTest.cpp)
# target_include_directories(ServiceMethodLocatorTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
# target_link_libraries(ServiceMethodLocatorTest 
#     pb 
#     ${Protobuf_LIBRARIES} 
#     testing io fiber base 
#     ${libcommon}
# )
# gtest_discover_tests(ServiceMethodLocatorTest)


# #rpcServerControllerTest
# add_executable(rpcServerControllerTest rpcServerControllerTest.cpp)
# target_include_directories(rpcServerControllerTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
# target_link_libraries(rpcServerControllerTest 
#     pb 
#     ${Protobuf_LIBRARIES} 
#     testing io fiber base 
#     ${libcommon}
# )
# gtest_discover_tests(rpcServerControllerTest)