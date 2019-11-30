

#############################
## UNIT TEST
#############################
if(ENABLE_TESTS)
    if(DEPENDENCIES_FOUND)
        enable_testing()
        add_subdirectory(tests/simpleWrite          )
        add_subdirectory(tests/largeWrite           )
        add_subdirectory(tests/overWrite            )
        add_subdirectory(tests/readWrite            )
        add_subdirectory(tests/readWriteAttributes  )
        add_subdirectory(tests/copySwap             )
    else()
        message(STATUS "Dependencies missing, ignoring ENABLE_TESTS=ON")
    endif()
endif()
