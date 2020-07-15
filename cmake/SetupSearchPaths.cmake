cmake_minimum_required(VERSION 3.12)

# Append search paths for find_package and find_library calls
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX}) # Works like HINTS but can be ignored by NO_DEFAULT_PATH NO_CMAKE_PATH and NO_CMAKE_ENVIRONMENT_PATH

list(APPEND CMAKE_PREFIX_PATH
        ${H5PP_PREFIX_PATH}
        ${EIGEN3_PREFIX_PATH}
        ${SPDLOG_PREFIX_PATH}
        ${HDF5_PREFIX_PATH}
        ${H5PP_DIRECTORY_HINTS}
        ${SPDLOG_DIRECTORY_HINTS}
        ${EIGEN3_DIRECTORY_HINTS}
        ${HDF5_DIRECTORY_HINTS}
        )

if(H5PP_PREFER_CONDA_LIBS)
    list(APPEND CONDA_CANDIDATE_PATHS
            $ENV{CONDA_PREFIX}
            $ENV{HOME}/anaconda3
            $ENV{HOME}/anaconda
            $ENV{HOME}/miniconda3
            $ENV{HOME}/miniconda
            $ENV{HOME}/.conda
            $ENV{HOME}/anaconda3/envs/dmrg
            )
    foreach(path ${CONDA_CANDIDATE_PATHS})
        if(EXISTS ${path})
            list(APPEND CMAKE_PREFIX_PATH ${path})
        endif()
    endforeach()
    unset(CONDA_CANDIDATE_PATHS)
endif()

list(APPEND CMAKE_PREFIX_PATH
        $ENV{EBROOTHDF5}
        $ENV{EBROOTSPDLOG}
        $ENV{EBROOTEIGEN}
        ${fmt_ROOT}
        ${spdlog_ROOT}
        ${HDF5_ROOT}
        $ENV{HDF5_ROOT}
        ${Eigen3_ROOT}
        )

list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
list(APPEND H5PP_SEARCH_PATHS ${CMAKE_PREFIX_PATH})