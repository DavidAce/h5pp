cmake_minimum_required(VERSION 3.19)

# Append search paths for find_package and find_library calls
list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)

# Make sure find_library prefers static/shared library depending on BUILD_SHARED_LIBS
# This is important when finding dependencies such as zlib which provides both shared and static libraries.
if(BUILD_SHARED_LIBS AND NOT DEFINED CMAKE_FIND_LIBRARY_SUFFIXES)
    # This is order is the default
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_SHARED_LIBRARY_SUFFIX};${CMAKE_STATIC_LIBRARY_SUFFIX} CACHE STRING "Prefer finding shared libraries")
else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX};${CMAKE_SHARED_LIBRARY_SUFFIX} CACHE STRING "Prefer finding static libraries")
endif()


# Setup build and install directories for dependencies
if(NOT H5PP_DEPS_BUILD_DIR)
    set(H5PP_DEPS_BUILD_DIR ${CMAKE_BINARY_DIR}/h5pp-deps-build)
endif()
if(NOT H5PP_DEPS_INSTALL_DIR)
    set(H5PP_DEPS_INSTALL_DIR ${CMAKE_BINARY_DIR}/h5pp-deps-install)
endif()


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

mark_as_advanced(H5PP_CONDA_CANDIDATE_PATHS)
mark_as_advanced(H5PP_CONAN_CANDIDATE_PATHS)
mark_as_advanced(H5PP_DEPS_BUILD_DIR)
mark_as_advanced(H5PP_DEPS_INSTALL_DIR)