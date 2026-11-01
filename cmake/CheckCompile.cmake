include_guard(GLOBAL)

# Keep the original package/target/source arguments; optional arguments let a
# feature check choose its C++ standard, headers, and preprocessor definitions.
function(check_compile pkg tgt file)
    cmake_parse_arguments(PARSE_ARGV 3 CHECK
        "COMPILE_ONLY;RECHECK" "CXX_STANDARD" "INCLUDE_DIRECTORIES;COMPILE_DEFINITIONS")
    if(NOT CHECK_CXX_STANDARD)
        set(CHECK_CXX_STANDARD 20)
    endif()
    if(CHECK_RECHECK)
        unset(check_compile_${pkg} CACHE)
        unset(check_compile_${pkg})
    endif()
    if(NOT check_compile_${pkg})
        if(CHECK_COMPILE_ONLY)
            set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
        endif()
        message(STATUS "Performing Test check_compile_${pkg}")
        try_compile(check_compile_${pkg}
                ${CMAKE_BINARY_DIR}
                ${file}
                CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${CHECK_INCLUDE_DIRECTORIES}"
                COMPILE_DEFINITIONS ${CHECK_COMPILE_DEFINITIONS}
                OUTPUT_VARIABLE compile_out
                LINK_LIBRARIES ${tgt}
                CXX_STANDARD ${CHECK_CXX_STANDARD}
                CXX_STANDARD_REQUIRED ON
                CXX_EXTENSIONS OFF
                )
        if(check_compile_${pkg})
            file(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log "${compile_out}")
            message(STATUS "Performing Test check_compile_${pkg} - Success")
            set(check_compile_${pkg} ${check_compile_${pkg}} CACHE BOOL "")
        else()
            file(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log "${compile_out}")
            message(STATUS "Performing Test check_compile_${pkg} - Failed")
        endif()
        mark_as_advanced(check_compile_${pkg})
    endif()
endfunction()
