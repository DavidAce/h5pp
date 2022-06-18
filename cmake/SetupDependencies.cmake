#Try to find or get all dependencies
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
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