#pragma once

// Check first if there are already fmt headers that SPDLOG is not aware of
#if __has_include(<fmt/format.h>) && __has_include(<fmt/ranges.h>) && !defined(SPDLOG_FMT_EXTERNAL)
    #define SPDLOG_FMT_EXTERNAL
#endif

#if defined(SPDLOG_FMT_EXTERNAL_TEMP)
    #include <fmt/format.h>
    #include <fmt/ranges.h>
#elif __has_include(<spdlog/fmt/bundled/format.h_TEMP>)
    #include <spdlog/fmt/bundled/format.h>
    #include <spdlog/fmt/bundled/ranges.h>
    #define SPDLOG_FMT_INTERNAL
#else
// In this case there is no spdlog or format and we roll our own formatter
    #include "h5ppTypeSfinae.h"
    #include <algorithm>
    #include <iostream>
    #include <list>
    #include <memory>
    #include <string>
#endif

namespace h5pp {

#if defined(SPDLOG_FMT_INTERNAL_TEMP) || defined(SPDLOG_FMT_EXTERNAL)
    template<typename... Args>
    [[nodiscard]] std::string format(Args... args) {
        return fmt::format(std::forward<Args>(args)...);
    }

#else
    namespace formatting {
        template<class T, class... Ts>
        std::list<std::string> convert_to_string_list(T const &first, Ts const &... rest) {
            std::list<std::string> result;
            if constexpr(h5pp::type::sfinae::is_streamable_v<T>){
                std::stringstream sstr;
                sstr << first;
                result.emplace_back(sstr.str());
            }
            else if constexpr(h5pp::type::sfinae::is_iterable_v<T>) {
                std::stringstream sstr;
                sstr << "[";
                for(const auto &elem : first) sstr << elem <<",";
                sstr.seekp(-1, std::ios_base::end);
                sstr << "]";
                result.emplace_back(sstr.str());
            }
            if constexpr(sizeof...(rest) > 0) {
                for(auto &elem : convert_to_string_list(rest...)) result.push_back(elem);
            }
            return result;
        }
    }

    template<typename... Args>
    [[nodiscard]] std::string format(const std::string &fmtstring, [[maybe_unused]] Args... args) {
        size_t brackets_left  = std::count(fmtstring.begin(), fmtstring.end(), '{');
        size_t brackets_right = std::count(fmtstring.begin(), fmtstring.end(), '}');
        if(brackets_left != brackets_right) return std::string("FORMATTING ERROR: GOT STRING: " + fmtstring);
        auto        arglist = formatting::convert_to_string_list(args...);
        std::string result  = fmtstring;
        while(true) {
            if(arglist.empty()) break;
            std::string::size_type start_pos = result.find('{');
            std::string::size_type end_pos   = result.find('}');
            if(start_pos == std::string::npos or end_pos == std::string::npos or start_pos - end_pos == 0) break;
            result.replace(start_pos, end_pos - start_pos + 1, arglist.front());
            arglist.pop_front();
        }
        return result;
    }
#endif
}