#pragma once
#include "h5ppOptional.h"
#if __has_include(<spdlog/spdlog.h>)
    #define H5PP_SPDLOG
    #include <spdlog/sinks/stdout_color_sinks.h>
    #include <spdlog/spdlog.h>
    #if __has_include(<fmt/ranges.h>) && !defined(SPDLOG_FMT_EXTERNAL)
        #define SPDLOG_FMT_EXTERNAL
    #endif
    #if defined(SPDLOG_FMT_EXTERNAL)
        #include <fmt/ranges.h>
    #else
        #include <spdlog/fmt/bundled/ranges.h>
    #endif
#else
    #include <memory>
#endif

namespace h5pp::logger {
#ifdef H5PP_SPDLOG
    inline std::shared_ptr<spdlog::logger> log;

    inline void enableTimestamp() { log->set_pattern("[%Y-%m-%d %H:%M:%S][%n]%^[%=8l]%$ %v"); }
    inline void disableTimestamp() { log->set_pattern("[%n]%^[%=8l]%$ %v"); }

    template<typename levelType>
    inline void setLogLevel(levelType levelZeroToFive) {
        if constexpr(std::is_same_v<levelType, spdlog::level::level_enum>)
            log->set_level(levelZeroToFive);
        else if constexpr(std::is_integral_v<levelType>) {
            if(levelZeroToFive > 5) { throw std::runtime_error("Expected verbosity level integer in [0-5]. Got: " + std::to_string(levelZeroToFive)); }
            auto lvlEnum = static_cast<spdlog::level::level_enum>(levelZeroToFive);
            log->set_level(lvlEnum);
        } else {
            throw std::runtime_error("Given wrong type for spdlog verbosity level");
        }
    }

    inline void setLogger(const std::string &name, std::optional<size_t> levelZeroToFive = std::nullopt, std::optional<bool> timestamp = std::nullopt) {
        if(spdlog::get(name) == nullptr)
            log = spdlog::stdout_color_mt(name);
        else
            log = spdlog::get(name);

        spdlog::level::level_enum lvlEnum;
        if(levelZeroToFive and levelZeroToFive.value() <= 5)
            lvlEnum = static_cast<spdlog::level::level_enum>(levelZeroToFive.value());
        else
            lvlEnum = log->level();

        log->set_level(lvlEnum);

        if(timestamp and timestamp.value())
            enableTimestamp();
        else if(timestamp and not timestamp.value())
            disableTimestamp();
    }

    template<typename... Args>
    auto format(Args... args) {
        return fmt::format(std::forward<Args>(args)...);
    }

#else

    class DummyLogger {
        public:
        template<typename... Args>
        void trace([[maybe_unused]] Args... args) const {}
        template<typename... Args>
        void debug([[maybe_unused]] Args... args) const {}
        template<typename... Args>
        void info([[maybe_unused]] Args... args) const {}
        template<typename... Args>
        void warn([[maybe_unused]] Args... args) const {}
        template<typename... Args>
        void error([[maybe_unused]] Args... args) const {}
        template<typename... Args>
        void        critical([[maybe_unused]] Args... args) const {}
        std::string name() const { return ""; }
    };
    inline std::shared_ptr<DummyLogger> log;

    inline void enableTimestamp() {}
    inline void disableTimestamp() {}
    template<typename levelType>
    inline void setLogLevel([[maybe_unused]] levelType levelZeroToFive) {}
    inline void setLogger([[maybe_unused]] const std::string & name,
                          [[maybe_unused]] std::optional<int>  levelZeroToFive = std::nullopt,
                          [[maybe_unused]] std::optional<bool> timestamp       = std::nullopt) {}
    template<typename... Args>
    auto format(const std::string &first_param, [[maybe_unused]] Args... args) {
        return first_param;
    }

#endif

}
