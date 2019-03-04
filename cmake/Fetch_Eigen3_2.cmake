#find_package(Eigen3 3.3.4 HINTS ${INSTALL_DIRECTORY_THIRD_PARTY}/Eigen3)

if(Eigen3_FOUND)
    message(STATUS "EIGEN FOUND IN SYSTEM: ${EIGEN3_INCLUDE_DIR}")
else()
    message(STATUS "Eigen3 will be installed into ${INSTALL_DIRECTORY_THIRD_PARTY}/Eigen3 on first build.")
    include(FetchContent)
    FetchContent_Declare(
            Eigen3
            GIT_REPOSITORY https://github.com/eigenteam/eigen-git-mirror.git
            GIT_TAG 3.3.7
            GIT_PROGRESS 1
#            PREFIX      ${BUILD_DIRECTORY_THIRD_PARTY}/Eigen3
            INSTALL_DIR ${INSTALL_DIRECTORY_THIRD_PARTY}/Eigen3
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    )

    set(FETCHCONTENT_QUIET OFF)
    # Check if population has already been performed
    FetchContent_GetProperties(Eigen3)
    if(NOT eigen3_POPULATED)
        # Fetch the content using previously declared details
        FetchContent_Populate(Eigen3)
        # Set custom variables, policies, etc.
        # ...
        message("${eigen3_SOURCE_DIR} ${eigen3_BINARY_DIR}")
        # Bring the populated content into the build
        add_subdirectory(${eigen3_SOURCE_DIR} ${eigen3_BINARY_DIR})
    endif()
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        if("${_variableName}" MATCHES "Eigen" OR "${_variableName}" MATCHES "eigen" OR "${_variableName}" MATCHES "EIGEN")
            message(STATUS "${_variableName}=${${_variableName}}")
        endif()
    endforeach()
    find_package(Eigen3 3.3.4  )
    message("ARE TARGETS: ")
    if(TARGET Eigen3)
        message("   Eigen3")
    endif()
    if(TARGET eigen3)
        message("   eigen3")
    endif()
    if(TARGET Eigen3::Eigen)
        message("   Eigen3::Eigen")
    endif()
    if(TARGET Eigen3::eigen)
        message("   Eigen3::eigen")
    endif()
    if(TARGET Eigen)
        message("   Eigen")
    endif()
    if(TARGET eigen)
        message("   eigen")
    endif()
    add_library(Eigen3::Eigen ALIAS eigen)
endif()