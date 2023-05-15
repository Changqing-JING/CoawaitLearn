cmake_minimum_required(VERSION 3.5)

project(CoawaitLearn)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")

aux_source_directory(src sourceFiles)

add_executable(${PROJECT_NAME} ${sourceFiles})

find_package(libuv REQUIRED)
target_link_libraries(${PROJECT_NAME} uv)