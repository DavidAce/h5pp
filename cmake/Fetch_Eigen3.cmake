
#find_package(Eigen3 3.3.4 HINTS ${INSTALL_DIRECTORY_THIRD_PARTY}/Eigen3)
find_package(Eigen3 3.3.4  PATHS ${H5PP_INSTALL_DIR_THIRD_PARTY}/Eigen3 NO_DEFAULT_PATH)
find_package(Eigen3 3.3.4  PATHS ${H5PP_INSTALL_DIR_THIRD_PARTY}/Eigen3 NO_CMAKE_PACKAGE_REGISTRY)
find_package(Eigen3 3.3.4  PATHS ${H5PP_INSTALL_DIR_THIRD_PARTY}/Eigen3)

if(BLAS_LIBRARIES)
    set(EIGEN3_COMPILER_FLAGS  -Wno-parentheses) # -Wno-parentheses
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" )
        list(APPEND EIGEN3_COMPILER_FLAGS -Wno-unused-but-set-variable)
    endif()
    if(MKL_FOUND)
        list(APPEND EIGEN3_COMPILER_FLAGS -DEIGEN_USE_MKL_ALL)
        list(APPEND EIGEN3_COMPILER_FLAGS -DEIGEN_USE_LAPACKE_STRICT)
        list(APPEND EIGEN3_INCLUDE_DIR ${MKL_INCLUDE_DIR})
        message(STATUS "Eigen3 will use MKL")
    elseif (BLAS_FOUND)
        list(APPEND EIGEN3_COMPILER_FLAGS -DEIGEN_USE_BLAS)
        list(APPEND EIGEN3_COMPILER_FLAGS -DEIGEN_USE_LAPACKE)
        message(STATUS "Eigen3 will use BLAS and LAPACKE")
    endif()
endif()


if(EIGEN3_FOUND)
    message(STATUS "EIGEN FOUND IN SYSTEM: ${EIGEN3_INCLUDE_DIR}")
    add_library(Eigen3 INTERFACE)
    get_target_property(EIGEN3_INCLUDE_DIR Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
#    target_link_libraries(Eigen3 INTERFACE Eigen3::Eigen)
    target_include_directories(Eigen3 INTERFACE ${EIGEN3_INCLUDE_DIR})
    target_compile_options(Eigen3::Eigen INTERFACE ${EIGEN3_COMPILER_FLAGS})
elseif (DOWNLOAD_EIGEN3 OR DOWNLOAD_ALL)
    message(STATUS "Eigen3 will be installed into ${INSTALL_DIRECTORY_THIRD_PARTY}/Eigen3 on first build.")

    include(ExternalProject)
    ExternalProject_Add(external_EIGEN3
            GIT_REPOSITORY https://github.com/eigenteam/eigen-git-mirror.git
            GIT_TAG 3.3.7
            GIT_PROGRESS 1
            PREFIX      ${H5PP_BUILD_DIR_THIRD_PARTY}/Eigen3
            INSTALL_DIR ${H5PP_INSTALL_DIR_THIRD_PARTY}/Eigen3
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
            UPDATE_COMMAND ""
            TEST_COMMAND ""
#            INSTALL_COMMAND ""
#            CONFIGURE_COMMAND ""
        )


    ExternalProject_Get_Property(external_EIGEN3 INSTALL_DIR)
    add_library(Eigen3 INTERFACE)
    add_library(Eigen3::Eigen ALIAS Eigen3)
    set(EIGEN3_ROOT_DIR ${INSTALL_DIR})
    set(EIGEN3_INCLUDE_DIR ${INSTALL_DIR}/include/eigen3)
    set(Eigen3_DIR ${INSTALL_DIR}/share/eigen3/cmake)
    add_dependencies(Eigen3 external_EIGEN3)
    target_compile_options(Eigen3 INTERFACE ${EIGEN3_COMPILER_FLAGS})
    target_include_directories(
            Eigen3
            INTERFACE
            $<BUILD_INTERFACE:${INSTALL_DIR}/include/eigen3>
            $<INSTALL_INTERFACE:third-party/Eigen3/include/eigen3>
    )
else()
    message(STATUS "Dependency Eigen3 not found and DOWNLOAD_EIGEN3 is OFF")
endif()



