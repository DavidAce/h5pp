find_program(DOXYGEN doxygen)
if (NOT DOXYGEN)
    message(STATUS "Target 'docs' disabled (requires doxygen)")
    return ()
endif ()

find_program(SPHINXBUILD sphinx-build)
if (NOT SPHINXBUILD)
    message(STATUS "Target 'docs' disabled (requires sphinx-build)")
    return ()
endif ()

add_custom_target(docs
        COMMAND ${CMAKE_MAKE_PROGRAM} html
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        )

include(GNUInstallDirs)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/h5pp OPTIONAL
        PATTERN ".doctrees" EXCLUDE)