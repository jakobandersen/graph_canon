cmake_minimum_required(VERSION 3.10)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

###########################################################################
# Package Properties
###########################################################################
file(READ "VERSION" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION) # remove the newline

project(GraphCanon VERSION ${PROJECT_VERSION})
set(GraphCanon_VERSION ${PROJECT_VERSION} CACHE INTERNAL "" FORCE)
set(PNAME_FILE graph-canon) # how the project name should be written in file names
set(CPACK_PACKAGE_CONTACT "Jakob Lykke Andersen <jlandersen@imada.sdu.dk>")


set(CMAKE_CXX_FLAGS_OPTDEBUG "-g -O3")
set(CMAKE_C_FLAGS_OPTDEBUG "-g -O3")
set(CMAKE_EXE_LINKER_FLAGS_OPTDEBUG "")
set(CMAKE_SHARED_LINKER_FLAGS_OPTDEBUG "")
mark_as_advanced(CMAKE_CXX_FLAGS_OPTDEBUG CMAKE_C_FLAGS_OPTDEBUG CMAKE_EXE_LINKER_FLAGS_OPTDEBUG CMAKE_SHARED_LINKER_FLAGS_OPTDEBUG)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
            "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel OptDebug."
            FORCE)
endif()


###########################################################################
# Options
###########################################################################
option(BUILD_BIN "Enable building of any command line tools." ON)
option(BUILD_BIN_BENCHMARK "Enable building of the benchmark version of the selected algorithm configurations." OFF)
option(BUILD_BIN_ALL_ALG "Enable building of all algorithm configurations of otherwise selected command line tools." OFF)
option(BUILD_DOC "Enable documentation building." ON)
option(BUILD_EXAMPLES "Enable example building." OFF)
option(BUILD_TESTING "Enable test building." OFF)
option(BUILD_TESTING_SANITIZERS "Compile tests with sanitizers." ON)
enable_testing()  # should be included here to add the targets in the top-level folder

option(USE_NESTED_PERM_GROUP "Use the PermGroup version in external/perm_group." ON)

get_directory_property(hasParent PARENT_DIRECTORY)
if(hasParent)
    set(BUILD_BIN 0)
    set(BUILD_DOC 0)
    set(BUILD_EXAMPLES 0)
    set(BUILD_TESTING 0)
endif()


###########################################################################
# Individual checks
###########################################################################
set(CMAKE_CXX_STANDARD 14)


# Boost
# -------------------------------------------------------------------------
set(v 1.64.0)
if(BUILD_BIN)
    find_package(Boost ${v} REQUIRED COMPONENTS program_options)
else()
    find_package(Boost ${v} REQUIRED)
endif()


# PermGroup
# -------------------------------------------------------------------------
set(v 0.4)
if(USE_NESTED_PERM_GROUP)
    message(STATUS "PermGroup: using external/perm_group")
    add_subdirectory(external/perm_group)
    if(PermGroup_VERSION VERSION_LESS v)
        message(FATAL_ERROR "Nested PermGroup version not new enough: is ${PermGroup_VERSION}, needs ${v}."
                " Try again after 'git submodule update --init --recursive; ./bootstrap.sh'."
                " Otherwise please report as bug.")
    endif()
else()
    find_package(PermGroup ${v} REQUIRED)
    message(STATUS "PermGroup version: ${PermGroup_VERSION}")
endif()


# Sphinx
# -------------------------------------------------------------------------
if(BUILD_DOC)
    find_program(SPHINX NAMES sphinx-build sphinx-build3 sphinx-build-3 sphinx-build2)
    if(SPHINX)
        message(STATUS "Sphinx: ${SPHINX}")
    else()
        message(FATAL_ERROR "Sphinx not found, required for building the documentation. Use --DBUILD_DOC=off to disable.")
    endif()
endif()


###########################################################################
# Targets and Artefacts
###########################################################################
include(CMakeFiles.txt)
include(GNUInstallDirs)

if(NOT hasParent)
    set(CMAKE_NO_BUILTIN_CHRPATH TRUE)                 #
    # https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
    set(CMAKE_SKIP_BUILD_RPATH FALSE)                  # use RPATH in the build tree
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)          # but not the RPATH used in installation
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}") # there, add this one
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)        # in addition to those deduced automatically
endif()


# libgraph_canon (header-only)
# -------------------------------------------------------------------------
# TODO: the files are ${graph_canon_INCLUDE_FILES}, but should they be put somewhere?
add_library(graph_canon INTERFACE)
add_library(GraphCanon::graph_canon ALIAS graph_canon)
target_include_directories(graph_canon
        INTERFACE
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        ${Boost_INCLUDE_DIR})
target_link_libraries(graph_canon INTERFACE PermGroup::perm_group)
#target_compile_options(graph_canon INTERFACE -Wall -Wextra)
install(TARGETS graph_canon
        EXPORT PROJECT_exports
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT GraphCanon_lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT GraphCanon_lib
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT GraphCanon_lib)
install(DIRECTORY include/graph_canon
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT GraphCanon_dev
        FILES_MATCHING PATTERN "*.hpp")


# config.hpp
# -------------------------------------------------------------------------
configure_file(config.hpp.in include/graph_canon/config.hpp @ONLY)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/graph_canon
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT GraphCanon_dev)


# Subdirs
# -------------------------------------------------------------------------
add_subdirectory(bin)
add_subdirectory(doc)
add_subdirectory(test)
add_subdirectory(examples)


