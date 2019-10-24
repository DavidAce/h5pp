find_package(Eigen3 3.3.4  PATHS ${DIRECTORY_HINTS} PATH_SUFFIXES Eigen3 eigen3 include/Eigen3 include/eigen3  NO_CMAKE_PACKAGE_REGISTRY)

if(EIGEN3_FOUND AND TARGET Eigen3::Eigen)
    message(STATUS "Eigen3 found in system: ${EIGEN3_INCLUDE_DIR}")
#    include(cmake-modules/PrintTargetProperties.cmake)
#    print_target_properties(Eigen3::Eigen)

elseif (DOWNLOAD_MISSING)
    message(STATUS "Eigen3 will be installed into ${INSTALL_DIRECTORY_THIRD_PARTY}/Eigen3")
    include(cmake-modules/BuildThirdParty.cmake)
    build_third_party(
            "eigen3"
            "${H5PP_CONFIG_DIR_THIRD_PARTY}"
            "${H5PP_BUILD_DIR_THIRD_PARTY}"
            "${H5PP_INSTALL_DIR_THIRD_PARTY}"
            "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    )

    find_package(Eigen3 3.3.4  PATHS ${H5PP_INSTALL_DIR_THIRD_PARTY}/Eigen3 NO_DEFAULT_PATH)
    if(EIGEN3_FOUND)
        message(STATUS "Eigen3 installed successfully: ${EIGEN3_INCLUDE_DIR}")
    else()
        message(STATUS "cfg_result: ${cfg_result}")
        message(STATUS "bld_result: ${bld_result}")
        message(FATAL_ERROR "Eigen3 could not be downloaded.")
    endif()

else()
    message(STATUS "Dependency Eigen3 not found and DOWNLOAD_MISSING is OFF")
endif()


if(TARGET Eigen3::Eigen AND TARGET blas )
    set(EIGEN3_COMPILER_FLAGS  -Wno-parentheses) # -Wno-parentheses
    set(EIGEN3_USING_BLAS ON)
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" )
        list(APPEND EIGEN3_COMPILER_FLAGS -Wno-unused-but-set-variable)
    endif()
    if(TARGET mkl)
        list(APPEND EIGEN3_COMPILER_FLAGS -DEIGEN_USE_MKL_ALL)
        list(APPEND EIGEN3_COMPILER_FLAGS -DEIGEN_USE_LAPACKE_STRICT)
        list(APPEND EIGEN3_INCLUDE_DIR ${MKL_INCLUDE_DIR})
        message(STATUS "Eigen3 will use MKL")
    else ()
        list(APPEND EIGEN3_COMPILER_FLAGS -DEIGEN_USE_BLAS)
        list(APPEND EIGEN3_COMPILER_FLAGS -DEIGEN_USE_LAPACKE_STRICT)
        message(STATUS "Eigen3 will use BLAS and LAPACKE")
    endif()
    target_compile_options    (Eigen3::Eigen INTERFACE ${EIGEN3_COMPILER_FLAGS})
endif()

