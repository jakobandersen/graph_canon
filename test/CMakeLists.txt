if(NOT BUILD_TESTING)
    return()
endif()

add_custom_target(tests DEPENDS ${graph_canon_TEST_FILES})

set(sanFlags -g -fsanitize=undefined -fsanitize=address -fsanitize=leak)
foreach(testName ${graph_canon_TEST_FILES})
    add_executable(${testName} EXCLUDE_FROM_ALL ${testName}.cpp)
    target_include_directories(${testName} PRIVATE ${PROJECT_SOURCE_DIR}/bin)
    target_link_libraries(${testName} PRIVATE graph_canon Boost::unit_test_framework)
    if(BUILD_TESTING_SANITIZERS)
        target_compile_options(${testName} PRIVATE ${sanFlags})
        target_link_libraries(${testName} PRIVATE ${sanFlags})
    endif()
    target_compile_options(${testName} PRIVATE -Wall -Wextra -pedantic
            -Wno-sign-compare
            -Wno-unused-parameter
            -Wno-comment
            -Wno-parentheses
            -Wno-unused-local-typedefs
            $<$<CXX_COMPILER_ID:GNU>:-Wno-error=maybe-uninitialized>
            $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9>>:-Wno-error=deprecated-copy>)
    add_test(${testName} ${testName})
endforeach()

add_subdirectory(cmake_add_subdirectory)
add_subdirectory(cmake_import)
