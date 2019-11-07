function(CheckCXX17Compiles REQUIRED_FLAGS REQUIRED_LIBRARIES)
    include(CheckIncludeFileCXX)
    string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS     "${REQUIRED_FLAGS}")
    string(REPLACE ";" " " CMAKE_REQUIRED_LIBRARIES "${REQUIRED_LIBRARIES}")

    check_include_file_cxx(optional      has_optional  )
    check_include_file_cxx(filesystem    has_filesystem  )
    check_include_file_cxx(experimental/filesystem    has_experimental_filesystem  )
    check_include_file_cxx(experimental/type_traits   has_experimental_type_traits )
    if(NOT has_filesystem OR NOT has_experimental_filesystem OR NOT has_experimental_type_traits )
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
    #include <filesystem>
    #include <experimental/filesystem>
    #include <experimental/type_traits>
    #include <optional>
    namespace fs = std::filesystem;

    int main(){
        fs::path testpath = fs::relative(fs::current_path());
        std::optional<int> optint;
        return 0;
    }
    " CXX17_COMPILES)
    if(NOT CXX17_COMPILES)
        message(FATAL_ERROR "Unable to compile with experimental/filesystem headers")
    endif()
endfunction()