

#############################
## UNIT TEST
#############################
if(ENABLE_TESTS)
    if(DEPENDENCIES_FOUND)
        enable_testing()
        add_subdirectory(tests/simpleWrite          EXCLUDE_FROM_ALL)
        add_subdirectory(tests/largeWrite           EXCLUDE_FROM_ALL)
        add_subdirectory(tests/overWrite            EXCLUDE_FROM_ALL)
        add_subdirectory(tests/readWrite            EXCLUDE_FROM_ALL)
        add_subdirectory(tests/readWriteAttributes  EXCLUDE_FROM_ALL)
        add_subdirectory(tests/copySwap             EXCLUDE_FROM_ALL)

        add_custom_target(all_tests ALL
                DEPENDS simpleWrite
                DEPENDS largeWrite
                DEPENDS overWrite
                DEPENDS readWrite
                DEPENDS readWriteAttributes
                DEPENDS copySwap
                )

        add_custom_command(TARGET all_tests
                POST_BUILD
                COMMENT "Running Tests:"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS deps all_tests
                COMMAND ${CMAKE_CTEST_COMMAND} --build-config $<CONFIG> --output-on-failures)
    else()
        message(STATUS "Dependencies missing, ignoring ENABLE_TESTS=ON")
    endif()
endif()
