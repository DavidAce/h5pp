# Give h5pp one self-contained target: linking to h5pp::hdf5 supplies the
# HDF5 C and high-level (HL) libraries, their headers, compiler definitions,
# and other libraries they need. Call this after find_package(HDF5 COMPONENTS C HL).
# A CMake target can carry all of that information; a library file path cannot.
#
# find_package can use HDF5's installed hdf5-config.cmake or CMake's FindHDF5
# module. The config file normally describes the C and HL libraries as targets
# with their dependencies attached. FindHDF5 can load those targets, then put
# only library file paths in its combined HDF5::HDF5 target. For example, linking
# libhdf5.a alone does not automatically link the zlib or SZIP it was built with.
#
# Prefer the C/HL targets supplied by HDF5's config file when they describe the
# same library file that find_package selected. Otherwise, use HDF5::HDF5 or the
# reported library lists. h5pp creates its own target and keeps the existing
# targets' link settings, which can include package-specific dependency choices.

# Set the caller's variable named by result to TRUE if tgt refers to a file
# in HDF5_LIBRARIES. Imported targets describe libraries built outside this
# project. Their file paths are stored in IMPORTED_LOCATION properties.
# The caller supplies paths with symlinks resolved. Check both IMPORTED_LOCATION
# and configuration-specific locations, since a package may provide separate
# Debug and Release files. This lets us recover a library's target and its
# dependencies while keeping the library selection made by find_package.
function(h5pp_hdf5_target_matches_library tgt HDF5_LIBRARIES result)
    set(${result} FALSE PARENT_SCOPE)
    get_target_property(configs ${tgt} IMPORTED_CONFIGURATIONS)
    set(properties IMPORTED_LOCATION)
    if(configs)
        foreach(config IN LISTS configs)
            string(TOUPPER "${config}" config)
            list(APPEND properties "IMPORTED_LOCATION_${config}")
        endforeach()
    endif()
    foreach(property IN LISTS properties)
        get_target_property(lib ${tgt} ${property})
        if(EXISTS "${lib}" AND NOT IS_DIRECTORY "${lib}")
            file(REAL_PATH "${lib}" lib)
            if(lib IN_LIST HDF5_LIBRARIES)
                set(${result} TRUE PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
endfunction()

function(h5pp_get_modern_hdf5_target_name)
    if(NOT HDF5_FOUND)
        message(FATAL_ERROR "Call find_package(HDF5 COMPONENTS C HL) before configuring h5pp's HDF5 target")
    endif()
    if(TARGET h5pp::hdf5)
        return()
    endif()

    # For names ending in -static or -shared, try the user's explicit HDF5
    # preference first, then the type reported by HDF5's config file. The final
    # shared/static entries cover packages that report neither setting.
    # If find_package already selected library files, the file comparison below
    # takes precedence over this search order. BUILD_SHARED_LIBS is not used:
    # it controls what a project builds, not which installed library it links.
    set(HDF5_LIBRARY_TYPES ${HDF5_LIB_TYPE})
    if(DEFINED HDF5_USE_STATIC_LIBRARIES)
        if(HDF5_USE_STATIC_LIBRARIES)
            list(PREPEND HDF5_LIBRARY_TYPES static)
        else()
            list(PREPEND HDF5_LIBRARY_TYPES shared)
        endif()
    endif()
    list(APPEND HDF5_LIBRARY_TYPES shared static)
    list(REMOVE_DUPLICATES HDF5_LIBRARY_TYPES)

    # Collect candidate C/HL pairs once, including names with and without the
    # hdf5:: namespace and static/shared suffixes. The lists are parallel: a C
    # target and the HL target at the same index form a pair for ZIP_LISTS below,
    # for example hdf5::hdf5-static and hdf5::hdf5_hl-static.
    # X denotes the companion HL library; h5pp does not need the C++, Fortran,
    # or Java libraries. Candidates need not exist; they are checked below.
    set(HDF5_TARGET_NAMES hdf5::hdf5 hdf5)
    set(HDF5_C_TARGET)
    set(HDF5_X_TARGETS)
    foreach(name IN LISTS HDF5_TARGET_NAMES)
        list(APPEND HDF5_C_TARGET ${name})
        list(APPEND HDF5_X_TARGETS ${name}_hl)
    endforeach()
    foreach(type IN LISTS HDF5_LIBRARY_TYPES)
        string(TOUPPER "${type}" Type)
        if(HDF5_C_${Type}_LIBRARY AND HDF5_HL_${Type}_LIBRARY)
            # These names also cover HDF5 installations with a custom namespace.
            list(APPEND HDF5_C_TARGET ${HDF5_C_${Type}_LIBRARY})
            list(APPEND HDF5_X_TARGETS ${HDF5_HL_${Type}_LIBRARY})
        endif()
        foreach(name IN LISTS HDF5_TARGET_NAMES)
            list(APPEND HDF5_C_TARGET ${name}-${type})
            list(APPEND HDF5_X_TARGETS ${name}_hl-${type})
        endforeach()
    endforeach()

    # When find_package supplies HDF5::HDF5 or library lists, replace them only
    # if an exported target refers to the same C library file. A complete
    # HDF5::HDF5 target that links through other target names, as Conan's does,
    # can therefore pass through unchanged via the fallback below.
    set(HDF5_MATCH_SELECTED_LIBRARY FALSE)
    if(TARGET HDF5::HDF5 OR HDF5_LIBRARIES OR HDF5_C_LIBRARIES)
        set(HDF5_MATCH_SELECTED_LIBRARY TRUE)
    endif()
    # Only existing files can be compared with imported target locations.
    # Other entries can be target names or instructions for choosing a library:
    # debug/optimized keywords and $<...> generator expressions, for example.
    # Skip those here; the fallback still uses the original lists unchanged.
    set(HDF5_SELECTED_C_LIBRARIES)
    foreach(lib IN LISTS HDF5_C_LIBRARIES HDF5_LIBRARIES)
        if(EXISTS "${lib}" AND NOT IS_DIRECTORY "${lib}")
            file(REAL_PATH "${lib}" lib)
            list(APPEND HDF5_SELECTED_C_LIBRARIES "${lib}")
        endif()
    endforeach()

    # With reported library files, look for a target pair describing those files.
    # With only separate targets available, use the first existing pair in the
    # search order above. In both cases, linking the targets brings along the
    # dependencies declared by their HDF5 package.
    set(HDF5_LINK_LIBS)
    foreach(tgt tgtx IN ZIP_LISTS HDF5_C_TARGET HDF5_X_TARGETS)
        if(NOT TARGET "${tgt}" OR NOT TARGET "${tgtx}")
            continue()
        endif()
        if(HDF5_MATCH_SELECTED_LIBRARY)
            # Some FindHDF5 component targets also contain only file paths.
            # Replacing its combined target with those would not recover any
            # missing dependencies, so check for link relationships first.
            get_target_property(HDF5_INTERFACE_LINK_LIBRARIES ${tgt} INTERFACE_LINK_LIBRARIES)
            get_target_property(HDF5_X_LINK_LIBRARIES ${tgtx} INTERFACE_LINK_LIBRARIES)
            if(NOT HDF5_INTERFACE_LINK_LIBRARIES AND NOT HDF5_X_LINK_LIBRARIES)
                continue() # FindHDF5's individual targets can lose dependencies too.
            endif()
            h5pp_hdf5_target_matches_library(${tgt} "${HDF5_SELECTED_C_LIBRARIES}" HDF5_TARGET_MATCHES)
            if(NOT HDF5_TARGET_MATCHES)
                continue()
            endif()
        endif()
        set(HDF5_LINK_LIBS ${tgtx} ${tgt})
        break()
    endforeach()

    # A matching exported pair is not always available. Keep a complete
    # HDF5::HDF5 target when provided; older find modules may supply only lists.
    # FindHDF5 reports HL separately, so add it to either form of the C libraries.
    if(NOT HDF5_LINK_LIBS)
        set(HDF5_X_LIBRARIES ${HDF5_HL_LIBRARIES})
        if(NOT HDF5_X_LIBRARIES)
            set(HDF5_X_LIBRARIES ${HDF5_C_HL_LIBRARIES})
        endif()
        # The static HL library calls functions in the C library, so put HL
        # first: a static linker needs to encounter the C library afterward.
        # Preserve debug/optimized entries and generator expressions so CMake
        # can still select the appropriate files for each build configuration.
        if(TARGET HDF5::HDF5)
            set(HDF5_LINK_LIBS ${HDF5_X_LIBRARIES} HDF5::HDF5)
        elseif(HDF5_LIBRARIES OR HDF5_C_LIBRARIES)
            set(HDF5_LINK_LIBS ${HDF5_X_LIBRARIES} ${HDF5_LIBRARIES})
            if(NOT HDF5_LIBRARIES)
                set(HDF5_LINK_LIBS ${HDF5_X_LIBRARIES} ${HDF5_C_LIBRARIES})
            endif()
        else()
            message(FATAL_ERROR "HDF5 was found, but h5pp could not identify its C and HL libraries. Set HDF5_DIR to a complete HDF5 package, or use CMake's FindHDF5 module.")
        endif()
    endif()

    # When another project loads an installed h5pp package, it calls this
    # function again. Build h5pp::hdf5 from that project's find_package results
    # so its paths describe the HDF5 installation available on that machine.
    add_library(h5pp::hdf5 INTERFACE IMPORTED)
    target_link_libraries(h5pp::hdf5 INTERFACE ${HDF5_LINK_LIBS})
    # Raw library lists do not carry header paths or compiler definitions.
    # Add the values reported by find_package for those installations too.
    set(HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIR} ${HDF5_C_INCLUDE_DIRS})
    list(REMOVE_DUPLICATES HDF5_INCLUDE_DIRS)
    if(HDF5_INCLUDE_DIRS)
        target_include_directories(h5pp::hdf5 SYSTEM INTERFACE ${HDF5_INCLUDE_DIRS})
    endif()
    # find_package may return -DNAME=value; target_compile_definitions expects
    # NAME=value, so remove that command-line prefix when present.
    foreach(def IN LISTS HDF5_DEFINITIONS HDF5_C_DEFINITIONS)
        string(REGEX REPLACE "^-D" "" def "${def}")
        target_compile_definitions(h5pp::hdf5 INTERFACE "${def}")
    endforeach()
    message(VERBOSE "h5pp HDF5 libraries: ${HDF5_LINK_LIBS}")
endfunction()
