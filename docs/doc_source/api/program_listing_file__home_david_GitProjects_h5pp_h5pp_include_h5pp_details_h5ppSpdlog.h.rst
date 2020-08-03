
.. _program_listing_file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppSpdlog.h:

Program Listing for File h5ppSpdlog.h
=====================================

|exhale_lsh| :ref:`Return to documentation for file <file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppSpdlog.h>` (``/home/david/GitProjects/h5pp/h5pp/include/h5pp/details/h5ppSpdlog.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #pragma once
   
   #if defined (SPDLOG_FMT_EXTERNAL) && __has_include(<fmt/core.h>) &&  __has_include(<fmt/format.h>) && __has_include(<fmt/ranges.h>) &&  __has_include(<fmt/ostream.h>)
       #include <fmt/core.h>
       #include <fmt/format.h>
       #include <fmt/ostream.h>
       #include <fmt/ranges.h>
   #elif !defined(SPDLOG_FMT_EXTERNAL) && __has_include(<spdlog/fmt/fmt.h>) && __has_include(<spdlog/fmt/bundled/ranges.h>) &&  __has_include(<spdlog/fmt/bundled/ostream.h>)
       #include <spdlog/fmt/bundled/ostream.h>
       #include <spdlog/fmt/bundled/ranges.h>
       #include <spdlog/fmt/fmt.h>
   #elif __has_include(<fmt/core.h>) &&  __has_include(<fmt/format.h>) && __has_include(<fmt/ranges.h>) &&  __has_include(<fmt/ostream.h>)
       // Check if there are already fmt headers installed independently from Spdlog
       // Note that in this case the user hasn't enabled Spdlog for h5pp, so the build hasn't linked any compiled FMT libraries
       // To avoid undefined references we coult opt in to the header-only mode of FMT.
       // Note that this check should be skipped if using conan. Then, SPDLOG_FMT_EXTERNAL is defined
       #ifdef FMT_HEADER_ONLY
           #pragma message "{fmt} has been included as header-only library by defining the compile option FMT_HEADER_ONLY. This may cause a large compile-time overhead"
       #endif
       #include <fmt/core.h>
       #include <fmt/format.h>
       #include <fmt/ostream.h>
       #include <fmt/ranges.h>
   #else
   // In this case there is no fmt so we make our own simple formatter
       #include "h5ppTypeSfinae.h"
       #include <algorithm>
       #include <iostream>
       #include <list>
       #include <memory>
       #include <sstream>
       #include <string>
   #endif
   
   
   
   #if __has_include(<spdlog/spdlog.h>) && __has_include(<spdlog/sinks/stdout_color_sinks.h>)
   #include <spdlog/sinks/stdout_color_sinks.h>
   #include <spdlog/spdlog.h>
   #define H5PP_SPDLOG
   #else
   #pragma message("h5pp warning: could not find header <spdlog/spdlog.h>: A hand-made replacement logger will be used instead. Consider using spdlog for maximum performance")
   #endif
