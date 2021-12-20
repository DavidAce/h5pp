function(h5pp_make_hdf5_target_from_vars target_name)
    if(NOT TARGET ${target_name})
        add_library(${target_name} INTERFACE IMPORTED)
    endif()
    target_include_directories(${target_name} SYSTEM INTERFACE ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIR})
    target_compile_definitions(${target_name} INTERFACE ${HDF5_DEFINITIONS})
    # Link found libraries
    foreach(lib ${HDF5_C_HL_LIBRARIES} ${HDF5_HL_LIBRARIES} ${HDF5_C_LIBRARIES} ${HDF5_LIBRARIES})
        get_filename_component(name ${lib} NAME)
        if("${name}" MATCHES "hdf5")
            target_link_libraries(${target_name} INTERFACE ${lib})
        elseif(${name} MATCHES "pthread") # Link with -pthread instead of libpthread directly
            set(THREADS_PREFER_PTHREAD_FLAG TRUE)
            find_package(Threads REQUIRED)
            target_link_libraries(${target_name} INTERFACE Threads::Threads)
        elseif("${name}" MATCHES "libdl")
            # dl and m have to be linked with "-ldl" or "-lm", in particular on static builds.
            add_library(hdf5::dl INTERFACE IMPORTED)
            target_link_libraries(hdf5::dl INTERFACE dl)
            target_link_libraries(${target_name} INTERFACE hdf5::dl)
        elseif("${name}" MATCHES "libm")
            # dl and m have to be linked with "-ldl" or "-lm", in particular on static builds.
            add_library(hdf5::dl INTERFACE IMPORTED)
            target_link_libraries(hdf5::m INTERFACE m)
            target_link_libraries(${target_name} INTERFACE hdf5::m)
        else()
            target_link_libraries(${target_name} INTERFACE ${lib})
        endif()
    endforeach()
endfunction()


function(h5pp_get_modern_hdf5_target_name)
    # This function defines a standardized target "HDF5::HDF5" that is self-contained.
    # By self-contained we mean that the user can call
    #       target_link_libraries(<target> PRIVATE HDF5::HDF5)
    # and get everything working without further hassle.
    #
    # This is the motivation:
    # FindHDF5.cmake bundled with h5pp defines target HDF5::HDF5 with everything (self-contained)
    # Conan also defines a target HDF5::HDF5 with everything (self-contained)
    # CMake version >= 3.20 defines target HDF5::HDF5 with imp lib only (not self-contained)
    # CMake version >= 3.19 Earlier versions of CMake define target hdf5::hdf5_hl (not self-contained)
    # hdf5-config.cmake from source installations will define target hdf5_hl-<static|shared> (self-contained)
    if(NOT HDF5_FOUND)
        message(FATAL_ERROR "h5pp_get_modern_hdf5_target_name: this function should only be called after a successful call to find_package(HDF5)")
    endif()

    # If HDF5::ALIAS is a sentinel target from h5pp-bundled FindHDF5.cmake
    # so, if it is defined then we expect HDF5::HDF5 to be self-contained
    if(TARGET HDF5::HDF5 AND TARGET HDF5::ALIAS)
        message(VERBOSE "Found good HDF5::HDF5 target from bundled FindHDF5.cmake")
        return()
    endif()

    # If HDF5::HDF5 is defined without HDF5::ALIAS, then it can come from conan or cmake-bundled FindHDF5.cmake
    # We can test it by checking if its interface libraries match "CONAN"
    if(TARGET HDF5::HDF5)
        get_target_property(HDF5_LINK_LIBS HDF5::HDF5 INTERFACE_LINK_LIBRARIES)
        if(HDF5_LINK_LIBS MATCHES "CONAN")
            return()
        endif()
    endif()

    # We have no HDF5::HDF5 target at all.
    # In this case we may have targets from a source installation hdf5-config.cmake,
    # or from cmake-bundled FindHDF5.cmake from CMake version < 3.20
    # We can differentiate them: The ones from FindHDF5.cmake lack suffix "-<static|shared>"

    list(APPEND HDF5_TARGET_NAMES hdf5 hdf5_hl hdf5_cpp hdf5_hl_cpp hdf5_cpp_hl hdf5_fortran hdf5_fortran_hl)
    if(NOT BUILD_SHARED_LIBS OR HDF5_USE_STATIC_LIBRARIES)
        list(APPEND HDF5_LINK_TYPE static)
    else()
        list(APPEND HDF5_LINK_TYPE shared)
    endif()
    foreach(name ${HDF5_TARGET_NAMES})
        foreach(type ${HDF5_LINK_TYPE})
            foreach(tgt ${name} hdf5::${name})
                if(TARGET ${tgt})
                    # tgt is not self-contained!
                    if(name STREQUAL "hdf5")
                        set(HDF5_C_TARGET ${tgt})
                    else()
                        list(APPEND HDF5_X_TARGETS ${tgt})
                    endif()
                endif()
            endforeach()

            foreach(tgt ${name}-${type} hdf5::${name}-${type})
                if(TARGET ${tgt})
                    # tgt is probably self-contained!
                    list(APPEND HDF5_X_TARGETS ${tgt})
                endif()
            endforeach()
        endforeach()
    endforeach()

    if(HDF5_C_TARGET AND HDF5_X_TARGETS)
        # The targets are not self-contained so we handle their interdependencies here
        # All found libraries depend on the main C library, so we link them
        h5pp_make_hdf5_target_from_vars(${HDF5_C_TARGET})
        foreach(tgtx ${HDF5_X_TARGETS})
            target_link_libraries(${tgtx} INTERFACE ${HDF5_C_TARGET})
        endforeach()
    endif()

    if(HDF5_X_TARGETS)
        add_library(HDF5::HDF5 INTERFACE IMPORTED)
        target_link_libraries(HDF5::HDF5 INTERFACE ${HDF5_X_TARGETS} ${HDF5_C_TARGET})
    elseif(HDF5_LIBRARIES)
        h5pp_make_hdf5_target_from_vars(HDF5::HDF5) # This should be a last resort
    else()
        message(FATAL_ERROR "Failed to define a standard HDF5::HDF5: Could not find any known HDF5 targets or variable HDF5_LIBRARIES")
    endif()

endfunction()