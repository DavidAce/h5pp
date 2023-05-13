#Try to find or get all dependencies
if(NOT THREADS_PREFER_PTHREAD_FLAG)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
endif()
find_package(Threads)

if (H5PP_ENABLE_MPI AND NOT WIN32)
    find_package(MPI COMPONENTS C CXX REQUIRED)
    target_link_libraries(deps INTERFACE MPI::MPI_CXX MPI::MPI_C)
endif ()

include(cmake/SetupStdFilesystem.cmake)
include(cmake/SetupDependenciesFind.cmake)
include(cmake/SetupDependenciesFetch.cmake)
include(cmake/SetupDependenciesCMake.cmake)
include(cmake/SetupDependenciesCPM.cmake)
include(cmake/SetupDependenciesConan.cmake)

if(H5PP_ENABLE_SPDLOG)
    target_compile_definitions(deps INTERFACE H5PP_USE_SPDLOG)
endif()

if(H5PP_ENABLE_FMT)
    target_compile_definitions(deps INTERFACE H5PP_USE_FMT)
endif()

if(H5PP_ENABLE_EIGEN3)
    target_compile_definitions(deps INTERFACE H5PP_USE_EIGEN3)
endif()


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