# CMake Package Support
# -------------------------------------------------------------------------
include(CMakePackageConfigHelpers)
# due to the stupid restrictive version formatting we change it
set(exportDir ${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${PROJECT_NAME})
write_basic_package_version_file(
        ${exportDir}/${PROJECT_NAME}Config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)
install(EXPORT PROJECT_exports
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION lib/cmake/${PROJECT_NAME}
        FILE ${PROJECT_NAME}Config.cmake)
install(DIRECTORY ${exportDir}
        DESTINATION lib/cmake
        COMPONENT GraphCanon_dev)


# pkg-config file
# -------------------------------------------------------------------------
set(exportDir ${CMAKE_CURRENT_BINARY_DIR}/lib/pkgconfig)
configure_file(lib/pkgconfig/graph-canon.pc.in ${exportDir}/graph-canon.pc @ONLY)
install(FILES ${exportDir}/graph-canon.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        COMPONENT GraphCanon_dev)


# Packaging
# -------------------------------------------------------------------------
string(TOUPPER ${PROJECT_NAME} PNAME_UPPER)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})  # because the default doesn't include the tweak number
set(CPACK_GENERATOR "DEB")
#set(CPACK_GENERATOR "DEB" "RPM")
set(CPACK_DEB_COMPONENT_INSTALL ON)
#set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# we use CACHE INTERNAL to make it work when in a subproject
set(CPACK_DEBIAN_${PNAME_UPPER}_RUN_PACKAGE_SHLIBDEPS ON                                          CACHE INTERNAL "")
set(CPACK_DEBIAN_${PNAME_UPPER}_LIB_PACKAGE_SHLIBDEPS ON                                          CACHE INTERNAL "")
set(CPACK_DEBIAN_${PNAME_UPPER}_DEV_PACKAGE_SHLIBDEPS ON                                          CACHE INTERNAL "")
set(CPACK_DEBIAN_${PNAME_UPPER}_DOC_PACKAGE_SHLIBDEPS ON                                          CACHE INTERNAL "")

set(CPACK_COMPONENT_${PNAME_UPPER}_RUN_DEPENDS ${PNAME_UPPER}_LIB                                 CACHE INTERNAL "")
set(CPACK_COMPONENT_${PNAME_UPPER}_DEV_DEPENDS ${PNAME_UPPER}_LIB                                 CACHE INTERNAL "")

set(CPACK_DEBIAN_${PNAME_UPPER}_RUN_PACKAGE_NAME    "${PNAME_FILE}"                               CACHE INTERNAL "")
set(CPACK_DEBIAN_${PNAME_UPPER}_LIB_PACKAGE_NAME "lib${PNAME_FILE}"                               CACHE INTERNAL "")
set(CPACK_DEBIAN_${PNAME_UPPER}_DEV_PACKAGE_NAME "lib${PNAME_FILE}-dev"                           CACHE INTERNAL "")
set(CPACK_DEBIAN_${PNAME_UPPER}_DOC_PACKAGE_NAME    "${PNAME_FILE}-doc"                           CACHE INTERNAL "")
set(CPACK_DEBIAN_${PNAME_UPPER}_RUN_FILE_NAME       "${PNAME_FILE}_${PROJECT_VERSION}.deb"        CACHE INTERNAL "")
set(CPACK_DEBIAN_${PNAME_UPPER}_LIB_FILE_NAME    "lib${PNAME_FILE}_${PROJECT_VERSION}.deb"        CACHE INTERNAL "")
set(CPACK_DEBIAN_${PNAME_UPPER}_DEV_FILE_NAME    "lib${PNAME_FILE}-dev_${PROJECT_VERSION}.deb"    CACHE INTERNAL "")
set(CPACK_DEBIAN_${PNAME_UPPER}_DOC_FILE_NAME       "${PNAME_FILE}-doc_${PROJECT_VERSION}.deb"    CACHE INTERNAL "")

#set(CPACK_RPM_${PNAME_UPPER}_RUN_PACKAGE_NAME       "${PNAME_FILE}"                              CACHE INTERNAL "")
#set(CPACK_RPM_${PNAME_UPPER}_LIB_PACKAGE_NAME    "lib${PNAME_FILE}"                              CACHE INTERNAL "")
#set(CPACK_RPM_${PNAME_UPPER}_DEV_PACKAGE_NAME    "lib${PNAME_FILE}-devel"                        CACHE INTERNAL "")
#set(CPACK_RPM_${PNAME_UPPER}_DOC_PACKAGE_NAME       "${PNAME_FILE}-doc"                          CACHE INTERNAL "")
#set(CPACK_RPM_${PNAME_UPPER}_RUN_FILE_NAME          "${PNAME_FILE}-${PROJECT_VERSION}.rpm"       CACHE INTERNAL "")
#set(CPACK_RPM_${PNAME_UPPER}_LIB_FILE_NAME       "lib${PNAME_FILE}-${PROJECT_VERSION}.rpm"       CACHE INTERNAL "")
#set(CPACK_RPM_${PNAME_UPPER}_DEV_FILE_NAME       "lib${PNAME_FILE}-devel-${PROJECT_VERSION}.rpm" CACHE INTERNAL "")
#set(CPACK_RPM_${PNAME_UPPER}_DOC_FILE_NAME          "${PNAME_FILE}-doc-${PROJECT_VERSION}.rpm"   CACHE INTERNAL "")

if(NOT hasParent)
    set(CPACK_SOURCE_PACKAGE_FILE_NAME
            "${PNAME_FILE}-${CPACK_PACKAGE_VERSION}")
    set(CPACK_SOURCE_GENERATOR "TGZ")
    set(CPACK_SOURCE_IGNORE_FILES
            ".swp$"
            "~$"
            "__pycache__"
            *.pyc
            "/.git"
            "/.idea"
            "/build"
            "/stage"
            "/bootstrap.sh"
            "external")
    add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
    include(CPack)
endif()
