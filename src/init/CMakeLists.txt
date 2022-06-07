file(GLOB src_init *.h *.cpp)

add_library(init STATIC ${src_init})
target_link_libraries(init base)