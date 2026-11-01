include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/CheckCompile.cmake")

function(CheckStdFloat128)
    # Compile h5pp's own header to check both std::float128_t and its complex type.
    set(probe_source "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/CMakeTmp/check-std-float128.cpp")
    file(WRITE "${probe_source}" "
        #include <h5pp/details/h5ppFloat128.h>
        static_assert(sizeof(h5pp::fp128) == 16);
        int main() {
            h5pp::fp128 value = 1;
            h5pp::cx128 number{value, value};
            return number.real() == value ? 0 : 1;
        }")
    # Recheck when compiler flags or the selected standard library change.
    check_compile(StdFloat128 "" "${probe_source}"
        CXX_STANDARD 23
        INCLUDE_DIRECTORIES "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../include"
        COMPILE_DEFINITIONS -DH5PP_USE_FLOAT128
        COMPILE_ONLY RECHECK)
    if(NOT check_compile_StdFloat128)
        message(FATAL_ERROR
            "H5PP_USE_FLOAT128 requires C++23, <stdfloat>, and std::float128_t. "
            "The selected compiler and standard library could not compile h5pp's float128 header. "
            "Disable H5PP_USE_FLOAT128 or use H5PP_USE_QUADMATH with a working libquadmath installation.")
    endif()
endfunction()
