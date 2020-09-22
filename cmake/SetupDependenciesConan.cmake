cmake_minimum_required(VERSION 3.12)

if(H5PP_DOWNLOAD_METHOD MATCHES "conan")

    if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
        message(STATUS "Detected Conan build info: ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
        include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
        conan_basic_setup(TARGETS)
        ##################################################################
        ### Link all the things!                                       ###
        ##################################################################
        list(APPEND H5PP_TARGETS ${CONAN_TARGETS})
        target_link_libraries(deps INTERFACE ${CONAN_TARGETS})
    else()

        ##################################################################
        ### Install dependencies from conanfile.txt                    ###
        ### This uses conan to get spdlog/eigen3/h5pp/ceres            ###
        ###    eigen/3.3.7@                                            ###
        ###    spdlog/1.7.0                                            ###
        ###    hdf5/1.12.0                                             ###
        ##################################################################

        find_program (
                CONAN_COMMAND
                conan
                HINTS ${H5PP_CONAN_CANDIDATE_PATHS}
                PATH_SUFFIXES bin envs/dmrg/bin
        )

        if(NOT CONAN_COMMAND)
            message(FATAL_ERROR "Could not find conan program executable")
        else()
            message(STATUS "Found conan: ${CONAN_COMMAND}")
        endif()



        # Download automatically, you can also just copy the conan.cmake file
        if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
            message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
            file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
                    "${CMAKE_BINARY_DIR}/conan.cmake")
        endif()

        include(${CMAKE_BINARY_DIR}/conan.cmake)

        if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
            # Let it autodetect libcxx
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
            # There is no libcxx
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            list(APPEND conan_libcxx compiler.libcxx=libstdc++11)
        endif()

        conan_cmake_run(
                CONANFILE conanfile.txt
                CONAN_COMMAND ${CONAN_COMMAND}
                SETTINGS compiler.cppstd=17
                SETTINGS "${conan_libcxx}"
                SETTINGS compiler.cppstd=17
                PROFILE_AUTO ALL
                BUILD_TYPE ${CMAKE_BUILD_TYPE}
                BASIC_SETUP CMAKE_TARGETS
                BUILD missing)
        ##################################################################
        ### Link all the things!                                       ###
        ##################################################################
        list(APPEND H5PP_TARGETS ${CONAN_TARGETS})
        target_link_libraries(deps INTERFACE ${CONAN_TARGETS})
    endif()
endif()
