cmake_minimum_required(VERSION 3.12)

# Append search paths for find_package and find_library calls
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)


# Paths to search for conda libraries
# These paths should only be searched when H5PP_PREFER_CONDA_LIBS = ON
if(H5PP_PREFER_CONDA_LIBS)
    list(APPEND H5PP_CONDA_CANDIDATE_PATHS
            ${CONDA_PREFIX}
            $ENV{CONDA_PREFIX}
            $ENV{HOME}/anaconda3
            $ENV{HOME}/anaconda
            $ENV{HOME}/miniconda3
            $ENV{HOME}/miniconda
            $ENV{HOME}/.conda
            )
    list(APPEND Eigen3_ROOT ${H5PP_CONDA_CANDIDATE_PATHS})
    list(APPEND spdlog_ROOT ${H5PP_CONDA_CANDIDATE_PATHS})
    list(APPEND fmt_ROOT ${H5PP_CONDA_CANDIDATE_PATHS})
    list(APPEND HDF5_ROOT ${H5PP_CONDA_CANDIDATE_PATHS})
endif()

# Paths to search for conan installation.
list(APPEND H5PP_CONAN_CANDIDATE_PATHS
        ${CONAN_PREFIX}
        $ENV{CONAN_PREFIX}
        ${CONDA_PREFIX}
        $ENV{CONDA_PREFIX}
        $ENV{HOME}/anaconda3
        $ENV{HOME}/anaconda
        $ENV{HOME}/miniconda3
        $ENV{HOME}/miniconda
        $ENV{HOME}/.conda
        )

# Append candidate paths to <PackageName>_ROOT variables so that find_package(...) calls can search these paths
# Note that user-specified paths are searched first.
list(APPEND Eigen3_ROOT
        ${EIGEN3_PREFIX_PATH}
        ${EIGEN3_DIRECTORY_HINTS}
        $ENV{EBROOTEIGEN}
        ${CMAKE_INSTALL_PREFIX}/Eigen3
        ${CMAKE_INSTALL_PREFIX}
        )

list(APPEND spdlog_ROOT
        ${SPDLOG_PREFIX_PATH}
        ${SPDLOG_DIRECTORY_HINTS}
        $ENV{EBROOTSPDLOG}
        ${CMAKE_INSTALL_PREFIX}/spdlog
        ${CMAKE_INSTALL_PREFIX}
        )

list(APPEND fmt_ROOT
        ${FMT_PREFIX_PATH}
        ${FMT_DIRECTORY_HINTS}
        $ENV{EBROOTFMT}
        ${CMAKE_INSTALL_PREFIX}/fmt
        ${CMAKE_INSTALL_PREFIX}
        )

list(APPEND HDF5_ROOT
        ${HDF5_PREFIX_PATH}
        ${HDF5_DIRECTORY_HINTS}
        $ENV{EBROOTHDF5}
        $ENV{HDF5_ROOT}
        ${CMAKE_INSTALL_PREFIX}/hdf5
        ${CMAKE_INSTALL_PREFIX}
        )

mark_as_advanced(Eigen3_ROOT)
mark_as_advanced(spdlog_ROOT)
mark_as_advanced(fmt_ROOT)
mark_as_advanced(HDF5_ROOT)
mark_as_advanced(H5PP_CONDA_CANDIDATE_PATHS)
mark_as_advanced(H5PP_CONAN_CANDIDATE_PATHS)


