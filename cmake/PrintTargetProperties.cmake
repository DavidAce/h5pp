cmake_minimum_required(VERSION 3.15)
# Taken from
# https://stackoverflow.com/questions/32183975/how-to-print-all-the-properties-of-a-target-in-cmake


# Get all propreties that cmake supports
execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

# Convert command output into a CMake list
STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
# Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
#list(FILTER CMAKE_PROPERTY_LIST EXCLUDE REGEX "^LOCATION$|^LOCATION_|_LOCATION$")
# For some reason, "TYPE" shows up twice - others might too?
list(REMOVE_DUPLICATES CMAKE_PROPERTY_LIST)

# build whitelist by filtering down from CMAKE_PROPERTY_LIST in case cmake is
# a different version, and one of our hardcoded whitelisted properties
# doesn't exist!
unset(CMAKE_WHITELISTED_PROPERTY_LIST)
foreach(prop ${CMAKE_PROPERTY_LIST})
    # Edit: Added location
    if(prop MATCHES "(^LOCATION$|^LOCATION_|_LOCATION$)|^(INTERFACE|[_a-z]|IMPORTED_LIBNAME_|MAP_IMPORTED_CONFIG_)|^(COMPATIBLE_INTERFACE_(BOOL|NUMBER_MAX|NUMBER_MIN|STRING)|EXPORT_NAME|IMPORTED(_GLOBAL|_CONFIGURATIONS|_LIBNAME)?|NAME|TYPE|NO_SYSTEM_FROM_IMPORTED)$")
        list(APPEND CMAKE_WHITELISTED_PROPERTY_LIST ${prop})
    endif()
endforeach()

function(print_properties)
    message ("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction()

function(print_whitelisted_properties)
    message ("CMAKE_WHITELISTED_PROPERTY_LIST = ${CMAKE_WHITELISTED_PROPERTY_LIST}")
endfunction()

function(print_target_properties tgt)
    if(NOT TARGET ${tgt})
        message(STATUS "There is no target named '${tgt}'")
        return()
    endif()

    get_target_property(target_type ${tgt} TYPE)
    get_target_property(target_imported ${tgt} IMPORTED)

    if(NOT target_imported OR target_type MATCHES "EXECUTABLE")
        set(PROP_LIST ${CMAKE_WHITELISTED_PROPERTY_LIST})
        list(FILTER PROP_LIST EXCLUDE REGEX "^LOCATION$|^LOCATION_|_LOCATION$")
    elseif(target_imported AND target_type MATCHES "INTERFACE_LIBRARY") # An "INTERFACE IMPORTED" library
        set(PROP_LIST ${CMAKE_WHITELISTED_PROPERTY_LIST})
    else()
        set(PROP_LIST ${CMAKE_PROPERTY_LIST})
    endif()
    string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER )

    foreach (prop ${PROP_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE_UPPER}" prop ${prop})
        string(REPLACE "<LANG>" "CXX" prop ${prop})
        get_property(propval TARGET ${tgt} PROPERTY ${prop} SET)
        if (propval)
            if(propval MATCHES "LOCATION" AND target_type MATCHES MATCHES "INTERFACE" AND CMAKE_VERSION VERSION_LESS 3.19)
                continue()
            endif()
            get_target_property(propval ${tgt} ${prop})
            message (STATUS "${tgt} ${prop} = ${propval}")
        endif()
    endforeach()
endfunction()