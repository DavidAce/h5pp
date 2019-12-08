function(CheckCXX17FilesystemCompiles)
    set(CMAKE_REQUIRED_FLAGS     "-lstdc++fs -std=c++17")
    set(CMAKE_REQUIRED_LIBRARIES "-lstdc++fs" )

    include(CheckIncludeFileCXX)
    check_include_file_cxx(filesystem    has_filesystem  )
    check_include_file_cxx(experimental/filesystem    has_experimental_filesystem  )

    if(NOT has_filesystem AND NOT has_experimental_filesystem)
        message(FATAL_ERROR "\n\
                Missing one or more C++17 headers.\n\
                Consider using a newer compiler (GCC 7 or above, Clang 7 or above),\n\
                or checking the compiler flags. If using Clang, pass the variable \n\
                GCC_TOOLCHAIN=<path> \n\
                where path is the install directory of a recent GCC installation (version > 8).
                Also, don't forget to compile with flags:  [-lstdc++fs -std=c++17].
        ")
    endif()

    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("
        #if    __cplusplus > 201103L // C++14 to C++17
            #include<experimental/filesystem>
            namespace fs = std::experimental::filesystem;
        #elif  __cplusplus > 201402L // C++17 or newer
            #include<filesystem>
            namespace fs = std::filesystem;
        #endif

        int main(){
            fs::path testpath;
            return 0;
        }
        " FILESYSTEM_COMPILES)
    if(NOT FILESYSTEM_COMPILES)
        message(FATAL_ERROR "Unable to compile with filesystem headers")
    endif()
endfunction()