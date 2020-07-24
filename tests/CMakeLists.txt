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
endforeach ()
add_custom_command(
        TARGET all-tests
        POST_BUILD
        COMMENT "Running Tests"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS h5pp deps flags
        COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --output-on-failure)
