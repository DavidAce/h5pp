
if(H5PP_DOWNLOAD_METHOD)
    message(WARNING "The following CMake variable has been deprecated\n"
            "\t H5PP_DOWNLOAD_METHOD=[none|find|fetch|find-or-fetch|conan]\n"
            "Update this variable to\n"
            "\t H5PP_PACKAGE_MANAGER=[find|cmake|find-or-cmake|conan]")
endif()

if(H5PP_DEPS_IN_SUBDIR)
    message(WARNING "The option [H5PP_DEPS_IN_SUBDIR] has been deprecated\n"
            "Use the following variable instead:\n"
            "\t H5PP_PREFIX_ADD_PKGNAME:BOOL=[TRUE|FALSE]")
    set(H5PP_PREFIX_ADD_PKGNAME ${H5PP_DEPS_IN_SUBDIR})
endif()