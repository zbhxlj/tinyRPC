file(GLOB_RECURSE src_fiber_detail "detail/*.cpp" "detai/*.S")

add_library(fiber_detail STATIC
        ${src_fiber_detail}
        ) 