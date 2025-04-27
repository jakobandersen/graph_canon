add_test(NAME cmake_add_subdirectory_configure
        COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
        -DGraphCanon_DIR=${PROJECT_SOURCE_DIR}
        -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}/project)
add_test(NAME cmake_add_subdirectory_build
        COMMAND ${CMAKE_COMMAND} --build .)
set_tests_properties(cmake_add_subdirectory_configure PROPERTIES
        FIXTURES_SETUP cmake_add_subdirectory)
set_tests_properties(cmake_add_subdirectory_build PROPERTIES
        FIXTURES_REQUIRED cmake_add_subdirectory)
