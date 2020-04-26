#pragma once


#if defined(H5PP_SPDLOG) && __has_include(<spdlog/fmt/fmt.h>) && __has_include(<spdlog/fmt/bundled/ranges.h>) && !defined(SPDLOG_FMT_EXTERNAL)
    #include <spdlog/fmt/fmt.h>
    #include <spdlog/fmt/bundled/ranges.h>
#elif __has_include(<fmt/core.h>) &&  __has_include(<fmt/format.h>) && __has_include(<fmt/ranges.h>)
    // Check if there are already fmt headers installed independently from Spdlog
    // Note that in this case the user hasn't enabled Spdlog for h5pp, so the build hasn't linked any compiled FMT libraries
    // To avoid undefined references we should opt in to the header-only mode of FMT.
    // Note that this should be skipped if using conan. Then, SPDLOG_FMT_EXTERNAL is defined
    #if !defined(H5PP_SPDLOG)
        #define FMT_HEADER_ONLY
    #endif
    #include <fmt/core.h>
    #include <fmt/format.h>
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
                sstr.seekp(-(long)std::min((size_t)1ul,first.size()), std::ios_base::end);
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