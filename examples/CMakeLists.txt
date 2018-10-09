if(NOT BUILD_EXAMPLES)
    return()
endif()

add_custom_target(examples DEPENDS ${graph_canon_EXAMPLE_FILES})

foreach(ex ${graph_canon_EXAMPLE_FILES})
    add_executable(${ex} EXCLUDE_FROM_ALL ${ex}.cpp)
    target_link_libraries(${ex} PRIVATE
            graph_canon)
endforeach()