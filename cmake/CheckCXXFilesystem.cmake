function(CheckCXXFilesystem)
    cmake_policy(SET CMP0075 NEW)
    set(CMAKE_REQUIRED_FLAGS     "-std=c++17")
    set(CMAKE_REQUIRED_LIBRARIES "-lstdc++fs" )
    include(CheckIncludeFileCXX)
    check_include_file_cxx(filesystem    has_filesystem  )
    check_include_file_cxx(experimental/filesystem    has_experimental_filesystem  )

    if(NOT has_filesystem AND NOT has_experimental_filesystem)
        message(FATAL_ERROR "\n\
                Missing <filesystem> and/or <experimental/filesystem> headers.\n\
                Consider using a newer compiler (GCC 6 or above, Clang 5 or above),\n\
                or checking the compiler flags. If using Clang, pass the variable \n\
                GCC_TOOLCHAIN=<path> \n\
                where path is the install directory of a recent GCC installation.
                Also, don't forget to compile with flags:  [-lstdc++fs -std=c++17].
        ")
    endif()

    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("
        // Include filesystem or experimental/filesystem
        #if __has_include(<filesystem>)
        #include <filesystem>
            namespace h5pp{
                namespace fs =  std::filesystem;
            }
        #elif __has_include(<experimental/filesystem>)
            #include <experimental/filesystem>
            namespace h5pp {
                namespace fs = std::experimental::filesystem;
            }
        #else
            #error Could not find <filesystem> or <experimental/filesystem>
        #endif
        int main(){
            using namespace h5pp;
            fs::path testpath;
            return 0;
        }
        " FILESYSTEM_COMPILES)
    if(NOT FILESYSTEM_COMPILES)
        message(FATAL_ERROR "Unable to compile with filesystem headers")
    endif()
endfunction()