#pragma once
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

    inline void setLogLevel(size_t levelZeroToFive) {
        if(levelZeroToFive > 5) { throw std::runtime_error("Expected verbosity level integer in [0-5]. Got: " + std::to_string(levelZeroToFive)); }
        auto lvlEnum = static_cast<spdlog::level::level_enum>(levelZeroToFive);
        // Set console settings
        log->set_level(lvlEnum);
    }

    inline void setLogger(const std::string &name, size_t levelZeroToFive = 2, bool timestamp = false) {
        if(spdlog::get(name) == nullptr) {
            log = spdlog::stdout_color_mt(name);
        } else {
            log = spdlog::get(name);
        }

        if(timestamp)
            enableTimestamp();
        else
            disableTimestamp();

        setLogLevel(levelZeroToFive);
    }
    template<typename... Args>
    auto format(Args... args) {
        return fmt::format(std::forward<Args>(args)...);
    }

#else

    class DummyLogger {
        public:
        template<typename... Args>
        void trace(Args... args) const {}
        template<typename... Args>
        void debug(Args... args) const {}
        template<typename... Args>
        void info(Args... args) const {}
        template<typename... Args>
        void warn(Args... args) const {}
        template<typename... Args>
        void error(Args... args) const {}
        template<typename... Args>
        void        critical(Args... args) const {}
        std::string name() const { return ""; }
    };
    inline std::shared_ptr<DummyLogger> log;

    inline void enableTimestamp() {}
    inline void disableTimestamp() {}
    inline void setLogLevel([[maybe_unused]] size_t levelZeroToFive) {}
    inline void setLogger([[maybe_unused]] const std::string &name, [[maybe_unused]] size_t levelZeroToFive = 2, [[maybe_unused]] bool timestamp = false) {}
    template<typename... Args>
    auto format(const std::string &first_param, Args... args) {
        return first_param;
    }

#endif

}
