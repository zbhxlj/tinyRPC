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
        fiber
        base
        ${libcommon}
        )
                
gtest_discover_tests(SchedulingGroupTest)

add_executable(ContextBenchmark detail/ContextBenchmark.cpp)
target_include_directories(ContextBenchmark PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ContextBenchmark
${PROJECT_SOURCE_DIR}/lib/benchmark/libbenchmark.a
${PROJECT_SOURCE_DIR}/lib/benchmark/libbenchmark_main.a
        fiber
        base
        )

add_executable(FiberEntityTest detail/FiberEntityTest.cpp)
target_include_directories(FiberEntityTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(FiberEntityTest
        fiber
        base
        ${libcommon}
        )

gtest_discover_tests(FiberEntityTest)