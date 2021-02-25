#pragma once

#if defined(SPDLOG_FMT_EXTERNAL) &&                                                                                                        \
    __has_include(<fmt/core.h>) &&  __has_include(<fmt/format.h>) && __has_include(<fmt/ranges.h>) &&  __has_include(<fmt/ostream.h>)
    #include <fmt/core.h>
    #include <fmt/format.h>
    #include <fmt/ostream.h>
    #include <fmt/ranges.h>
#elif !defined(SPDLOG_FMT_EXTERNAL) &&                                                                                                     \
    __has_include(<spdlog/fmt/fmt.h>) && __has_include(<spdlog/fmt/bundled/ranges.h>) &&  __has_include(<spdlog/fmt/bundled/ostream.h>)
    #include <spdlog/fmt/bundled/ostream.h>
    #include <spdlog/fmt/bundled/ranges.h>
    #include <spdlog/fmt/fmt.h>
#elif __has_include(<fmt/core.h>) &&  __has_include(<fmt/format.h>) && __has_include(<fmt/ranges.h>) &&  __has_include(<fmt/ostream.h>)
// Check if there are already fmt headers installed independently from Spdlog
// Note that in this case the user hasn't enabled Spdlog for h5pp, so the build hasn't linked any compiled FMT libraries
// To avoid undefined references we coult opt in to the header-only mode of FMT.
// Note that this check should be skipped if using conan. Then, SPDLOG_FMT_EXTERNAL is defined
    #ifdef FMT_HEADER_ONLY
        #pragma message                                                                                                                    \
            "{fmt} has been included as header-only library by defining the compile option FMT_HEADER_ONLY. This may cause a large compile-time overhead"
    #endif
    #include <fmt/core.h>
    #include <fmt/format.h>
    #include <fmt/ostream.h>
    #include <fmt/ranges.h>
#else
    // In this case there is no fmt so we make our own simple formatter
    #pragma message                                                                                                                       \
        "h5pp warning: could not find header fmt library headers <fmt/core.h> or <spdlog/fmt/fmt.h>: A hand-made formatter will be used instead. Consider using the fmt library for maximum performance"

#endif

#if defined(FMT_FORMAT_H_)

namespace h5pp {
    template<typename... Args>
    [[nodiscard]] std::string format(Args... args) {
        return fmt::format(std::forward<Args>(args)...);
    }
    template<typename... Args>
    void print(Args... args) {
        fmt::print(std::forward<Args>(args)...);
    }

#else

    // In this case there is no fmt so we make our own simple formatter
    #include "h5ppTypeSfinae.h"
    #include <algorithm>
    #include <iostream>
    #include <list>
    #include <memory>
    #include <sstream>
    #include <string>

namespace h5pp {
    namespace formatting {

        template<class T, class... Ts>
        std::list<std::string> convert_to_string_list(const T &first, const Ts &...rest) {
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
                long rewind     = -1 * std::min(max_rewind, min_rewind);
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
        auto                   arglist  = formatting::convert_to_string_list(args...);
        std::string            result   = fmtstring;
        std::string::size_type curr_pos = 0;
        while(true) {
            if(arglist.empty()) break;
            std::string::size_type start_pos = result.find('{', curr_pos);
            std::string::size_type end_pos   = result.find('}', curr_pos);
            if(start_pos == std::string::npos or end_pos == std::string::npos or start_pos - end_pos == 0) break;
            result.replace(start_pos, end_pos - start_pos + 1, arglist.front());
            curr_pos = start_pos + arglist.front().size();
            arglist.pop_front();
        }
        return result;
    }

    template<typename... Args>
    void print(Args... args) {
        std::printf("%s", h5pp::format(std::forward<Args>(args)...).c_str());
    }

#endif
}