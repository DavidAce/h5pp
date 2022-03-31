
list(APPEND CONDA_ROOT
        ${CONDA_PREFIX}
        $ENV{HOME}/anaconda3
        $ENV{HOME}/miniconda3
        $ENV{HOME}/anaconda3/envs/h5pp
        $ENV{HOME}/miniconda3/envs/h5pp
        $ENV{HOME}/anaconda3/envs/dmrg
        $ENV{HOME}/miniconda3/envs/dmrg)

list(APPEND Doxygen_ROOT ${CONDA_ROOT})
list(APPEND Sphinx_ROOT ${CONDA_ROOT})

set(DOXYGEN_GENERATE_XML YES)
set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
find_package(Doxygen REQUIRED dot)
doxygen_add_docs(h5pp-doxygen
        ${PROJECT_SOURCE_DIR}/include
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Documentation -- Doxygen XML")


find_package(Sphinx REQUIRED)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(SPHINX_BUILD ${CMAKE_CURRENT_SOURCE_DIR}/sphinx)

add_custom_target(h5pp-sphinx
        COMMAND
        ${SPHINX_EXECUTABLE} -b html
        -Dbreathe_projects.DMRG++=${CMAKE_CURRENT_SOURCE_DIR}/xml # Tell Breathe where to find the Doxygen XML output
        ${SPHINX_SOURCE} ${SPHINX_BUILD}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/source
        COMMENT "Documentation -- Sphinx")

add_custom_target(h5pp-docs
        COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG> --target h5pp-doxygen h5pp-sphinx
        COMMENT "Generating documentation Doxygen+Sphinx")

