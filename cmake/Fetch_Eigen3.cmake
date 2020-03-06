
find_package(Eigen3 3.3.4)


if(TARGET Eigen3::Eigen)
#    message(STATUS "Eigen3 found")

elseif(TARGET Eigen3)
    add_library(Eigen3::Eigen ALIAS Eigen3)
elseif ("${H5PP_DOWNLOAD_METHOD}" MATCHES "native")
    message(STATUS "Eigen3 will be installed into ${CMAKE_INSTALL_PREFIX}")
    include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
    list(APPEND H5PP_EIGEN3_OPTIONS  "")
    build_dependency(Eigen3 "${eigen3_install_prefix}" "${H5PP_EIGEN3_OPTIONS}")
    find_package(Eigen3 3.3.7
            HINTS ${eigen3_install_prefix}
            PATH_SUFFIXES Eigen3 eigen3 include/Eigen3 include/eigen3  NO_DEFAULT_PATH)
    if(TARGET Eigen3::Eigen)
        message(STATUS "Eigen3 installed successfully")
        target_link_libraries(Eigen3::Eigen INTERFACE -lrt)
    else()
        message(FATAL_ERROR "Eigen3 could not be downloaded.")
    endif()

else()
    message(STATUS "Dependency Eigen3 not found in your system. Set H5PP_DOWNLOAD_METHOD to one of 'conan|native'")
endif()
