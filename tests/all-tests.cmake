

#############################
## UNIT TEST
#############################
if(ENABLE_TESTS)
    if(DEPENDENCIES_FOUND)
        enable_testing()
        # Set up a target "all-tests" which will contain all the tests.
        add_subdirectory(tests/simpleWrite          EXCLUDE_FROM_ALL)
        add_subdirectory(tests/largeWrite           EXCLUDE_FROM_ALL)
        add_subdirectory(tests/overWrite            EXCLUDE_FROM_ALL)
        add_subdirectory(tests/readWrite            EXCLUDE_FROM_ALL)
        add_subdirectory(tests/readWriteAttributes  EXCLUDE_FROM_ALL)
        add_subdirectory(tests/copySwap             EXCLUDE_FROM_ALL)

        add_custom_target(all-tests ALL
                DEPENDS
                test-simpleWrite
                test-largeWrite
                test-overWrite
                test-readWrite
                test-readWriteAttributes
                test-copySwap
                )

        if(ENABLE_TESTS_POST_BUILD)
            #Run all tests as soon as the tests have been built
            add_custom_command(TARGET all-tests
                    POST_BUILD
                    COMMENT "Running Tests"
                    DEPENDS h5pp deps flags
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                    COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> --output-on-failures)
        endif()

#
#        add_custom_command(TARGET all_tests
#                POST_BUILD
#                COMMENT "Running Tests:"
#                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#                DEPENDS deps all_tests
#                COMMAND ${CMAKE_CTEST_COMMAND} --build-config $<CONFIG> --output-on-failures)
    else()
        message(STATUS "Dependencies missing, ignoring ENABLE_TESTS=ON")
    endif()
endif()
