cmake_minimum_required(VERSION 3.12)
project(tests CXX)
file(GLOB TESTS "*.cpp")
add_custom_target(all-tests)
foreach (test ${TESTS})
    get_filename_component(test_src ${test} NAME)
    get_filename_component(test_nwe ${test} NAME_WE)
    add_executable(${test_nwe} ${test_src})
    target_link_libraries(${test_nwe} PRIVATE h5pp)
    add_test(NAME ${test_nwe} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND ${test_nwe})
    if (MSVC)
        set_target_properties(${test_nwe} PROPERTIES LINK_FLAGS "/ignore:4099")
    endif()
    add_dependencies(all-tests ${test_nwe})
    list(APPEND test_tgt_list ${test_nwe})
endforeach ()

add_custom_command(
        TARGET all-tests
        POST_BUILD
        COMMENT "Running Tests"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS h5pp deps flags
        COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --output-on-failure)




# Speed up compilation with precompiled headers
if(H5PP_ENABLE_PCH)
    if(COMMAND target_precompile_headers)
        foreach (tgt ${test_tgt_list})
            # Later CMake versions will only include system header paths for pch generation
            target_include_directories(${tgt} SYSTEM INTERFACE ../include)
            if(NOT TARGET ${first_tgt})
                target_precompile_headers(${tgt} PRIVATE <h5pp/h5pp.h>)
                set(first_tgt ${tgt})
            else()
                target_precompile_headers(${tgt} REUSE_FROM ${first_tgt})
            endif()
        endforeach()
    endif()
endif()

# Speed up compilation with ccache
if(H5PP_ENABLE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        foreach (tgt ${test_tgt_list})
            set_target_properties(${tgt} PROPERTIES CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
        endforeach()
        message(STATUS "h5pp tests: using ccache ${CCACHE_PROGRAM}")
        if(H5PP_ENABLE_PCH AND COMMAND target_precompile_headers)
            message(STATUS "h5pp tests: detected ccache + pch: remember to set --> sloppiness = include_file_mtime,pch_defines,time_macros <-- in your ccache.conf")
        endif()
    else()
        message(STATUS "ccache program could not be found")
    endif()
endif()