function(CheckFileSystemCompiles target_name)
    if(TARGET ${target_name})
        get_target_property(TARGET_INC  ${target_name} INTERFACE_INCLUDE_DIRECTORIES)
        get_target_property(TARGET_LIB  ${target_name} INTERFACE_LINK_LIBRARIES)
        get_target_property(TARGET_OPT  ${target_name} INTERFACE_COMPILE_OPTIONS)

        if(TARGET_INC)
            string(REPLACE ";" " " CMAKE_REQUIRED_INCLUDES  "${TARGET_INC}")
        endif()
        if(TARGET_LIB)
            string(REPLACE ";" " " CMAKE_REQUIRED_LIBRARIES "${TARGET_LIB}")
        endif()
        if(TARGET_OPT)
            string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS     "${TARGET_OPT}")
        endif()

        include(CheckIncludeFileCXX)
        check_include_file_cxx(filesystem    has_filesystem  )
        if(NOT has_filesystem)
            message(FATAL_ERROR "\n\
                Missing one or more C++17 headers.\n\
                Consider using a newer compiler (GCC 8 or above, Clang 7 or above),\n\
                or checking the compiler flags. If using Clang, pass the variable \n\
                GCC_TOOLCHAIN=<path> \n\
                where path is the install directory of a recent GCC installation (version > 8).
                Also, don't forget to compile with flags:  [-lstdc++fs -std=c++17].
        ")
        endif()

        include(CheckCXXSourceCompiles)
        check_cxx_source_compiles("
        #include<filesystem>
        namespace fs = std::filesystem;
        int main(){
            fs::path testpath;
            return 0;
        }
        " FILESYSTEM_COMPILES)
        if(NOT FILESYSTEM_COMPILES)
            message(FATAL_ERROR "Unable to compile with filesystem headers")
        endif()
    else()
        message(WARNING "Target does not exist: ${target_name}")
    endif()
endfunction()