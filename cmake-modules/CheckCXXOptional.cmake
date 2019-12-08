function(CheckCXXOptional)
    include(CheckIncludeFileCXX)
    check_include_file_cxx(optional    has_optional)
    check_include_file_cxx(experimental/optional has_experimental_optional )

    if(NOT has_optional AND NOT has_experimental_optional)
        message(FATAL_ERROR "\n\
                Missing <optional> and/or <experimental/optional> headers.\n\
                Consider using a newer compiler (GCC 6 or above, Clang 5 or above),\n\
                or checking the compiler flags. If using Clang, pass the variable \n\
                GCC_TOOLCHAIN=<path> \n\
                where path is the install directory of a recent GCC.
        ")
    endif()

    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("
        #if __cplusplus > 201103L // C++14 to C++17
        #include <experimental/optional>
        namespace std {
            constexpr std::experimental::nullopt_t nullopt = std::experimental::nullopt ;
            template<typename T> using optional = std::experimental::optional<T>;
        }
        #elif  __cplusplus > 201402L // C++17 or newer
            #include <optional>
        #endif

        int main(){
            std::optional<int> optVar = std::nullopt;
            return 0;
        }
        " OPTIONAL_COMPILES)
    if(NOT OPTIONAL_COMPILES)
        message(FATAL_ERROR "Unable to compile with <optional> or <experimental/optional> headers")
    endif()
endfunction()