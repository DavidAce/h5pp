cmake_minimum_required(VERSION 3.15)

# Append search paths for find_package and find_library calls
if(NOT "${CMAKE_SOURCE_DIR}/cmake/modules" IN_LIST CMAKE_MODULE_PATH)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
endif()

# Transform CMAKE_INSTALL_PREFIX to full path
if(DEFINED CMAKE_INSTALL_PREFIX AND NOT IS_ABSOLUTE CMAKE_INSTALL_PREFIX)
    get_filename_component(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}
            ABSOLUTE BASE_DIR ${CMAKE_BINARY_DIR} CACHE FORCE)
    message(DEBUG "Setting absolute path CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
endif()


if(WIN32)
    # On Windows it is standard practice to collect binaries into one directory.
    # This way we avoid errors from .dll's not being found at runtime.
    # These directories will contain h5pp tests, examples and possibly dependencies
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" CACHE PATH "Collect .exe and .dll")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE PATH "Collect .lib")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE PATH "Collect .lib")
endif()
