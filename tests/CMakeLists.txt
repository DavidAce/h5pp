cmake_minimum_required(VERSION 3.26)
project(h5pp-v2-tests CXX)
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/SetupExecutableDirectory.cmake")
h5pp_setup_executable_directory()

find_package(Catch2 3 REQUIRED)

file(GLOB TESTS CONFIGURE_DEPENDS "*.cpp")
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/output)
add_custom_target(h5pp-v2-test-all)
foreach(test ${TESTS})
    get_filename_component(test_name ${test} NAME_WE)
    add_executable(h5pp-${test_name} ${test})
    if(test_name STREQUAL "test-formatFallback")
        # Test the fallback formatter without h5pp's optional fmt/spdlog definitions.
        target_link_libraries(h5pp-${test_name} PRIVATE Catch2::Catch2)
        target_include_directories(h5pp-${test_name} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../include")
        target_compile_features(h5pp-${test_name} PRIVATE cxx_std_20)
    else()
        target_link_libraries(h5pp-${test_name} PRIVATE h5pp Catch2::Catch2)
    endif()
    target_compile_definitions(h5pp-${test_name} PRIVATE H5PP_TEST_DIR=\"output/\")
    add_test(NAME h5pp-${test_name} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND h5pp-${test_name})
    if(MSVC)
        target_compile_definitions(h5pp-${test_name} PRIVATE _SILENCE_NONFLOATING_COMPLEX_DEPRECATION_WARNING)
        target_link_options(h5pp-${test_name} PRIVATE /ignore:4099)
    endif()
    if(H5PP_ENABLE_COVERAGE)
        target_compile_options(h5pp-${test_name} PRIVATE --coverage)
        target_link_options(h5pp-${test_name} PRIVATE --coverage)
    endif()
    add_dependencies(h5pp-v2-test-all h5pp-${test_name})
endforeach()

# Only this test intentionally calls the deprecated v1 API methods.
target_compile_options(h5pp-test-v1Api PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/wd4996>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-deprecated-declarations>)
