if(NOT BUILD_TESTING)
    return()
endif()

add_custom_target(tests DEPENDS ${graph_canon_TEST_FILES})

set(sanFlags -g -fsanitize=undefined -fsanitize=address -fsanitize=leak)
foreach(testName ${graph_canon_TEST_FILES})
    add_executable(${testName} EXCLUDE_FROM_ALL ${testName}.cpp)
    target_include_directories(${testName}
            PRIVATE
                ${PROJECT_SOURCE_DIR}/bin
                ${Boost_INCLUDE_DIR})
    target_link_libraries(${testName} PRIVATE
            graph_canon
            Boost::random)
    target_compile_options(${testName} PRIVATE ${sanFlags})
    target_link_libraries(${testName} PRIVATE ${sanFlags})
    add_test(${testName} ${testName})
endforeach()

add_subdirectory(cmake_add_subdirectory)
add_subdirectory(cmake_import)