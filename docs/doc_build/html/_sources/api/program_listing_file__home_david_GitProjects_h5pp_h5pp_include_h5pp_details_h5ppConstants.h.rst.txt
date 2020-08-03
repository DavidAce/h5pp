
.. _program_listing_file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppConstants.h:

Program Listing for File h5ppConstants.h
========================================

|exhale_lsh| :ref:`Return to documentation for file <file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppConstants.h>` (``/home/david/GitProjects/h5pp/h5pp/include/h5pp/details/h5ppConstants.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   
   #pragma once
   
   namespace h5pp::constants {
       static constexpr unsigned long maxSizeCompact    = 32 * 1024;  // Max size of compact datasets is 32 kb
       static constexpr unsigned long maxSizeContiguous = 512 * 1024; // Max size of contiguous datasets is 512 kb
       static constexpr unsigned long minChunkSize   = 10 * 1024;
       static constexpr unsigned long maxChunkSize   = 1000 * 1024;
   }
