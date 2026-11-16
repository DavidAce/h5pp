include_guard(GLOBAL)

function(CheckFilesystem)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    # Link an executable as well: finding the header alone is not enough.
    set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles([[
        #include <filesystem>
        int main() {
            return std::filesystem::exists(std::filesystem::current_path()) ? 0 : 1;
        }
    ]] HAS_STD_FILESYSTEM)
    if(NOT HAS_STD_FILESYSTEM)
        message(FATAL_ERROR "h5pp requires a C++20 compiler and standard library with working std::filesystem support.")
    endif()
endfunction()
