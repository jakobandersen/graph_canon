get_filename_component(graph_canon_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)
list(APPEND CMAKE_MODULE_PATH ${graph_canon_CMAKE_DIR})

@graph_canon_config_dependencies@

include("${graph_canon_CMAKE_DIR}/GraphCanonTargets.cmake")