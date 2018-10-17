cmake_minimum_required(VERSION 3.8)

project(pattern_match)

set(CMAKE_C_STANDARD 99)

set(PROJECT_INCLUDE_DIR
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/container"
    "${PROJECT_SOURCE_DIR}/common"
    )

include_directories(${PROJECT_INCLUDE_DIR})

add_subdirectory(container)
add_subdirectory(common)

add_executable(pattern_match 
    ac.c 
    main.c
    pattern_match.c
    pattern_set.c
)   

target_link_libraries(pattern_match container common)