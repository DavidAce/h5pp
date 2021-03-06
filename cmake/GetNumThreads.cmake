cmake_minimum_required(VERSION 3.15)
function(get_num_threads num_threads)
    cmake_host_system_information(RESULT _host_name   QUERY HOSTNAME)

    if($ENV{CMAKE_BUILD_PARALLEL_LEVEL})
        set(NUM $ENV{CMAKE_BUILD_PARALLEL_LEVEL})
    elseif(CMAKE_BUILD_PARALLEL_LEVEL)
        set(NUM ${CMAKE_BUILD_PARALLEL_LEVEL})
    elseif(DEFINED ENV{MAKEFLAGS})
        string(REGEX MATCH "-j[ ]*[0-9]+|--jobs[ ]*[0-9]+" REGMATCH "$ENV{MAKEFLAGS}")
        string(REGEX MATCH "[0-9]+" NUM "${REGMATCH}")
    elseif(MAKE_THREADS)
        string(REGEX MATCH "-j[ ]*[0-9]+|--jobs[ ]*[0-9]+" REGMATCH "${MAKE_THREADS}")
        string(REGEX MATCH "[0-9]+" NUM "${REGMATCH}")
    else()
        include(ProcessorCount)
        ProcessorCount(NUM)
    endif()
    if(NOT NUM)
        set(NUM 1)
    endif()

    set(${num_threads} ${NUM} PARENT_SCOPE)
    set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} ${NUM})
endfunction()