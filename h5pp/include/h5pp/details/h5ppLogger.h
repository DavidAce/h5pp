#pragma once
#include "h5ppFormat.h"
#include "h5ppOptional.h"
#include "h5ppSpdlog.h"
#include "h5ppTypeSfinae.h"

namespace h5pp::logger {
#ifdef SPDLOG_H
    inline std::shared_ptr<spdlog::logger> log;

    inline void enableTimestamp() {
        log->trace("Enabled timestamp");
        log->set_pattern("[%Y-%m-%d %H:%M:%S][%n]%^[%=8l]%$ %v");
    }
    inline void disableTimestamp() {
        log->trace("Disabled timestamp");
        log->set_pattern("[%n]%^[%=8l]%$ %v");
    }

    inline size_t getLogLevel() {
        if(log != nullptr)
            return static_cast<size_t>(log->level());
        else
            return 2;
    }

    template<typename levelType>
    inline bool logIf(levelType levelZeroToFive) {
        if constexpr(std::is_integral_v<levelType>)
            return getLogLevel() <= static_cast<size_t>(levelZeroToFive);
        else if constexpr(std::is_same_v<levelType, spdlog::level::level_enum>)
            return static_cast<spdlog::level::level_enum>(getLogLevel()) <= levelZeroToFive;
        else
            static_assert(h5pp::type::sfinae::invalid_type_v<levelType>,
                          "Log level type must be an integral type or spdlog::level::level_enum");
    }

    template<typename levelType>
    inline void setLogLevel(levelType levelZeroToFive) {
        if constexpr(std::is_same_v<levelType, spdlog::level::level_enum>)
            log->set_level(levelZeroToFive);
        else if constexpr(std::is_integral_v<levelType>) {
            if(levelZeroToFive > 5) {
                throw std::runtime_error("Expected verbosity level integer in [0-5]. Got: " + std::to_string(levelZeroToFive));
            }
            return setLogLevel(static_cast<spdlog::level::level_enum>(levelZeroToFive));
        } else if constexpr(std::is_same_v<levelType, std::optional<spdlog::level::level_enum>> or
                            std::is_same_v<levelType, std::optional<size_t>>) {
            if(levelZeroToFive)
                return setLogLevel(levelZeroToFive.value());
            else
                return;
        } else {
            static_assert(h5pp::type::sfinae::invalid_type_v<levelType>,
                          "Log level type must be an integral type or spdlog::level::level_enum");
        }
    }

    inline void setLogger(const std::string &   name,
                          std::optional<size_t> levelZeroToFive = std::nullopt,
                          std::optional<bool>   timestamp       = std::nullopt) {
        if(spdlog::get(name) == nullptr)
            log = spdlog::stdout_color_mt(name, spdlog::color_mode::automatic);
        else
            log = spdlog::get(name);
        log->set_pattern("[%n]%^[%=8l]%$ %v"); // Disabled timestamp is the default
        setLogLevel(levelZeroToFive);
        if(timestamp and timestamp.value()) enableTimestamp();
    }

#else

    class ManualLogger {
        private:
        public:
        std::string logName;
        size_t      logLevel = 2;
        template<typename... Args>
        void trace(const std::string &fmtstring, Args... args) const {
            if(logLevel <= 0) std::cout << h5pp::format("[{}][{}] " + fmtstring, logName, " trace  ", args...) << '\n';
        }
        template<typename... Args>
        void debug(const std::string &fmtstring, Args... args) const {
            if(logLevel <= 1) std::cout << h5pp::format("[{}][{}] " + fmtstring, logName, " debug  ", args...) << '\n';
        }
        template<typename... Args>
        void info(const std::string &fmtstring, Args... args) const {
            if(logLevel <= 2) std::cout << h5pp::format("[{}][{}] " + fmtstring, logName, " info   ", args...) << '\n';
        }
        template<typename... Args>
        void warn(const std::string &fmtstring, Args... args) const {
            if(logLevel <= 3) std::cout << h5pp::format("[{}][{}] " + fmtstring, logName, " warn   ", args...) << '\n';
        }
        template<typename... Args>
        void error(const std::string &fmtstring, Args... args) const {
            if(logLevel <= 4) std::cout << h5pp::format("[{}][{}] " + fmtstring, logName, " error  ", args...) << '\n';
        }
        template<typename... Args>
        void critical(const std::string &fmtstring, Args... args) const {
            if(logLevel <= 5) std::cout << h5pp::format("[{}][{}] " + fmtstring, logName, "critical", args...) << '\n';
        }
        [[nodiscard]] std::string name() const { return logName; }
    };
    inline std::shared_ptr<ManualLogger> log;

    inline void   enableTimestamp() {}
    inline void   disableTimestamp() {}
    inline size_t getLogLevel() {
        if(log != nullptr)
            return log->logLevel;
        else
            return 2;
    }

    template<typename levelType>
    inline bool logIf(levelType levelZeroToFive) {
        if constexpr(std::is_integral_v<levelType>)
            return getLogLevel() <= static_cast<size_t>(levelZeroToFive);
        else
            static_assert(h5pp::type::sfinae::invalid_type_v<levelType>, "Log level type must be an integral type");
    }

    template<typename levelType>
    inline void setLogLevel([[maybe_unused]] levelType levelZeroToFive) {
        if constexpr(std::is_integral_v<levelType>) {
            if(levelZeroToFive > 5) {
                throw std::runtime_error("Expected verbosity level integer in [0-5]. Got: " + std::to_string(levelZeroToFive));
            }
            //            log->info("Log verbosity level: {}   | trace:0 | debug:1 | info:2 | warn:3 | error:4 | critical:5 |",
            //            levelZeroToFive);
            log->debug("Log verbosity level: {}");
            if(log != nullptr) log->logLevel = levelZeroToFive;
        } else if constexpr(std::is_same_v<levelType, std::optional<size_t>>) {
            if(levelZeroToFive)
                return setLogLevel(levelZeroToFive.value());
            else
                return;
        } else {
            static_assert(h5pp::type::sfinae::invalid_type_v<levelType>,
                          "Log level type must be an integral type or spdlog::level::level_enum");
            throw std::runtime_error("Given wrong type for spdlog verbosity level");
        }
    }

    inline void setLogger([[maybe_unused]] const std::string &   name_,
                          [[maybe_unused]] std::optional<size_t> levelZeroToFive = std::nullopt,
                          [[maybe_unused]] std::optional<bool>   timestamp       = std::nullopt) {
        log          = std::make_shared<ManualLogger>();
        log->logName = name_;
        setLogLevel(levelZeroToFive);
    }
#endif

}
