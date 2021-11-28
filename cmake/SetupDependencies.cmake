#Try to find or get all dependencies
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