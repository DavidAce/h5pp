function(h5pp_make_hdf5_target)
    if(TARGET HDF5::HDF5)
        message(VERBOSE "Detected existing target HDF5::HDF5")
        get_target_property(H5PP_HDF5_PROPERTY_IMPORTED_LOCATION        HDF5::HDF5 LOCATION)
        get_target_property(H5PP_HDF5_PROPERTY_INTERFACE_LINK_LIBRARIES HDF5::HDF5 INTERFACE_LINK_LIBRARIES)
        set_target_properties(HDF5::HDF5 PROPERTIES INTERFACE_LINK_LIBRARIES "")  # Clean it up
        set_target_properties(HDF5::HDF5 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "")  # Clean it up
        set_target_properties(HDF5::HDF5 PROPERTIES INCLUDE_DIRECTORIES "")  # Clean it up
        mark_as_advanced(H5PP_HDF5_PROPERTY_IMPORTED_LOCATION)
        mark_as_advanced(H5PP_HDF5_PROPERTY_INTERFACE_LINK_LIBRARIES)
    else()
        message(VERBOSE "Defining target HDF5::HDF5")
        add_library(HDF5::HDF5 INTERFACE IMPORTED)
    endif()

    target_include_directories(HDF5::HDF5 SYSTEM INTERFACE ${HDF5_INCLUDE_DIRS})
    target_include_directories(HDF5::HDF5 SYSTEM INTERFACE ${HDF5_INCLUDE_DIR})
    target_compile_definitions(HDF5::HDF5 INTERFACE ${HDF5_DEFINITIONS})


    message(VERBOSE "HDF5_LIBRARIES                          : ${HDF5_LIBRARIES}")
    message(VERBOSE "HDF5_C_HL_LIBRARIES                     : ${HDF5_C_HL_LIBRARIES}")
    message(VERBOSE "HDF5_HL_LIBRARIES                       : ${HDF5_HL_LIBRARIES}")
    message(VERBOSE "HDF5_C_LIBRARIES                        : ${HDF5_C_LIBRARIES}")

    # Link found libraries
    foreach(lib  ${H5PP_HDF5_PROPERTY_IMPORTED_LOCATION} ${H5PP_HDF5_PROPERTY_INTERFACE_LINK_LIBRARIES} ${HDF5_LIBRARIES})
        if(NOT lib)
            continue()
        endif()
        get_filename_component(name_we ${lib} NAME_WE)
        string(REPLACE lib "" name ${name_we})
