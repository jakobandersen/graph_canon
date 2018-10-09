cmake_minimum_required(VERSION 3.10)

project(CMakeTestProject CXX)

add_subdirectory(${GraphCanon_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/GraphCanon)

add_executable(doStuff main.cpp)
target_link_libraries(doStuff GraphCanon::graph_canon)