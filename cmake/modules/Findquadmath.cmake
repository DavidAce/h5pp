function(find_quadmath)
    include(CheckTypeSize)
    check_type_size(__float128 FLOAT128_EXISTS BUILTIN_TYPES_ONLY LANGUAGE CXX)
    mark_as_advanced(FLOAT128_EXISTS)
    include(CMakePushCheckState)
    include(CheckCXXSourceCompiles)
    cmake_push_check_state(RESET)
    list(APPEND CMAKE_REQUIRED_LIBRARIES "quadmath")
    check_cxx_source_compiles("
        #include <quadmath.h>
        int main(void){
            __float128 foo = ::sqrtq(123.456);
        }"
        QUADMATH_LINK_WORKS
    )
    cmake_pop_check_state()
    if (NOT QUADMATH_LINK_WORKS AND CMAKE_CXX_COMPILER_ID MATCHES Clang|MSVC)
        message(STATUS "Failed to compile a simple quadmath program: Note that ${CMAKE_CXX_COMPILER_ID} may not support quadmath")
    endif()
endfunction()

find_quadmath()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(quadmath DEFAULT_MSG QUADMATH_LINK_WORKS FLOAT128_EXISTS)

if(quadmath_FOUND)
    if(NOT TARGET quadmath::quadmath)
        add_library(quadmath::quadmath INTERFACE IMPORTED)
        target_link_libraries(quadmath::quadmath INTERFACE quadmath)
        message(DEBUG "Defined target quadmath::quadmath")
    endif()
endif()