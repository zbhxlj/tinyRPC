include(GoogleTest)

#add_subdirectory(plugin)

file(GLOB src_pb *.cpp *.h *.cc)
list(FILTER src_pb EXCLUDE REGEX "Test.cpp$")

add_library(pb STATIC ${src_pb})

#find_package(Protobuf REQUIRED)
#include_directories(${Protobuf_INCLUDE_DIRS})


# Use custom protobuf libs
# set(PROTOBUF_ROOT ${PROJECT_SOURCE_DIR}/lib/protobuf-3.4.1)
# set ( Protobuf_USE_STATIC_LIBS ON )

# include(${PROTOBUF_ROOT}/cmake/protobuf/protobuf-config.cmake)
# include(${PROTOBUF_ROOT}/cmake/protobuf/protobuf-module.cmake)
# include(${PROTOBUF_ROOT}/cmake/protobuf/protobuf-options.cmake)
# include(${PROTOBUF_ROOT}/cmake/protobuf/protobuf-targets.cmake)

# find_package( Protobuf REQUIRED HINTS ${PROTOBUF_ROOT}/cmake )
# if ( Protobuf_FOUND )
#     message( STATUS "Protobuf version : ${Protobuf_VERSION}" )
#     message( STATUS "Protobuf include path : ${Protobuf_INCLUDE_DIRS}" )
#     message( STATUS "Protobuf libraries : ${Protobuf_LIBRARIES}" )
#     message( STATUS "Protobuf compiler libraries : ${Protobuf_PROTOC_LIBRARIES}")
#     message( STATUS "Protobuf lite libraries : ${Protobuf_LITE_LIBRARIES}")
# else()
#     message( WARNING "Protobuf package not found -> specify search path via PROTOBUF_ROOT variable")
# endif()




#ServiceMethodLocatorTest
add_executable(ServiceMethodLocatorTest ServiceMethodLocatorTest.cpp)
target_include_directories(ServiceMethodLocatorTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ServiceMethodLocatorTest 
    pb 
    ${Protobuf_LIBRARIES} 
    testing io fiber base 
    ${libcommon}
)
gtest_discover_tests(ServiceMethodLocatorTest)


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