#        message(STATUS "Linking HDF5::HDF5 with [${name}]: ${lib}")
        if(TARGET ${lib})
             list(APPEND H5PP_HDF5_INTERFACE_LINK_TARGETS ${lib})
        elseif(TARGET ${name})
             list(APPEND H5PP_HDF5_INTERFACE_LINK_TARGETS ${name})
        elseif(TARGET hdf5::${name})
             list(APPEND H5PP_HDF5_INTERFACE_LINK_TARGETS hdf5::${name})
        elseif("${name}" MATCHES "hdf5" AND EXISTS "${lib}")
            if(NOT TARGET hdf5::${name})
                add_library(hdf5::${name} INTERFACE IMPORTED)
                set_target_properties(hdf5::${name} PROPERTIES IMPORTED_LOCATION "${lib}")
            endif()
            list(APPEND H5PP_HDF5_INTERFACE_LINK_TARGETS hdf5::${name})
        elseif("${name}" MATCHES "pthread") # Link with -pthread instead of libpthread directly
            find_package(Threads REQUIRED)
            list(APPEND H5PP_HDF5_INTERFACE_LINK_LIBRARIES Threads::Threads)
        elseif(EXISTS "${lib}")
            find_library(${name}_LIBRARY NAMES ${name} QUIET REQUIRED)
            list(APPEND H5PP_HDF5_INTERFACE_LINK_LIBRARIES ${name})
            if("${name}" MATCHES "sz")
                find_library(aec_LIBRARY NAMES aec QUIET)
                if(aec_LIBRARY_FOUND)
                    list(APPEND H5PP_HDF5_INTERFACE_LINK_LIBRARIES aec)
                endif()
            endif()
        endif()
    endforeach()
    mark_as_advanced(H5PP_HDF5_INTERFACE_LINK_LIBRARIES)

    foreach(tgt ${H5PP_HDF5_INTERFACE_LINK_TARGETS})
        foreach(lib ${H5PP_HDF5_INTERFACE_LINK_LIBRARIES})
            target_link_libraries(${tgt} INTERFACE ${lib})
        endforeach()
    endforeach()

    foreach(tgt ${H5PP_HDF5_INTERFACE_LINK_TARGETS})
        target_link_libraries(HDF5::HDF5 INTERFACE ${tgt})
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
    # CMake version >= 3.19 defines target hdf5::hdf5_hl (not self-contained)
    # hdf5-config.cmake from source defines target hdf5_hl-<static|shared> (self-contained)

    if(NOT HDF5_FOUND)
        message(FATAL_ERROR "h5pp_get_modern_hdf5_target_name: HDF5_FOUND is not TRUE. Call find_package(HDF5) first.")
    endif()


    # If HDF5::HDF5 is defined without HDF5::ALIAS, then it can come from conan or cmake-bundled FindHDF5.cmake
    # We can test it by checking if its interface libraries match "CONAN"
    if(TARGET HDF5::HDF5)
        get_target_property(H5PP_HDF5_LINK_LIBS HDF5::HDF5 INTERFACE_LINK_LIBRARIES)
        foreach(tgt ${H5PP_HDF5_LINK_LIBS})
            if("${tgt}" MATCHES "CONAN|conan")
                return()
            endif()
            if(TARGET ${tgt})
                get_target_property(HDF5_HDF5_LINK_SUBLIBS ${tgt} INTERFACE_LINK_LIBRARIES)
                foreach(sub ${HDF5_HDF5_LINK_SUBLIBS})
                    if("${sub}" MATCHES "CONAN|conan")
                        return()
                    endif()
                endforeach()
            endif()
        endforeach()
    endif()

    # In this case we may have targets from a source installation hdf5-config.cmake, or from CMake version < 3.20
    # The targets from FindHDF5.cmake lack interdependencies between each other.
    # We can differentiate them: The ones from FindHDF5.cmake lack suffix "-<static|shared>"


    list(APPEND HDF5_TARGET_NAMES hdf5 hdf5_hl hdf5_cpp hdf5_hl_cpp hdf5_cpp_hl hdf5_fortran hdf5_fortran_hl)
    foreach(name ${HDF5_TARGET_NAMES})
        foreach(tgt ${name} ${name}-static ${name}-shared hdf5::${name} hdf5::${name}-static hdf5::${name}-shared )
            if(TARGET ${tgt})
                # tgt is not self-contained!
                if(name STREQUAL "hdf5")
                    if(${tgt} MATCHES "static")
                        list(APPEND HDF5_C_STATIC_TARGET ${tgt})
                    elseif(${tgt} MATCHES "shared")
                        list(APPEND HDF5_C_SHARED_TARGET ${tgt})
                    else()
                        list(APPEND HDF5_C_UNKNOWN_TARGET ${tgt})
                    endif()
                else()
                    if(${tgt} MATCHES "static")
                        list(APPEND HDF5_X_STATIC_TARGETS ${tgt})
                    elseif(${tgt} MATCHES "shared")
                        list(APPEND HDF5_X_SHARED_TARGETS ${tgt})
                    else()
                        list(APPEND HDF5_X_UNKNOWN_TARGETS ${tgt})
                    endif()
                endif()
            endif()
        endforeach()
    endforeach()

    if(BUILD_SHARED_LIBS OR NOT HDF5_USE_STATIC_LIBRARIES)
        # Prefer shared
        if(HDF5_C_SHARED_TARGET AND HDF5_X_SHARED_TARGETS)
            list(APPEND HDF5_C_TARGET ${HDF5_C_SHARED_TARGET} ${HDF5_C_UNKNOWN_TARGET})
            list(APPEND HDF5_X_TARGETS ${HDF5_X_SHARED_TARGETS} ${HDF5_X_UNKNOWN_TARGETS})
        elseif(HDF5_C_STATIC_TARGET AND HDF5_X_STATIC_TARGETS)
            list(APPEND HDF5_C_TARGET ${HDF5_C_STATIC_TARGET} ${HDF5_C_UNKNOWN_TARGET})
            list(APPEND HDF5_X_TARGETS ${HDF5_X_STATIC_TARGETS} ${HDF5_X_UNKNOWN_TARGETS})
        endif()
        if(NOT HDF5_C_TARGET)
            set(HDF5_C_TARGET ${HDF5_C_SHARED_TARGET} ${HDF5_C_STATIC_TARGET} ${HDF5_C_UNKNOWN_TARGET})
        endif()
        if(NOT HDF5_X_TARGETS)
            set(HDF5_X_TARGETS ${HDF5_X_SHARED_TARGETS} ${HDF5_X_STATIC_TARGETS} ${HDF5_X_UNKNOWN_TARGETS})
        endif()
        list(REMOVE_DUPLICATES HDF5_C_TARGET)
        list(REMOVE_DUPLICATES HDF5_X_TARGETS)
    elseif(NOT BUILD_SHARED_LIBS OR HDF5_USE_STATIC_LIBRARIES)
        # Prefer static
        if(HDF5_C_STATIC_TARGET AND HDF5_X_STATIC_TARGETS)
            list(APPEND HDF5_C_TARGET ${HDF5_C_STATIC_TARGET} ${HDF5_C_UNKNOWN_TARGET})
            list(APPEND HDF5_X_TARGETS ${HDF5_X_STATIC_TARGETS} ${HDF5_X_UNKNOWN_TARGETS})
        elseif(HDF5_C_SHARED_TARGET AND HDF5_X_SHARED_TARGETS)
            list(APPEND HDF5_C_TARGET ${HDF5_C_SHARED_TARGET} ${HDF5_C_UNKNOWN_TARGET})
            list(APPEND HDF5_X_TARGETS ${HDF5_X_SHARED_TARGETS} ${HDF5_X_UNKNOWN_TARGETS})
        endif()
        if(NOT HDF5_C_TARGET)
            set(HDF5_C_TARGET ${HDF5_C_STATIC_TARGET} ${HDF5_C_SHARED_TARGET} ${HDF5_C_UNKNOWN_TARGET})
        endif()
        if(NOT HDF5_X_TARGETS)
            set(HDF5_X_TARGETS ${HDF5_X_STATIC_TARGETS} ${HDF5_X_SHARED_TARGETS} ${HDF5_X_UNKNOWN_TARGETS})
        endif()
        list(REMOVE_DUPLICATES HDF5_C_TARGET)
        list(REMOVE_DUPLICATES HDF5_X_TARGETS)
    endif()
    message(VERBOSE "HDF5_C_TARGET          : ${HDF5_C_TARGET}")
    message(VERBOSE "HDF5_C_SHARED_TARGET   : ${HDF5_C_SHARED_TARGET}")
    message(VERBOSE "HDF5_C_STATIC_TARGET   : ${HDF5_C_STATIC_TARGET}")
    message(VERBOSE "HDF5_C_UNKNOWN_TARGET  : ${HDF5_C_UNKNOWN_TARGET}")
    message(VERBOSE "HDF5_X_TARGETS         : ${HDF5_X_TARGETS}")
    message(VERBOSE "HDF5_X_SHARED_TARGETS  : ${HDF5_X_SHARED_TARGETS}")
    message(VERBOSE "HDF5_X_STATIC_TARGETS  : ${HDF5_X_STATIC_TARGETS}")
    message(VERBOSE "HDF5_X_UNKNOWN_TARGETS : ${HDF5_X_UNKNOWN_TARGETS}")


    if(HDF5_C_TARGET AND HDF5_X_TARGETS)
        # The targets are not necessarily self-contained so we handle their interdependencies here
        # All found libraries depend on the main C library, so link to it to get the link order right
        foreach(tgtx ${HDF5_X_TARGETS})
            target_link_libraries(${tgtx} INTERFACE ${HDF5_C_TARGET})
        endforeach()
    endif()

    h5pp_make_hdf5_target()

    if(HDF5_X_TARGETS)
        get_target_property(HDF5_LINK_LIBS HDF5::HDF5 INTERFACE_LINK_LIBRARIES)
        foreach(tgt ${HDF5_X_TARGETS};${HDF5_C_TARGET})
            if(NOT ${tgt} IN_LIST HDF5_LINK_LIBS)
                message(DEBUG "LINKING HDF5::HDF5 INTERFACE ${tgt}")
                target_link_libraries(HDF5::HDF5 INTERFACE ${tgt})
            endif()
        endforeach()
    elseif(HDF5_LIBRARIES)
        h5pp_make_hdf5_target() # This should be a last resort
    else()
        message(FATAL_ERROR "Failed to define a standard HDF5::HDF5: Could not find any known HDF5 targets or variable HDF5_LIBRARIES")
    endif()


    # Check whether zlib or szip are linked to the targets, and make sure we can find them
    foreach (tgt ${HDF5_X_TARGETS};${HDF5_C_TARGET})
        get_target_property(HDF5_INTERFACE_LINK_LIBRARIES ${tgt} INTERFACE_LINK_LIBRARIES)
        foreach (ltgt ${HDF5_INTERFACE_LINK_LIBRARIES})
            if(ltgt MATCHES "ZLIB::ZLIB")
                set(HDF5_ZLIB_REQUIRED REQUIRED)
            endif()
            if(ltgt STREQUAL "z")
                set(HDF5_z_REQUIRED REQUIRED)
            endif()
            if(ltgt MATCHES "SZIP::SZIP")
                set(HDF5_SZIP_REQUIRED REQUIRED)
            endif()
            if(ltgt STREQUAL "sz")
                set(HDF5_sz_REQUIRED REQUIRED)
            endif()
        endforeach()
    endforeach ()
    find_package(ZLIB ${HDF5_ZLIB_REQUIRED} QUIET)
    find_package(SZIP ${HDF5_SZIP_REQUIRED} QUIET)
    find_library(LIBRARY_Z z ${HDF5_z_REQUIRED} QUIET)
    find_library(LIBRARY_SZ sz ${HDF5_sz_REQUIRED} QUIET)
endfunction()