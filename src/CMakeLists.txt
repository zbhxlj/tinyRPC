add_library(tinyRPC STATIC init.h init.cpp)
add_dependencies(tinyRPC io fiber base)
target_link_libraries(tinyRPC 
    io
    fiber    
    base
    ${libcommon}
) 