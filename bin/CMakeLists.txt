if(NOT BUILD_BIN)
    return()
endif()


# dimacs
# -------------------------------------------------------------------------
add_executable(dimacs dimacs.cpp)
target_link_libraries(dimacs PRIVATE
        graph_canon
        Boost::program_options
        Boost::random)
target_compile_options(dimacs PRIVATE -Wall -Wextra -pedantic
        -Wno-unused-parameter
        -Wno-comment
        -Wno-sign-compare
        -Wno-unused-local-typedefs)
install(TARGETS dimacs
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT GraphCanon_run)


# graph-canon-* binaries
# -------------------------------------------------------------------------
install(PROGRAMS graph-canon
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT GraphCanon_run)

# Note: yes, we do dirty flag manipulation of each executable to make sure
#       the 'test' programs has debug information and still optimization,
#       and the 'benchmark' programs has NDEBUG and optimization.
# TODO: is there a better way to do it?

if(BUILD_BIN_BENCHMARK)
    set(modes test benchmark)
else()
    set(modes test)
endif()
if(BUILD_BIN_ALL_ALG)
    set(autPruners none basic schreier)
    set(nodeInvBools 0 1)
else()
    set(autPruners basic schreier)
    set(nodeInvBools 1)
endif()
foreach(mode ${modes})
    foreach(eLabel no-el)
        foreach(refine WL-1)
            string(REPLACE - _ refineId ${refine})
            foreach(autPruner ${autPruners})
                foreach(degreeOne 0 1)
                foreach(autImplicit 0 1)
                foreach(partialLeaf ${nodeInvBools})
                foreach(trace ${nodeInvBools})
                foreach(quotient ${nodeInvBools})
                    set(names graph-canon-${mode} ${eLabel} ${refine})
                    set(defs)
                    if("${mode}" STREQUAL "benchmark")
                        list(APPEND defs NDEBUG)
                    endif()
                    if("${eLabel}" STREQUAL "el")
                        list(APPEND defs GRAPH_CANON_EDGE_LABELS)
                    endif()
                    list(APPEND defs GRAPH_CANON_REFINE=${refineId})
                    if(NOT ${autPruner} STREQUAL "none")
                        list(APPEND names ${autPruner})
                        list(APPEND defs GRAPH_CANON_AUT_PRUNER=${autPruner})
                    endif()
                    if(autImplicit)
                        list(APPEND names aut-implicit)
                        list(APPEND defs GRAPH_CANON_AUT_IMPLICIT)
                    endif()
                    if(partialLeaf)
                        list(APPEND names partial-leaf)
                        list(APPEND defs GRAPH_CANON_PARTIAL_LEAF)
                    endif()
                    if(trace)
                        list(APPEND names trace)
                        list(APPEND defs GRAPH_CANON_TRACE)
                    endif()
                    if(quotient)
                        list(APPEND names quotient)
                        list(APPEND defs GRAPH_CANON_QUOTIENT)
                    endif()
                    if(degreeOne)
                        list(APPEND names d1)
                        list(APPEND defs GRAPH_CANON_DEGREE_1)
                    endif()
                    string(REPLACE ";" "_" name "${names}" )
                    # JOIN only from 3.12
                    #string(JOIN _ name ${names})
                    add_executable(${name} graph_canon_${mode}.cpp)
                    target_compile_definitions(${name} PRIVATE ${defs})
                    target_compile_options(${name} PRIVATE -O3 -fno-stack-protector)
                    if("${mode}" STREQUAL "test")
                        target_compile_options(${name} PRIVATE -g)
                    endif()
                    target_link_libraries(${name}
                            PRIVATE
                                graph_canon
                                Boost::chrono
                                Boost::program_options)
                    target_compile_options(${name} PRIVATE -Wall -Wextra -pedantic
                            -Wno-unused-parameter
                            -Wno-comment
                            -Wno-sign-compare
                            -Wno-unused-local-typedefs)
                    install(TARGETS ${name}
                            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/GraphCanon
                            COMPONENT GraphCanon_run)
                endforeach(quotient)
                endforeach(trace)
                endforeach(partialLeaf)
                endforeach(autImplicit)
                endforeach(degreeOne)
            endforeach(autPruner)
        endforeach(refine)
    endforeach(eLabel)
endforeach(mode)
