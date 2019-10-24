
if(NOT TARGET hdf5::hdf5)
    enable_language(C)
    include(cmake-modules/FindPackageHDF5.cmake)

    if(HDF5_FOUND AND TARGET hdf5)
        message(STATUS "HDF5 FOUND IN SYSTEM: ${HDF5_BUILD_DIR} ${HDF5_CXX_INCLUDE_DIRS} ${HDF5_hdf5_LIBRARY}")
        return()
    elseif (DOWNLOAD_MISSING)
        message(STATUS "HDF5 will be installed into ${INSTALL_DIRECTORY}/hdf5 on first build.")
        include(ExternalProject)
        set(HDF5_IS_PARALLEL OFF)
        ExternalProject_Add(external_HDF5
                URL     https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.3/src/hdf5-1.10.3.tar.bz2
                PREFIX      ${BUILD_DIRECTORY}/hdf5
                INSTALL_DIR ${INSTALL_DIRECTORY}/hdf5
                UPDATE_DISCONNECTED 1
                TEST_COMMAND ""
                CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
                -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_ANSI_CFLAGS:STRING=-fPIC
                -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
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
        add_library(hdf5::hdf5 ALIAS hdf5)
        add_dependencies(hdf5          external_HDF5)
        set(HDF5_ROOT             ${INSTALL_DIR})
        set(HDF5_DIR              ${INSTALL_DIR}/share/cmake/hdf5)

        #    if (HDF5_IS_PARALLEL)
        #        list(APPEND HDF5_LINKER_FLAGS ${MPI_LIBRARIES})
        #        list(APPEND HDF5_INCLUDE_DIR  ${MPI_INCLUDE_PATH})
        #    endif()
        target_link_libraries(hdf5
                INTERFACE
                ${INSTALL_DIR}/lib/libhdf5_hl${HDF5_LIBRARY_SUFFIX}
                ${INSTALL_DIR}/lib/libhdf5${HDF5_LIBRARY_SUFFIX}
                $<LINK_ONLY:-ldl -lm -lz>
                Threads::Threads
                )
        target_include_directories(
                hdf5
                SYSTEM
                INTERFACE
                "$<BUILD_INTERFACE:${INSTALL_DIR}/include>"
        )

    else()
        message("WARNING: Dependency HDF5 not found and DOWNLOAD_MISSING is OFF. Build will fail.")
    endif()

endif()
