cmake_minimum_required (VERSION 3.4)
project(ClueSolver CXX)

#set(ClueSolver_VERSION_MAJOR 0)
#set(ClueSolver_VERSION_MINOR 1)
#configure_file("$(PROJECT_SOURCE_DIR)/Version.h.in" "$(PROJECT_BINARY_DIR)/Version.h")
#include_directories("$(PROJECT_BINARY_DIR)")

add_definitions(
    -DNOMINMAX
    -DWIN32_LEAN_AND_MEAN
    -DVC_EXTRALEAN
    -D_CRT_SECURE_NO_WARNINGS
    -D_SECURE_SCL=0
    -D_SCL_SECURE_NO_WARNINGS
)

option(BUILD_SHARED_LIBS "Build libraries as DLLs" FALSE)

find_package(nlohmann_json)

set(SOURCES
    main.cpp
    Solver.cpp
    Solver.h
)
source_group(Sources FILES ${SOURCES})

add_executable(ClueSolver ${SOURCES})
target_link_libraries(ClueSolver PRIVATE nlohmann_json::nlohmann_json)
