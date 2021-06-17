function(install_hdf5)
    include(cmake/InstallPackage.cmake)
    # Download HDF5 (and ZLIB and SZIP)
   if(NOT SZIP_FOUND)
       set(SZIP_ROOT ${H5PP_DEPS_INSTALL_DIR} CACHE PATH "Default root path for SZIP installed by h5pp")
       find_package(SZIP CONFIG NAMES szip sz COMPONENTS static shared
               PATH_SUFFIXES cmake share/cmake # Fixes bug in CMake 3.20.2 not generating search paths
               NO_SYSTEM_ENVIRONMENT_PATH
               NO_CMAKE_PACKAGE_REGISTRY
               NO_CMAKE_SYSTEM_PATH
               NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
       if(NOT SZIP_FOUND OR NOT TARGET szip-static)
           install_package(szip "${H5PP_DEPS_INSTALL_DIR}" "")
           find_package(SZIP CONFIG NAMES szip sz COMPONENTS static shared
                   PATH_SUFFIXES cmake share/cmake # Fixes bug in CMake 3.20.2 not generating search paths
                   NO_SYSTEM_ENVIRONMENT_PATH
                   NO_CMAKE_SYSTEM_PATH
                   NO_CMAKE_PACKAGE_REGISTRY
                   NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
                   REQUIRED)
       endif()
   endif()
   if(NOT ZLIB_FOUND)
       set(ZLIB_ROOT ${H5PP_DEPS_INSTALL_DIR} CACHE PATH "Default root path for ZLIB installed by h5pp")
       # Don't use the find_package module yet: it would just find system libz.a, which we are trying to avoid
       find_library(ZLIB_LIBRARY NAMES
               z zlib zdll zlib1 zlibstatic # Release names
               zd zlibd zdlld zlibd1 zlib1d zlibstaticd # Debug names
               HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib/lib NO_DEFAULT_PATH)
       if(NOT ZLIB_LIBRARY)
           install_package(zlib "${H5PP_DEPS_INSTALL_DIR}" "")
       endif()
       # HDF5 will call find_package(ZLIB). If it succeeds here it will probably succeed then.
       # This module searches ZLIB_ROOT first
       find_package(ZLIB REQUIRED)
   endif()

   if(NOT HDF5_FOUND OR NOT TARGET hdf5::all)
       set(HDF5_ROOT ${H5PP_DEPS_INSTALL_DIR} CACHE PATH "Default root path for HDF5 installed by h5pp")
       set(HDF5_NO_SYSTEM_ENVIRONMENT_PATH ON)
       set(HDF5_NO_CMAKE_PACKAGE_REGISTRY ON)
       set(HDF5_NO_CMAKE_SYSTEM_PATH ON)
       set(HDF5_NO_CMAKE_SYSTEM_PACKAGE_REGISTRY ON)
       set(HDF5_FIND_VERBOSE OFF)
       set(HDF5_FIND_DEBUG OFF)
       mark_as_advanced(HDF5_NO_SYSTEM_ENVIRONMENT_PATH)
       mark_as_advanced(HDF5_NO_CMAKE_PACKAGE_REGISTRY)
       mark_as_advanced(HDF5_NO_CMAKE_SYSTEM_PATH)
       mark_as_advanced(HDF5_NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
       mark_as_advanced(HDF5_FIND_VERBOSE)
       mark_as_advanced(HDF5_FIND_DEBUG)
       # This one uses our own module, but will call the config-mode internally first.
       find_package(HDF5 1.12 COMPONENTS C HL)
       if(NOT HDF5_FOUND OR NOT TARGET hdf5::all)
           message(STATUS "HDF5 will be installed into ${H5PP_DEPS_INSTALL_DIR}")
           list(APPEND H5PP_HDF5_OPTIONS  "-DHDF5_ENABLE_PARALLEL:BOOL=${H5PP_ENABLE_MPI}")
           list(APPEND H5PP_HDF5_OPTIONS  "-DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=ON")
           list(APPEND H5PP_HDF5_OPTIONS  "-DHDF5_ENABLE_SZIP_SUPPORT:BOOL=ON")
           install_package(hdf5 "${H5PP_DEPS_INSTALL_DIR}" "${H5PP_HDF5_OPTIONS}")
           # This one uses our own module, but will call the config-mode internally first.
           find_package(HDF5 1.12 COMPONENTS C HL REQUIRED)
           message(STATUS "hdf5 installed successfully")
       endif()
       if(HDF5_FOUND AND TARGET hdf5::all)
           list(APPEND H5PP_TARGETS hdf5::all)
           target_link_libraries(deps INTERFACE hdf5::all)
       else()
           message(FATAL_ERROR "HDF5 could not be downloaded and built from source")
       endif()
   endif()
endfunction()