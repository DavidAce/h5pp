# Append search paths for find_package and find_library calls
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX}) # Works like HINTS but can be ignored by NO_DEFAULT_PATH NO_CMAKE_PATH and NO_CMAKE_ENVIRONMENT_PATH

list(APPEND CMAKE_PREFIX_PATH
        ${EIGEN3_DIRECTORY_HINTS}
        ${SPDLOG_DIRECTORY_HINTS}
        ${HDF5_DIRECTORY_HINTS}
        )

if(H5PP_PREFER_CONDA_LIBS)
    list(APPEND CMAKE_PREFIX_PATH
            $ENV{CONDA_PREFIX}
            $ENV{HOME}/anaconda3
            $ENV{HOME}/anaconda
            $ENV{HOME}/miniconda3
            $ENV{HOME}/miniconda
            $ENV{HOME}/.conda
            )
endif()
list(APPEND CMAKE_PREFIX_PATH
        $ENV{EBROOTHDF5}
        $ENV{EBROOTSPDLOG}
        $ENV{EBROOTEIGEN}
        ${HDF5_ROOT}
        $ENV{HDF5_ROOT}
        )