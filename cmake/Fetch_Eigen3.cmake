
find_package(Eigen3 3.3.4)


if(TARGET Eigen3::Eigen)
#    message(STATUS "Eigen3 found")

elseif(TARGET Eigen3)
    add_library(Eigen3::Eigen ALIAS Eigen3)
elseif ("${DOWNLOAD_METHOD}" MATCHES "native")
    message(STATUS "Eigen3 will be installed into ${CMAKE_INSTALL_PREFIX}")
    include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
    build_dependency(Eigen3 "${eigen3_install_prefix}" "" "")
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
    message(STATUS "Dependency Eigen3 not found in your system. Set DOWNLOAD_METHOD to one of 'conan|native'")
endif()



if(TARGET Eigen3::Eigen AND TARGET blas )
    set(EIGEN3_USING_BLAS ON)
    if(TARGET mkl)
        message(STATUS "Eigen3 will use MKL")
        target_compile_definitions    (Eigen3::Eigen INTERFACE -DEIGEN_USE_MKL_ALL)
        target_compile_definitions    (Eigen3::Eigen INTERFACE -DEIGEN_USE_LAPACKE_STRICT)
        target_link_libraries         (Eigen3::Eigen INTERFACE mkl)
    else ()
        message(STATUS "Eigen3 will use BLAS and LAPACKE")
        target_compile_definitions    (Eigen3::Eigen INTERFACE -DEIGEN_USE_BLAS)
        target_compile_definitions    (Eigen3::Eigen INTERFACE -DEIGEN_USE_LAPACKE_STRICT)
        target_link_libraries         (Eigen3::Eigen INTERFACE blas)
    endif()

    # Use this flag if Ceres is giving you trouble!
    # For some reason it starts mixing alligned and hand-made aligned malloc and freeing them willy nilly
    # This flag forces its hand and avoids a segfault in some cases.
    # target_compile_definitions(Eigen3::Eigen INTERFACE -DEIGEN_MALLOC_ALREADY_ALIGNED=0) # Finally something works!!!


endif()
