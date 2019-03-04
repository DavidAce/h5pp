

include(cmake/FindPackageHDF5.cmake)
#find_file(HDF5_C_COMPILER_EXECUTABLE        NAMES h5cc  HINTS /usr/bin /usr/local/bin )
#find_file(HDF5_CXX_COMPILER_EXECUTABLE      NAMES h5c++ HINTS /usr/bin /usr/local/bin )
#if(BUILD_SHARED_LIBS)
#    set(HDF5_TARGET_SUFFIX "shared")
#    set(HDF5_LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
#    set(HDF5_USE_STATIC_LIBRARIES OFF)
#else()
#    set(HDF5_TARGET_SUFFIX "static")
#    set(HDF5_LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
#    set(HDF5_USE_STATIC_LIBRARIES TRUE)
#endif()
#
#set(HDF5_FIND_DEBUG OFF)
#set(HDF5_ROOT ${INSTALL_DIRECTORY_THIRD_PARTY}/hdf5)
#find_package(HDF5 1.10 COMPONENTS C CXX HL)
##find_package(HDF5 1.10 COMPONENTS C CXX HL HINTS ${HDF5_DIR} ${HDF5_ROOT} ${INSTALL_DIRECTORY_THIRD_PARTY}/hdf5)
#
#if(HDF5_FOUND)
#    message(STATUS "HDF5 FOUND IN SYSTEM: ${HDF5_BUILD_DIR} ${HDF5_CXX_INCLUDE_DIRS}")
#    # Add convenience libraries to collect all the hdf5 libraries
#    add_library(hdf5    INTERFACE)
#    if(TARGET hdf5::hdf5-${HDF5_TARGET_SUFFIX})
#        target_link_libraries(hdf5
#                INTERFACE
#                hdf5::hdf5-${HDF5_TARGET_SUFFIX}
#                hdf5::hdf5_hl-${HDF5_TARGET_SUFFIX}
#                hdf5::hdf5_cpp-${HDF5_TARGET_SUFFIX}
#                hdf5::hdf5_hl_cpp-${HDF5_TARGET_SUFFIX}
#                )
#    else()
#        add_dependencies(hdf5  SZIP)
#        target_link_libraries(
#                hdf5
#                INTERFACE
#                ${HDF5_CXX_LIBRARY_hdf5}
#                ${HDF5_CXX_LIBRARY_hdf5_hl}
#                ${HDF5_CXX_LIBRARY_hdf5_cpp}
#                ${HDF5_CXX_LIBRARY_hdf5_hl_cpp}
#                ${HDF5_CXX_LIBRARY_iomp5} ${HDF5_CXX_LIBRARY_sz}
#                $<LINK_ONLY:-lpthread>
#                $<LINK_ONLY:-Wl,--no-as-needed -ldl -lm -lz -Wl,--as-needed>
#
#        )
#        target_include_directories(
#                hdf5
#                INTERFACE
#                ${HDF5_INCLUDE_DIR}
#        )
#
#    endif()
#    get_cmake_property(_variableNames VARIABLES)
#    foreach (_variableName ${_variableNames})
#        if("${_variableName}" MATCHES "HDF5" OR "${_variableName}" MATCHES "hdf5" OR "${_variableName}" MATCHES "h5")
#            message(STATUS "${_variableName}=${${_variableName}}")
#        endif()
#    endforeach()

if(TARGET hdf5)
        message(STATUS "HDF5 FOUND IN SYSTEM: ${HDF5_BUILD_DIR} ${HDF5_CXX_INCLUDE_DIRS}")
elseif (DOWNLOAD_HDF5 OR DOWNLOAD_ALL)
    message(STATUS "HDF5 will be installed into ${INSTALL_DIRECTORY_THIRD_PARTY}/hdf5 on first build.")

    include(ExternalProject)
    set(HDF5_IS_PARALLEL OFF)
    ExternalProject_Add(external_HDF5
            URL     https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.3/src/hdf5-1.10.3.tar.bz2 # version 1.10.2
            PREFIX      ${BUILD_DIRECTORY_THIRD_PARTY}/hdf5
            INSTALL_DIR ${INSTALL_DIRECTORY_THIRD_PARTY}/hdf5
            UPDATE_DISCONNECTED 1
            TEST_COMMAND ""
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
            -DCMAKE_ANSI_CFLAGS:STRING=-fPIC
            -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=OFF
            -DHDF5_ENABLE_PARALLEL=${HDF5_IS_PARALLEL}
            -DALLOW_UNSUPPORTED=ON
            -DBUILD_TESTING:BOOL=OFF
            -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=ON
            -DHDF5_BUILD_TOOLS:BOOL=ON
            -DHDF5_BUILD_EXAMPLES:BOOL=OFF
            -DHDF5_BUILD_FORTRAN:BOOL=OFF
            -DHDF5_BUILD_JAVA:BOOL=OFF
            -DCMAKE_INSTALL_MESSAGE=NEVER #Avoid unnecessary output to console
            -DCMAKE_C_FLAGS=-w

            )

    ExternalProject_Get_Property(external_HDF5 INSTALL_DIR)
    add_library(hdf5 INTERFACE)
#    add_library(hdf5::hdf5          ALIAS hdf5)
    add_dependencies(hdf5          external_HDF5)
    set(HDF5_ROOT             ${INSTALL_DIR})
    set(HDF5_DIR              ${INSTALL_DIR}/share/cmake/hdf5)

    #    if (HDF5_IS_PARALLEL)
    #        list(APPEND HDF5_LINKER_FLAGS ${MPI_LIBRARIES})
    #        list(APPEND HDF5_INCLUDE_DIR  ${MPI_INCLUDE_PATH})
    #    endif()


    target_link_libraries(hdf5
            INTERFACE
            ${INSTALL_DIR}/lib/libhdf5${HDF5_LIBRARY_SUFFIX}
            ${INSTALL_DIR}/lib/libhdf5_hl${HDF5_LIBRARY_SUFFIX}
            ${INSTALL_DIR}/lib/libhdf5_cpp${HDF5_LIBRARY_SUFFIX}
            ${INSTALL_DIR}/lib/libhdf5_hl_cpp${HDF5_LIBRARY_SUFFIX}
            $<LINK_ONLY:-Wl,--no-as-needed -ldl -lm -lz -Wl,--as-needed>
            $<LINK_ONLY:${PTHREAD_LIBRARY}>
            )
    target_include_directories(
            hdf5
            INTERFACE
            "$<BUILD_INTERFACE:${INSTALL_DIR}/include>"
            "$<INSTALL_INTERFACE:third-party/hdf5/include>"
    )

else()
    message("WARNING: Dependency HDF5 not found and DOWNLOAD_HDF5 is OFF. Build will fail.")
endif()


