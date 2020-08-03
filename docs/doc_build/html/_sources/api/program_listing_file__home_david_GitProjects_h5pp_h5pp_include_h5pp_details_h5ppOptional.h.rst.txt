
.. _program_listing_file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppOptional.h:

Program Listing for File h5ppOptional.h
=======================================

|exhale_lsh| :ref:`Return to documentation for file <file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppOptional.h>` (``/home/david/GitProjects/h5pp/h5pp/include/h5pp/details/h5ppOptional.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #pragma once
   // Include optional or experimental/optional
   #if __has_include(<optional>)
       #include <optional>
   #elif __has_include(<experimental/optional>)
       #include <experimental/optional>
   namespace h5pp {
       constexpr const std::experimental::nullopt_t &nullopt = std::experimental::nullopt;
       template<typename T>
       using optional = std::experimental::optional<T>;
   }
   #else
       #error Could not find <optional> or <experimental/optional>
   #endif
