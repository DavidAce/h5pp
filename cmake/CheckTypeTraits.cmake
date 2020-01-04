function(CheckTypeTraits)
    set(CMAKE_REQUIRED_FLAGS "-std=c++17")
    include(CheckIncludeFileCXX)
    check_include_file_cxx(experimental/type_traits    has_type_traits  )
    if(NOT has_type_traits)
        message(WARNING "\n\
                Missing <experimental/type_traits> header.\n\
                Consider using a newer compiler (GCC 8 or above, Clang 7 or above),\n\
                or checking the compiler flags. If using Clang, try passing the variable \n\
                GCC_TOOLCHAIN=<path> \n\
                where path is the install directory of a recent GCC installation.
                h5pp will try to mimic std::experimental::is_detected
        ")
    endif()
    list(APPEND CMAKE_REQUIRED_INCLUDES ${PROJECT_SOURCE_DIR}/h5pp/include/h5pp/details)
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("
        #include<vector>
        #include \"h5ppStdIsDetected.h\"

        namespace tc{
            template <typename T> using Data_t          = decltype(std::declval<T>().data());
            template <typename T> using hasMember_data  = std::experimental::is_detected<Data_t, T>;
        }
        int main(){
            if constexpr(tc::hasMember_data<std::vector<int>>::value){
                return 0;
            }else{
                return 1;
            }

            return 0;
        }
        " TYPETRAITS_COMPILES)
    if(NOT TYPETRAITS_COMPILES)
        message(FATAL_ERROR "Unable to compile with experimental/type_traits header")
    endif()
endfunction()