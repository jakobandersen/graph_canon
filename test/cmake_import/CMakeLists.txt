add_test(NAME cmake_import_configure
        COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
        -DBOOST_ROOT=${BOOST_ROOT}
        -DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
        ${CMAKE_CURRENT_SOURCE_DIR}/project)
add_test(NAME cmake_import_build
        COMMAND ${CMAKE_COMMAND} --build .)
set_tests_properties(cmake_import_configure PROPERTIES
        FIXTURES_SETUP cmake_import)
set_tests_properties(cmake_import_build PROPERTIES
        FIXTURES_REQUIRED cmake_import)
