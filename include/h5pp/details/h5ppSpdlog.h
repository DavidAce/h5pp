#pragma once

#include "h5ppFormat.h"

#if !defined(SPDLOG_COMPILED_LIB)
    #if !defined(SPDLOG_HEADER_ONLY)
        #define SPDLOG_HEADER_ONLY
    #endif
#endif

#if defined(H5PP_USE_SPDLOG)
    #if __has_include(<spdlog/spdlog.h>) && __has_include(<spdlog/sinks/stdout_color_sinks.h>)
        #include <spdlog/sinks/stdout_color_sinks.h>
        #include <spdlog/spdlog.h>
        #if SPDLOG_VER_MAJOR < 1 || (SPDLOG_VER_MAJOR == 1 && SPDLOG_VER_MINOR < 12)
            #error "h5pp requires spdlog >= 1.12.0"
        #endif
    #else
        #pragma message( \
            "h5pp: could not find header <spdlog/spdlog.h>: A hand-made replacement logger will be used instead. Consider using spdlog for maximum performance")
    #endif
#endif

#if !defined(SPDLOG_H)
    #include <iostream>
    #include <memory>
    #include <string>
#endif
