project(LibAwait)

aux_source_directory(${CMAKE_CURRENT_LIST_DIR} sourceFiles)

add_library(${PROJECT_NAME} ${sourceFiles})