# Append search paths for find_package and find_library calls
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)



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

# Append candidate paths to <PackageName>_ROOT variables so that find_package(...) calls can search these paths
list(APPEND Eigen3_ROOT $ENV{Eigen_ROOT} $ENV{Eigen3_ROOT} $ENV{EBROOTEIGEN} )
list(APPEND spdlog_ROOT $ENV{spdlog_ROOT} $ENV{EBROOTSPDLOG})
list(APPEND fmt_ROOT $ENV{fmt_ROOT} $ENV{EBROOTFMT})
list(APPEND HDF5_ROOT $ENV{HDF5_ROOT} $ENV{EBROOTHDF5})

mark_as_advanced(Eigen3_ROOT)
mark_as_advanced(spdlog_ROOT)
mark_as_advanced(fmt_ROOT)
mark_as_advanced(HDF5_ROOT)
mark_as_advanced(H5PP_CONDA_CANDIDATE_PATHS)
mark_as_advanced(H5PP_CONAN_CANDIDATE_PATHS)
mark_as_advanced(H5PP_DEPS_BUILD_DIR)
mark_as_advanced(H5PP_DEPS_INSTALL_DIR)