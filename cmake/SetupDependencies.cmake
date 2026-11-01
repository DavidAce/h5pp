

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

# Select one 128-bit floating-point implementation. Quadmath works with C++20, but
# std::float128_t needs C++23 and standard-library support.
if(H5PP_USE_QUADMATH AND H5PP_USE_FLOAT128)
    message(FATAL_ERROR "Choose only one of H5PP_USE_QUADMATH and H5PP_USE_FLOAT128")
endif()
if(H5PP_USE_QUADMATH)
    find_package(quadmath REQUIRED)
    target_link_libraries(flags INTERFACE quadmath)
    target_compile_definitions(flags INTERFACE H5PP_USE_QUADMATH)
elseif(H5PP_USE_FLOAT128)
    include(cmake/CheckStdFloat128.cmake)
    CheckStdFloat128()
    target_compile_features(flags INTERFACE cxx_std_23)
    target_compile_definitions(flags INTERFACE H5PP_USE_FLOAT128)
endif()

# Start finding the dependencies
if(H5PP_ENABLE_EIGEN3)
    find_package(Eigen3 5 QUIET)
    if(NOT Eigen3_FOUND)
        find_package(Eigen3 3.4 REQUIRED)
    endif()
    target_link_libraries(deps INTERFACE Eigen3::Eigen)
    target_compile_definitions(deps INTERFACE H5PP_USE_EIGEN3)
endif()
if(H5PP_ENABLE_FMT)
    find_package(fmt 9.1.0 REQUIRED)
    if(FMT_VERSION AND FMT_VERSION VERSION_LESS 9.1.0)
        message(FATAL_ERROR "H5PP_ENABLE_FMT: Found version ${FMT_VERSION} < 9.1.0 required by h5pp")
    endif()
    target_link_libraries(deps INTERFACE fmt::fmt)
    target_compile_definitions(deps INTERFACE H5PP_USE_FMT)
endif()
if(H5PP_ENABLE_SPDLOG)
    find_package(spdlog 1.12.0 REQUIRED)
    if(SPDLOG_VERSION AND SPDLOG_VERSION VERSION_LESS 1.12.0)
        message(FATAL_ERROR "H5PP_ENABLE_SPDLOG: Found version ${SPDLOG_VERSION} < 1.12.0 required by h5pp")
    endif()
    target_link_libraries(deps INTERFACE spdlog::spdlog)
    target_compile_definitions(deps INTERFACE H5PP_USE_SPDLOG)
endif()


# h5pp needs the C and high-level C APIs
find_package(HDF5 COMPONENTS C HL REQUIRED)
if(HDF5_VERSION AND HDF5_VERSION VERSION_LESS 1.8)
    message(FATAL_ERROR "Found version ${HDF5_VERSION}, but h5pp requires HDF5 version >= 1.8")
endif()
include(cmake/HDF5TargetUtils.cmake)
h5pp_get_modern_hdf5_target_name()
target_link_libraries(deps INTERFACE h5pp::hdf5)

# h5pp uses zlib directly for raw chunk compression/decompression.
find_package(ZLIB QUIET)
if(TARGET ZLIB::ZLIB)
    target_link_libraries(deps INTERFACE ZLIB::ZLIB)
endif()
