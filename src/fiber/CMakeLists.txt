include(GoogleTest)

file(GLOB_RECURSE src_fiber ${PROJECT_SOURCE_DIR}/src/fiber *.cpp)
list(FILTER src_fiber EXCLUDE REGEX "Test.cpp$")
message("${src_fiber}")

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

# FiberEntityTest
add_executable(FiberEntityTest detail/FiberEntityTest.cpp)
target_include_directories(FiberEntityTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(FiberEntityTest
        fiber
        base
        ${libcommon}
        )


gtest_discover_tests(FiberEntityTest)

# TimerWorkerTest
add_executable(TimerWorkerTest detail/TimerWorkerTest.cpp)
target_include_directories(TimerWorkerTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(TimerWorkerTest
        fiber
        base
        ${libcommon}
        )

gtest_discover_tests(TimerWorkerTest)

# FiberWorkerTest
add_executable(FiberWorkerTest detail/FiberWorkerTest.cpp)
target_include_directories(FiberWorkerTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(FiberWorkerTest
        fiber
        base
        ${libcommon}
        )

gtest_discover_tests(FiberWorkerTest)

# WaitableTest
add_executable(WaitableTest detail/WaitableTest.cpp)
target_include_directories(WaitableTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(WaitableTest
        fiber
        base
        ${libcommon}
        )

gtest_discover_tests(WaitableTest)

# ConditionalVariableTest
add_executable(ConditionalVariableTest ConditionalVariableTest.cpp)
target_include_directories(ConditionalVariableTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ConditionalVariableTest
        fiber
        base
        ${libcommon}
        )

gtest_discover_tests(ConditionalVariableTest)

# FiberTest
add_executable(FiberTest FiberTest.cpp)
target_include_directories(FiberTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(FiberTest
        fiber
        base
        ${libcommon}
        )

gtest_discover_tests(FiberTest)

# FiberLocalTest
add_executable(FiberLocalTest FiberLocalTest.cpp)
target_include_directories(FiberLocalTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(FiberLocalTest
        fiber
        base
        ${libcommon}
        )

gtest_discover_tests(FiberLocalTest)

# LatchTest
add_executable(LatchTest LatchTest.cpp)
target_include_directories(LatchTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(LatchTest
        fiber
        base
        ${libcommon}
        )

gtest_discover_tests(LatchTest)

# ThisFiberTest
add_executable(ThisFiberTest ThisFiberTest.cpp)
target_include_directories(ThisFiberTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ThisFiberTest
        fiber
        base
        ${libcommon}
        )

gtest_discover_tests(ThisFiberTest)

# TimerTest
add_executable(TimerTest TimerTest.cpp)
target_include_directories(TimerTest PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(TimerTest
        fiber
        base
        ${libcommon}
        )

gtest_discover_tests(TimerTest)