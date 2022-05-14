include(GoogleTest)

file(GLOB_RECURSE src_fiber ${PROJECT_SOURCE_DIR}/src/fiber *.cpp)
list(FILTER src_fiber EXCLUDE REGEX "Test.cpp$")

enable_language(ASM)
set(src_fiber
        ${src_fiber} 
        detail/jump_context.S
        detail/make_context.S
)

add_library(fiber STATIC
        ${src_fiber}
        ) 

add_dependencies(fiber base)

add_executable(SchedulingGroupTest detail/SchedulingGroupTest.cpp)
target_include_directories(SchedulingGroupTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(SchedulingGroupTest
        ${libcommon}
        fiber
        base
        )
                
gtest_discover_tests(SchedulingGroupTest)

add_executable(Debug detail/debug.cpp)
target_include_directories(Debug PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(Debug
        ${PROJECT_SOURCE_DIR}/lib/glog/libglog.a
        ${PROJECT_SOURCE_DIR}/lib/gflags/libgflags.a
        ${PROJECT_SOURCE_DIR}/lib/libunwind/libunwind.so
        pthread
        fiber
        base
        )