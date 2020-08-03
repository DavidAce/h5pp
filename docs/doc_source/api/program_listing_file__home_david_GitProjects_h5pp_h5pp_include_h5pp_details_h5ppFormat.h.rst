
.. _program_listing_file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppFormat.h:

Program Listing for File h5ppFormat.h
=====================================

|exhale_lsh| :ref:`Return to documentation for file <file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppFormat.h>` (``/home/david/GitProjects/h5pp/h5pp/include/h5pp/details/h5ppFormat.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #pragma once
   
   #include "h5ppSpdlog.h"
   
   namespace h5pp {
   
   #if defined(FMT_FORMAT_H_)
       template<typename... Args>
       [[nodiscard]] std::string format(Args... args) {
           return fmt::format(std::forward<Args>(args)...);
       }
   
   #else
       namespace formatting {
   
           template<class T, class... Ts>
           std::list<std::string> convert_to_string_list(const T &first, const Ts &... rest) {
               std::list<std::string> result;
               if constexpr(h5pp::type::sfinae::is_text_v<T>)
                   result.emplace_back(first);
               else if constexpr(std::is_arithmetic_v<T>)
                   result.emplace_back(std::to_string(first));
               else if constexpr(h5pp::type::sfinae::is_streamable_v<T>) {
                   std::stringstream sstr;
                   sstr << std::boolalpha << first;
                   result.emplace_back(sstr.str());
               } else if constexpr(h5pp::type::sfinae::is_iterable_v<T>) {
                   std::stringstream sstr;
                   sstr << std::boolalpha << "{";
                   for(const auto &elem : first) sstr << elem << ",";
                   //  Laborious casting here to avoid MSVC warnings and errors in std::min()
                   auto max_rewind = static_cast<long>(first.size());
                   auto min_rewind = static_cast<long>(1);
                   long rewind = -1*std::min(max_rewind,min_rewind);
                   sstr.seekp(rewind, std::ios_base::end);
                   sstr << "}";
                   result.emplace_back(sstr.str());
               }
               if constexpr(sizeof...(rest) > 0) {
                   for(auto &elem : convert_to_string_list(rest...)) result.push_back(elem);
               }
               return result;
           }
       }
   
       inline std::string format(const std::string &fmtstring) { return fmtstring; }
   
       template<typename... Args>
       [[nodiscard]] std::string format(const std::string &fmtstring, [[maybe_unused]] Args... args) {
           auto brackets_left  = std::count(fmtstring.begin(), fmtstring.end(), '{');
           auto brackets_right = std::count(fmtstring.begin(), fmtstring.end(), '}');
           if(brackets_left != brackets_right) return std::string("FORMATTING ERROR: GOT STRING: " + fmtstring);
           auto        arglist = formatting::convert_to_string_list(args...);
           std::string result  = fmtstring;
           std::string::size_type curr_pos = 0;
           while(true) {
               if(arglist.empty()) break;
               std::string::size_type start_pos = result.find('{',curr_pos);
               std::string::size_type end_pos   = result.find('}',curr_pos);
               if(start_pos == std::string::npos or end_pos == std::string::npos or start_pos - end_pos == 0) break;
               result.replace(start_pos, end_pos - start_pos + 1, arglist.front());
               curr_pos = start_pos + arglist.front().size();
               arglist.pop_front();
           }
           return result;
       }
   #endif
   }
