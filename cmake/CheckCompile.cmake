
function(check_compile pkg tgt file)
    if(NOT check_compile_${pkg})
        list(APPEND CMAKE_REQUIRED_LIBRARIES ${tgt})
        message(STATUS "Performing Test check_compile_${pkg}")
        try_compile(check_compile_${pkg}
                ${CMAKE_BINARY_DIR}
                ${file}
                OUTPUT_VARIABLE compile_out
                LINK_LIBRARIES ${tgt}
                CXX_STANDARD 17
                CXX_EXTENSIONS OFF
                )
        if(check_compile_${pkg})
            file(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log "${compile_out}")
            message(STATUS "Performing Test check_compile_${pkg} - Success")
            set(check_compile_${pkg} ${check_compile_${pkg}} CACHE BOOL "")
            mark_as_advanced(check_compile_${pkg})
        else()
            file(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log "${compile_out}")
            message(STATUS "Performing Test check_compile_${pkg} - Failed")
        endif()
    endif()
endfunction()
