
if(H5PP_DOWNLOAD_METHOD)
    message(WARNING "The following CMake variable has been deprecated\n"
            "\t H5PP_DOWNLOAD_METHOD=[none|find|fetch|find-or-fetch|conan]\n"
            "Update this variable to\n"
            "\t H5PP_PACKAGE_MANAGER=[find|cmake|find-or-cmake|conan]")
endif()