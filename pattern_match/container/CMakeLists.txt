include_directories(${PROJECT_INCLUDE_DIR})

link_libraries(common)

add_library(container 
    list.c
    vector.c
    )