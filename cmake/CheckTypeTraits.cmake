function(CheckTypeTraits)
    set(CMAKE_REQUIRED_FLAGS "-std=c++17")
    list(APPEND CMAKE_REQUIRED_INCLUDES ${PROJECT_SOURCE_DIR}/h5pp/include/h5pp/details)

    include(CheckIncludeFileCXX)
    check_include_file_cxx(experimental/type_traits    has_type_traits  )
    if(NOT has_type_traits)
        message(WARNING "
                Missing <experimental/type_traits> header.\n\
                Check that your compiler has the header.
                For now the build will try to mimic std::experimental::is_detected
                and hope that it works.
        ")
    endif()
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