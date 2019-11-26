function(CheckTypeTraitsCompiles target_name)
    if(TARGET ${target_name})
        get_target_property(TARGET_INC  ${target_name} INTERFACE_INCLUDE_DIRECTORIES)
        get_target_property(TARGET_LIB  ${target_name} INTERFACE_LINK_LIBRARIES)
        get_target_property(TARGET_OPT  ${target_name} INTERFACE_COMPILE_OPTIONS)
        include(CheckIncludeFileCXX)
        check_include_file_cxx(experimental/type_traits    has_type_traits  )
        if(TARGET_INC)
            string(REPLACE ";" " " CMAKE_REQUIRED_INCLUDES  "${TARGET_INC}")
        endif()
        if(TARGET_LIB)
            string(REPLACE ";" " " CMAKE_REQUIRED_LIBRARIES "${TARGET_LIB}")
        endif()
        if(TARGET_OPT)
            string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS     "${TARGET_OPT}")
        endif()


        if(NOT has_type_traits)
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
        #include<experimental/type_traits>
        int main(){
            return 0;
        }
        " TYPETRAITS_COMPILES)
        if(NOT TYPETRAITS_COMPILES)
            message(FATAL_ERROR "Unable to compile with experimental/type_traits header")
        endif()
    else()
        message(WARNING "Target does not exist: ${target_name}")
    endif()
endfunction()