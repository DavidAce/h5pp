
if(H5PP_DOWNLOAD_METHOD)
    message(WARNING "The variable [H5PP_DOWNLOAD_METHOD] has been deprecated\n"
            "Use the following variable instead:\n"
            "\t H5PP_PACKAGE_MANAGER:STRING=[find|cmake|fetch|find-or-cmake|find-or-fetch|conan]")
    set(H5PP_PACKAGE_MANAGER ${H5PP_DOWNLOAD_METHOD} CACHE STRING "")
endif()

if(H5PP_DEPS_IN_SUBDIR)
    message(WARNING "The option [H5PP_DEPS_IN_SUBDIR] has been deprecated\n"
            "Use the following variable instead:\n"
            "\t H5PP_PREFIX_ADD_PKGNAME:BOOL=[TRUE|FALSE]")
    set(H5PP_PREFIX_ADD_PKGNAME ${H5PP_DEPS_IN_SUBDIR})
endif()

if(H5PP_PRINT_INFO)
    message(WARNING "The option [H5PP_PRINT_INFO] has been deprecated\n"
            "Use the built-in CMake CLI option --loglevel=[TRACE|DEBUG|VERBOSE|STATUS...] instead")
endif()