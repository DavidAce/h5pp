
.. _program_listing_file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppSpdlog.h:

Program Listing for File h5ppSpdlog.h
=====================================

|exhale_lsh| :ref:`Return to documentation for file <file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppSpdlog.h>` (``/home/david/GitProjects/h5pp/h5pp/include/h5pp/details/h5ppSpdlog.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #pragma once
   
   #include "h5ppFormat.h"
   
   
   #if __has_include(<spdlog/spdlog.h>) && __has_include(<spdlog/sinks/stdout_color_sinks.h>)
   #include <spdlog/sinks/stdout_color_sinks.h>
   #include <spdlog/spdlog.h>
   #else
   #pragma message("h5pp warning: could not find header <spdlog/spdlog.h>: A hand-made replacement logger will be used instead. Consider using spdlog for maximum performance")
   #include <iostream>
   #include <string>
   #include <memory>
   #endif
