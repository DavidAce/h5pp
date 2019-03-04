cmake_minimum_required(VERSION 3.10)
project(h5pp  VERSION 1.0.0)


set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)




################################################################
### Set options for compilation                              ###
################################################################
option(BUILD_SHARED_LIBS  "Builds shared libraries (libh5pp.so)"  OFF)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "..." FORCE)
endif()


###################
### Setup flags ###
###################
message("Compiling with: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if(NOT MARCH)
    set(MARCH "native")
endif()
if (${MARCH} STREQUAL "zen")
    string(TOLOWER znver1 CXX_MARCH)
else()
    string(TOLOWER ${MARCH} CXX_MARCH)
endif()


message("Using microarchitechture: ${CXX_MARCH}")
set(COMMON_OPTIONS -g -Wall -march=${CXX_MARCH} -mtune=${CXX_MARCH})
set(DEBUG_OPTIONS       -Wpedantic -Wextra -O0)
set(RELEASE_OPTIONS     -DNDEBUG -O3)

##########################
### OS-dependent flags ###
##########################
if(${CMAKE_HOST_APPLE})
    list(APPEND COMMON_OPTIONS -mmacosx-version-min=10.13)
else()
    #    list(APPEND COMMON_OPTIONS -mavx ) #-mfma makes things slower?
endif()


################################
### Compiler-dependent flags ###
################################
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" )
    list(APPEND COMMON_OPTIONS -Wno-implicit-fallthrough -Wno-deprecated-declarations -Wno-ignored-attributes -Wno-int-in-bool-context -Wno-maybe-uninitialized -Wno-enum-compare)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    list(APPEND COMMON_OPTIONS -stdlib=libstdc++ -Wno-deprecated-declarations -Wno-ignored-attributes -Wno-invalid-partial-specialization -Wno-missing-braces -Wno-overloaded-virtual -Wno-uninitialized)
endif()


################################
### Print current CXX flags  ###
################################
message(STATUS "COMMON CXX flags:  ${COMMON_OPTIONS} ${CMAKE_CXX_FLAGS}")
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "RELEASE CXX flags: ${RELEASE_OPTIONS}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "DEBUG CXX flags: ${DEBUG_OPTIONS}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
    message(STATUS "PROFILE CXX flags: ${PROFILE_OPTIONS}")
endif()





###########################################
###  Add source files                   ###
###########################################
add_library(${PROJECT_NAME}
        source/include/h5pp/h5pp.h
        source/h5ppCpp/h5ppFile.cpp
        source/include/h5pp/details/h5ppFile.h
        source/include/h5pp/details/nmspc_tensor_extra.h
        source/include/h5pp/details/h5ppTypeCheck.h source/include/h5pp/details/h5ppTypeComplex.h)
add_library(h5pp::h5pp ALIAS h5pp)

target_include_directories(
        ${PROJECT_NAME}
        PRIVATE
        source source/include
        INTERFACE
        $<INSTALL_INTERFACE:include>

)







###########################################
###  Apply RELEASE/DEBUG compile flags  ###
###########################################
target_compile_options (${PROJECT_NAME} PRIVATE ${COMMON_OPTIONS})                                   ### Common options
target_compile_options (${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")               ### Debug build options
target_compile_options (${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")           ### Release build options
target_compile_options (${PROJECT_NAME} PRIVATE "$<$<CONFIG:PROFILE>:${PROFILE_OPTIONS}>")           ### Profiling build options

target_link_libraries  (${PROJECT_NAME} PRIVATE -Wl,--no-allow-shlib-undefined )
target_link_libraries  (${PROJECT_NAME} PRIVATE -Wl,--no-undefined )
target_link_libraries  (${PROJECT_NAME} PRIVATE -flto)                                               ### Link time optimization flag
target_link_libraries  (${PROJECT_NAME} PUBLIC -lstdc++fs)                                           ### For experimental::filesystem

###############################
###  Require c++17 support  ###
###############################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
set_target_properties(${PROJECT_NAME}
        PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)


######################################
###  Require experimental headers  ###
######################################
# Use cmake_policy CMP0067 to let try_compile honor current CMAKE_CXX_STANDARD setting.
# See here
# https://stackoverflow.com/questions/47213356/cmake-using-corrext-c-standard-when-checking-for-header-files
cmake_policy(SET CMP0067 NEW)
include(CheckIncludeFileCXX)
set(CMAKE_REQUIRED_FLAGS -std=c++17  -lstdc++fs -l${PATH_TO_LIBSTDCXXFS})
check_include_file_cxx(experimental/filesystem    has_experimental_filesystem)
check_include_file_cxx(experimental/type_traits   has_experimental_type_traits)
if(NOT has_experimental_filesystem OR NOT has_experimental_type_traits )
    message(FATAL_ERROR "\n\
        Missing one or more experimental headers.\n\
        Consider using a newer compiler (GCC 7 or above, Clang 6 or above),\n\
        or checking the compiler flags.")
endif()




###########################################
###  Find or fetch all dependencies   ###
###########################################
if (BUILD_SHARED_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES  ${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_FIND_LIBRARY_SUFFIXES})
else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES  ${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()







# Require packages before building
find_package(Eigen3 3.3.4 HINTS ${Eigen3_DIR} ${EIGEN_INCLUDE_DIR} REQUIRED)
find_package(spdlog HINTS ${spdlog_DIR} ${spdlog_ROOT} NO_DEFAULT_PATH REQUIRED)
#find_package(HDF5 COMPONENTS C HL HINTS ${HDF5_DIR} ${HDF5_ROOT} REQUIRED)
#find_package(HDF5 1.10 COMPONENTS C CXX HL REQUIRED)
include(cmake/FindPackageHDF5.cmake)


target_link_libraries(${PROJECT_NAME} PUBLIC hdf5)
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)

get_target_property(EIGEN_INFO  Eigen3::Eigen           INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(SPDLOG_INFO spdlog::spdlog          INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(HDF5_INFO   hdf5                    INTERFACE_LINK_LIBRARIES)


message("=============================================================================")
message("=== SUMMARY OF CMAKE CONFIGURATION FOR TARGET H5PP:                       ===")
message("=============================================================================")
message("    EIGEN3   :   ${EIGEN_INFO}")
message("    SPDLOG   :   ${SPDLOG_INFO}")
message("    HDF5     :   ${HDF5_INFO}")
message("=============================================================================")
message("")


#
########################################################
#### Install library                                 ###
#### Default path is ${PROJECT_SOURCE_DIR}/install   ###
#### where ${PROJECT_SOURCE_DIR}  is the path of     ###
#### this CMakeLists.txt                             ###
########################################################
#

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        EXPORT        h5ppTargets
        RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )

install(DIRECTORY source/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

