
.. _program_listing_file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppFilesystem.h:

Program Listing for File h5ppFilesystem.h
=========================================

|exhale_lsh| :ref:`Return to documentation for file <file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppFilesystem.h>` (``/home/david/GitProjects/h5pp/h5pp/include/h5pp/details/h5ppFilesystem.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #pragma once
   #if __has_include(<ghc/filesystem.hpp>)
   // Last resort
       #include <ghc/filesystem.hpp>
   namespace h5pp {
       namespace fs = ghc::filesystem;
   }
   #elif __has_include(<filesystem>)
       #include <filesystem>
       #include <utility>
   namespace h5pp {
       namespace fs = std::filesystem;
   }
   #elif __has_include(<experimental/filesystem>)
       #include <experimental/filesystem>
   namespace h5pp {
       namespace fs = std::experimental::filesystem;
   }
   #else
       #error Could not find includes: <filesystem> or <experimental/filesystem> <ghc/filesystem>
   #endif
