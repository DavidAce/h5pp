cmake_minimum_required(VERSION 3.19)
include(FetchContent)

function(Fetchspdlog)

    find_package(fmt)

    if(TARGET fmt::fmt)
        option(SPDLOG_FMT_EXTERNAL "Use external fmt library instead of bundled" ON)
    else()
        option(SPDLOG_FMT_EXTERNAL "Use external fmt library instead of bundled" OFF)
    endif()

    # install options
    option(SPDLOG_INSTALL "Generate the install target" ON)
    option(SPDLOG_FMT_EXTERNAL_HO "Use external fmt header-only library instead of bundled" OFF)
    option(SPDLOG_NO_EXCEPTIONS "Compile with -fno-exceptions. Call abort() on any spdlog exceptions" OFF)
    option(SPDLOG_BUILD_SHARED "Build shared library" ${BUILD_SHARED_LIBS})
    option(SPDLOG_ENABLE_PCH "Build static or shared library using precompiled header to speed up compilation time" ${H5PP_ENABLE_PCH})
    option(SPDLOG_BUILD_EXAMPLE "Build example" OFF)
    option(SPDLOG_BUILD_EXAMPLE_HO "Build header only example" OFF)
    option(SPDLOG_BUILD_TESTS "Build tests" OFF)
    option(SPDLOG_BUILD_TESTS_HO "Build tests using the header only version" OFF)


    FetchContent_Declare(fetch-spdlog
            URL         https://github.com/gabime/spdlog/archive/refs/tags/v1.8.5.tar.gz
            URL_MD5     8755cdbc857794730a022722a66d431a
            )
    FetchContent_MakeAvailable(fetch-spdlog)
    find_package(spdlog REQUIRED)
endfunction()

Fetchspdlog()
