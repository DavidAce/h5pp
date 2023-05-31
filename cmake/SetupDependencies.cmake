

# Fix issue with linking -pthreads or -lpthreads
if(NOT THREADS_PREFER_PTHREAD_FLAG)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
endif()
find_package(Threads)


# h5pp requires the filesystem header (and possibly stdc++fs library)
find_package(Filesystem COMPONENTS Final Experimental REQUIRED)
target_link_libraries(deps INTERFACE std::filesystem)


# Detect MPI if requested (we expect the HDF5 library to also be built with support for MPI)
if (H5PP_ENABLE_MPI AND NOT WIN32)
    find_package(MPI COMPONENTS C CXX REQUIRED)
    target_link_libraries(deps INTERFACE MPI::MPI_CXX MPI::MPI_C)
endif ()

# Detect quadmath/__float128 if requested
if(H5PP_USE_QUADMATH)
    find_package(quadmath REQUIRED)
    target_link_libraries(flags INTERFACE quadmath)
    target_compile_definitions(flags INTERFACE H5PP_USE_QUADMATH)
    target_compile_definitions(flags INTERFACE H5PP_USE_FLOAT128)
elseif(H5PP_USE_FLOAT128)
    include(CheckTypeSize)
    check_type_size(__float128 H5PP_FLOAT128_EXISTS BUILTIN_TYPES_ONLY LANGUAGE CXX)
    mark_as_advanced(H5PP_FLOAT128_EXISTS)
    if(H5PP_FLOAT128_EXISTS)
        target_compile_definitions(flags INTERFACE H5PP_USE_FLOAT128)
    else()
        message(FATAL_ERROR "CMake option H5PP_USE_FLOAT128 is ON, but type __float128 could not be detected")
    endif()
endif()


# Start finding the dependencies
if(H5PP_ENABLE_EIGEN3)
    find_package(Eigen3 3.3 REQUIRED)
    target_link_libraries(deps INTERFACE Eigen3::Eigen)
    target_compile_definitions(deps INTERFACE H5PP_USE_EIGEN3)
endif()
if(H5PP_ENABLE_FMT)
    find_package(fmt 6.1.2 REQUIRED)
    target_link_libraries(deps INTERFACE fmt::fmt)
    target_compile_definitions(deps INTERFACE H5PP_USE_FMT)
endif()
if(H5PP_ENABLE_SPDLOG)
    find_package(spdlog 1.5.0 REQUIRED)
    target_link_libraries(deps INTERFACE spdlog::spdlog)
    target_compile_definitions(deps INTERFACE H5PP_USE_SPDLOG)
endif()

# Note that the call below defaults to FindHDF5.cmake bundled with h5pp,
# because cmake/modules has been added to CMAKE_MODULE_PATH in cmake/SetupPaths.cmake
# Also, we don't impose any version requirement here: h5pp is compatible with 1.8 to 1.14.
find_package(ZLIB QUIET)
find_package(SZIP QUIET)
find_package(HDF5 COMPONENTS C HL REQUIRED)
include(cmake/HDF5TargetUtils.cmake)
h5pp_get_modern_hdf5_target_name()
target_link_libraries(deps INTERFACE HDF5::HDF5)

