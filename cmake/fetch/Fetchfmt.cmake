cmake_minimum_required(VERSION 3.15)

function(Fetchfmt)
    option(FMT_DOC "Generate the doc target." OFF)
    option(FMT_INSTALL "Generate the install target." ON)
    option(FMT_TEST "Generate the test target." OFF)
    option(FMT_FUZZ "Generate the fuzz target." OFF)
    option(FMT_CUDA_TEST "Generate the cuda-test target." OFF)
    include(FetchContent)
    FetchContent_Declare(fetch-fmt
            URL         https://github.com/fmtlib/fmt/archive/8.0.1.tar.gz
            URL_MD5     7d5af964c6633ef90cd6a47be3afe6a0
            )
    FetchContent_MakeAvailable(fetch-fmt)
    find_package(fmt REQUIRED)
endfunction()

Fetchfmt()
