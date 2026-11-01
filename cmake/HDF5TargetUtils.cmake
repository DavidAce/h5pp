# Give h5pp one target to link to: h5pp::hdf5, containing the HDF5 C library,
# its high-level C library (HL), and the dependencies needed to use them.
# This target is "self-contained": a project only needs to link to h5pp::hdf5
# to get both libraries, their dependencies, header paths, and compiler
# definitions. It should not need to find or link zlib, szip, or system
# libraries separately just to use the HDF5 C and HL APIs.
# Call this function after find_package(HDF5 COMPONENTS C HL).
#
# HDF5 installations describe their libraries in different ways:
#   - Conan can provide a self-contained HDF5::HDF5 target.
#   - CMake's FindHDF5 module provides HDF5::HDF5 too, but reports the HL
#     libraries separately. That target alone is therefore not self-contained
#     for h5pp's C and HL requirements. Older find modules may provide only
#     library lists.
#   - HDF5's own hdf5-config.cmake can provide separate C and HL targets,
#     with names that depend on the version and installation settings.
#
# This function collects those results into h5pp::hdf5. Existing HDF5 targets
# are left intact: their link settings may already specify compression libraries,
# system libraries, and different files for Debug and Release. Replacing those
# settings or searching for those libraries again could select different files
# from the ones the HDF5 installation requires.
#
# The same function is used when building h5pp and when another project loads
# an installed h5pp with find_package(h5pp).
function(h5pp_get_modern_hdf5_target_name)
    if(NOT HDF5_FOUND)
        message(FATAL_ERROR "Call find_package(HDF5 COMPONENTS C HL) before configuring h5pp's HDF5 target")
    endif()
    if(TARGET h5pp::hdf5)
        return()
    endif()

    # FindHDF5 reports the high-level libraries separately from the C library.
    # Use its library lists as provided: entries may select different files for
    # Debug and Release, and their order can matter when linking static libraries.
    set(hdf5_hl_libs ${HDF5_HL_LIBRARIES})
    if(NOT hdf5_hl_libs)
        set(hdf5_hl_libs ${HDF5_C_HL_LIBRARIES})
    endif()

    if(TARGET HDF5::HDF5)
        # Use HDF5::HDF5 when available. Add any separately reported
        # HL libraries before it, so a static linker encounters HL's references
        # before the C library that supplies the corresponding functions.
        set(hdf5_link_libraries ${hdf5_hl_libs} HDF5::HDF5)
    elseif(HDF5_LIBRARIES OR HDF5_C_LIBRARIES)
        # Some older find modules and distribution packages provide library
        # lists without an HDF5::HDF5 target. Keep their paths and linker options.
        if(HDF5_LIBRARIES)
            set(hdf5_link_libraries ${hdf5_hl_libs} ${HDF5_LIBRARIES})
        else()
            set(hdf5_link_libraries ${hdf5_hl_libs} ${HDF5_C_LIBRARIES})
        endif()
    else()
        # If only separate C and HL targets are available, find a matching pair.
        # When checking names ending in -static or -shared, try the user's HDF5
        # preference first, followed by the type selected by hdf5-config.cmake.
        # Otherwise try shared, then static. BUILD_SHARED_LIBS is not used here:
        # it controls what a project builds, not which installed HDF5 it uses.
        set(hdf5_library_types ${HDF5_LIB_TYPE})
        if(DEFINED HDF5_USE_STATIC_LIBRARIES)
            if(HDF5_USE_STATIC_LIBRARIES)
                list(PREPEND hdf5_library_types static)
            else()
                list(PREPEND hdf5_library_types shared)
            endif()
        endif()
        list(APPEND hdf5_library_types shared static)
        list(REMOVE_DUPLICATES hdf5_library_types)

        # Names such as hdf5::hdf5 and hdf5::hdf5_hl already refer to specific
        # libraries. Use them directly, without choosing static or shared again.
        foreach(hdf5_target_prefix "hdf5::" "")
            if(TARGET ${hdf5_target_prefix}hdf5 AND TARGET ${hdf5_target_prefix}hdf5_hl)
                set(hdf5_link_libraries ${hdf5_target_prefix}hdf5_hl ${hdf5_target_prefix}hdf5)
                break()
            endif()
        endforeach()
        if(NOT hdf5_link_libraries)
            foreach(type IN LISTS hdf5_library_types)
                string(TOUPPER "${type}" Type)
                # Prefer the target names reported by hdf5-config.cmake when
                # available. They also work if HDF5 was installed with a custom
                # prefix instead of hdf5:: (HDF_PACKAGE_NAMESPACE).
                if(HDF5_C_${Type}_LIBRARY AND HDF5_HL_${Type}_LIBRARY)
                    set(hdf5_link_libraries ${HDF5_HL_${Type}_LIBRARY} ${HDF5_C_${Type}_LIBRARY})
                    break()
                endif()
                foreach(hdf5_target_prefix "hdf5::" "")
                    if(TARGET ${hdf5_target_prefix}hdf5-${type} AND TARGET ${hdf5_target_prefix}hdf5_hl-${type})
                        set(hdf5_link_libraries ${hdf5_target_prefix}hdf5_hl-${type} ${hdf5_target_prefix}hdf5-${type})
                        break()
                    endif()
                endforeach()
                if(hdf5_link_libraries)
                    break()
                endif()
            endforeach()
        endif()
        if(NOT hdf5_link_libraries)
            message(FATAL_ERROR
                "HDF5 was found, but h5pp could not identify its C and HL libraries. "
                "Set HDF5_DIR to a complete HDF5 package, or use CMake's FindHDF5 module.")
        endif()
    endif()

    # Create a target for this project's HDF5 installation. When another
    # project loads an installed h5pp, h5ppDeps.cmake calls this function again
    # using that project's find_package results. This avoids recording this
    # machine's dependency paths in the installed h5pp target files.
    add_library(h5pp::hdf5 INTERFACE IMPORTED)
    target_link_libraries(h5pp::hdf5 INTERFACE ${hdf5_link_libraries})

    # Library lists alone do not carry header paths or compiler definitions.
    # Add those reported by find_package as well, for find modules that do not
    # provide targets with this information attached.
    set(hdf5_include_dirs ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIR} ${HDF5_C_INCLUDE_DIRS})
    list(REMOVE_DUPLICATES hdf5_include_dirs)
    if(hdf5_include_dirs)
        target_include_directories(h5pp::hdf5 SYSTEM INTERFACE ${hdf5_include_dirs})
    endif()
    foreach(hdf5_compile_definition IN LISTS HDF5_DEFINITIONS HDF5_C_DEFINITIONS)
        # FindHDF5 may return -DNAME=value; target_compile_definitions expects
        # NAME=value, so remove the leading -D if present.
        string(REGEX REPLACE "^-D" "" hdf5_compile_definition "${hdf5_compile_definition}")
        target_compile_definitions(h5pp::hdf5 INTERFACE "${hdf5_compile_definition}")
    endforeach()
    message(VERBOSE "h5pp HDF5 libraries: ${hdf5_link_libraries}")
endfunction()
