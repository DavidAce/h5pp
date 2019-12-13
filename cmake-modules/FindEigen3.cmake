
if(NOT Eigen3_FIND_VERSION)
    if(NOT Eigen3_FIND_VERSION_MAJOR)
        set(Eigen3_FIND_VERSION_MAJOR 2)
    endif()
    if(NOT Eigen3_FIND_VERSION_MINOR)
        set(Eigen3_FIND_VERSION_MINOR 91)
    endif()
    if(NOT Eigen3_FIND_VERSION_PATCH)
        set(Eigen3_FIND_VERSION_PATCH 0)
    endif()
    set(Eigen3_FIND_VERSION "${Eigen3_FIND_VERSION_MAJOR}.${Eigen3_FIND_VERSION_MINOR}.${Eigen3_FIND_VERSION_PATCH}")
endif()

macro(_eigen3_check_version)
    file(READ "${EIGEN3_INCLUDE_DIR}/Eigen/src/Core/util/Macros.h" _eigen3_version_header)
    string(REGEX MATCH "define[ \t]+EIGEN_WORLD_VERSION[ \t]+([0-9]+)" _eigen3_world_version_match "${_eigen3_version_header}")
    set(EIGEN3_WORLD_VERSION "${CMAKE_MATCH_1}")
    string(REGEX MATCH "define[ \t]+EIGEN_MAJOR_VERSION[ \t]+([0-9]+)" _eigen3_major_version_match "${_eigen3_version_header}")
    set(EIGEN3_MAJOR_VERSION "${CMAKE_MATCH_1}")
    string(REGEX MATCH "define[ \t]+EIGEN_MINOR_VERSION[ \t]+([0-9]+)" _eigen3_minor_version_match "${_eigen3_version_header}")
    set(EIGEN3_MINOR_VERSION "${CMAKE_MATCH_1}")

    set(EIGEN3_VERSION ${EIGEN3_WORLD_VERSION}.${EIGEN3_MAJOR_VERSION}.${EIGEN3_MINOR_VERSION})
    if(${EIGEN3_VERSION} VERSION_LESS ${Eigen3_FIND_VERSION})
        set(EIGEN3_VERSION_OK FALSE)
    else()
        set(EIGEN3_VERSION_OK TRUE)
    endif()
    if(NOT EIGEN3_VERSION_OK)
        message(STATUS "Eigen3 version ${EIGEN3_VERSION} found in ${EIGEN3_INCLUDE_DIR}, "
                "but at least version ${Eigen3_FIND_VERSION} is required")
    endif()
endmacro()


# First try finding a config file in a set of paths
# Note that the variables <package_name>_ROOT  <package_name>_DIR
# are special and should go first to allow overriding from environment
# If found correctly, this should define a target Eigen3::Eigen
find_package(Eigen3 ${Eigen3_FIND_VERSION}
        HINTS
        ${Eigen3_ROOT} $ENV{Eigen3_ROOT}
        ${EIGEN3_ROOT} $ENV{EIGEN3_ROOT}
        ${Eigen3_DIR}  $ENV{Eigen3_DIR}
        ${H5PP_DIRECTORY_HINTS}
        ${CMAKE_INSTALL_PREFIX}
        ${CMAKE_INSTALL_PREFIX}/include
        PATHS ${CMAKE_BINARY_DIR}/h5pp-deps-install  $ENV{EBROOTEIGEN} $ENV{CONDA_PREFIX}
        PATH_SUFFIXES Eigen3 eigen3 include/Eigen3 include/eigen3 Eigen3/include/eigen3
        CONFIG QUIET)


if (TARGET Eigen3::Eigen)
    _eigen3_check_version()
    get_target_property(EIGEN3_INCLUDE_DIR Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
    if(EIGEN3_VERSION_OK AND EIGEN3_INCLUDE_DIR)
        set(Eigen3_FOUND TRUE)
    endif()
endif()
if(NOT TARGET Eigen3::Eigen OR NOT EIGEN3_INCLUDE_DIR)
    # If no config was found, try finding Eigen in a similar way as the original FindEigen3.cmake does it
    # This way we can avoid supplying the original file and allow more flexibility for overriding
    find_path(EIGEN3_INCLUDE_DIR NAMES signature_of_eigen3_matrix_library
            HINTS
            $ENV{EIGEN3_ROOT}
            $ENV{EIGEN3_ROOT_DIR}
            ${Eigen3_ROOT} $ENV{Eigen3_ROOT}
            ${EIGEN3_ROOT} $ENV{EIGEN3_ROOT}
            ${Eigen3_DIR}  $ENV{Eigen3_DIR}
            ${H5PP_DIRECTORY_HINTS}
            ${CMAKE_INSTALL_PREFIX}
            ${CMAKE_INSTALL_PREFIX}/include
            PATHS
            ${KDE4_INCLUDE_DIR}
            PATH_SUFFIXES Eigen3 eigen3 include/Eigen3 include/eigen3 Eigen3/include/eigen3
            )
    if(EIGEN3_INCLUDE_DIR)
        _eigen3_check_version()
    endif()
    if(EIGEN3_VERSION_OK)
        add_library(Eigen3::Eigen INTERFACE)
        set(Eigen3_FOUND TRUE)
        set_target_properties(Eigen3::Eigen PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${EIGEN3_INCLUDE_DIR}")

    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Eigen3 DEFAULT_MSG Eigen3_FOUND EIGEN3_INCLUDE_DIR EIGEN3_VERSION_OK)
mark_as_advanced(EIGEN3_INCLUDE_DIR)
mark_as_advanced(Eigen3_FOUND)