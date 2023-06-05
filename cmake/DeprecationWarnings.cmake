if(H5PP_PACKAGE_MANAGER STREQUAL "find")
    message(WARNING
            "The variable [H5PP_PACKAGE_MANAGER=${H5PP_PACKAGE_MANAGER}] has been deprecated and will be ignored.")
elseif(H5PP_PACKAGE_MANAGER)
    message(FATAL_ERROR
            "The variable [H5PP_PACKAGE_MANAGER=${H5PP_PACKAGE_MANAGER}] has been deprecated.\n"
            "Instead use the CMake dependency provider mechanism (requires at least CMake 3.24):\n"
            "   https://cmake.org/cmake/help/latest/guide/using-dependencies/index.html#dependency-providers \n"
            "To enable, run \n"
            "   cmake -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES= ... \n"
            "pointing to either \n"
            "   (1) <h5pp-source>/cmake/conan_dependency_provider/conan_provider.cmake (to let conan install libraries) \n"
            "   (2) <h5pp-source>/cmake/cmake_dependency_provider/cmake_provider.cmake (to let cmake install libraries) \n"
            "Alternatively, use a bundled CMake Presets for similar effect. To list presets, run:\n"
            "   cmake --list-presets\n"
            )
endif()
if(H5PP_DOWNLOAD_METHOD)
    message(WARNING "The variable [H5PP_DOWNLOAD_METHOD] has been deprecated.\n"
            "Use the following variable instead:\n"
            "\t H5PP_PACKAGE_MANAGER:STRING=[find|cmake|fetch|find-or-cmake|find-or-fetch|conan]")
    set(H5PP_PACKAGE_MANAGER ${H5PP_DOWNLOAD_METHOD} CACHE STRING "")
endif()

if(H5PP_DEPS_IN_SUBDIR)
    message(WARNING "The option [H5PP_DEPS_IN_SUBDIR] has been deprecated.\n"
            "Use the following variable instead:\n"
            "\t H5PP_PREFIX_ADD_PKGNAME:BOOL=[TRUE|FALSE]")
    set(H5PP_PREFIX_ADD_PKGNAME ${H5PP_DEPS_IN_SUBDIR})
endif()

if(H5PP_PRINT_INFO)
    message(WARNING "The option [H5PP_PRINT_INFO] has been deprecated\n"
            "Use the built-in CMake CLI option --loglevel=[TRACE|DEBUG|VERBOSE|STATUS...] instead")
endif()