include_directories(${PROJECT_INCLUDE_DIR})

add_library(common 
    allocator.c
    cstring.c
    file_stream.c
    log.c
    util.c
    )