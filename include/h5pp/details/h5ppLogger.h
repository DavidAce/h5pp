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
    inline bool logIf(levelType levelZeroToSix) {
        if constexpr(std::is_integral_v<levelType>)
            return getLogLevel() <= static_cast<size_t>(levelZeroToSix);
        else if constexpr(std::is_same_v<levelType, spdlog::level::level_enum>)
            return static_cast<spdlog::level::level_enum>(getLogLevel()) <= levelZeroToSix;
        else
            static_assert(h5pp::type::sfinae::invalid_type_v<levelType>,
                          "Log level type must be an integral type or spdlog::level::level_enum");
    }

    template<typename LogLevelType>
    inline void setLogLevel(LogLevelType levelZeroToSix) {
        static_assert(type::sfinae::is_any_v<LogLevelType,
                                             spdlog::level::level_enum,
                                             h5pp::LogLevel,
                                             std::optional<spdlog::level::level_enum>,
                                             std::optional<h5pp::LogLevel>,
                                             std::optional<size_t>,
                                             std::optional<int>> or
                      std::is_integral_v<LogLevelType>);

        if constexpr(std::is_same_v<LogLevelType, spdlog::level::level_enum>) log->set_level(levelZeroToSix);
        if constexpr(std::is_same_v<LogLevelType, h5pp::LogLevel> or std::is_integral_v<LogLevelType>)
            log->set_level(static_cast<spdlog::level::level_enum>(Level2Num(levelZeroToSix)));
        if constexpr(type::sfinae::is_any_v<LogLevelType,
                                            std::optional<spdlog::level::level_enum>,
                                            std::optional<h5pp::LogLevel>,
                                            std::optional<size_t>,
                                            std::optional<int>>) {
            if(levelZeroToSix)
                return setLogLevel(levelZeroToSix.value());
            else
                return;
        }
    }
    template<typename LogLevelType>
    inline void setLogger(const std::string &name, LogLevelType levelZeroToSix = LogLevel::info, bool timestamp = false) {
        if(spdlog::get(name) == nullptr)
            log = spdlog::stdout_color_mt(name, spdlog::color_mode::automatic);
        else
            log = spdlog::get(name);
        log->set_pattern("[%n]%^[%=8l]%$ %v"); // Disabled timestamp is the default
        setLogLevel(levelZeroToSix);
        if(timestamp) enableTimestamp();
    }

#else

    class ManualLogger {
        private:
        h5pp::LogLevel logLevel = h5pp::LogLevel::info;

        public:
        std::string logName;
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
        h5pp::LogLevel            level() { return logLevel; }
        void                      set_level(h5pp::LogLevel lvl) { logLevel = lvl; }
    };
    inline std::shared_ptr<ManualLogger> log;

    inline void     enableTimestamp() {}
    inline void     disableTimestamp() {}
    inline LogLevel getLogLevel() {
        if(log != nullptr)
            return log->level();
        else
            return LogLevel::info;
    }

    template<typename LogLevelType>
    inline bool logIf(LogLevelType levelZeroToSix) {
        static_assert(std::is_same_v<LogLevelType, h5pp::LogLevel> or std::is_integral_v<LogLevelType>);
        return getLogLevel() <= levelZeroToSix;
    }
    template<typename LogLevelType>
    inline void setLogLevel(LogLevelType levelZeroToSix) {
        static_assert(
            type::sfinae::
                is_any_v<LogLevelType, h5pp::LogLevel, std::optional<h5pp::LogLevel>, std::optional<size_t>, std::optional<int>> or
            std::is_integral_v<LogLevelType>);

        if constexpr(std::is_same_v<LogLevelType, h5pp::LogLevel> or std::is_integral_v<LogLevelType>)
            if(log != nullptr) log->set_level(Num2Level(levelZeroToSix));
        if constexpr(type::sfinae::is_any_v<LogLevelType, std::optional<h5pp::LogLevel>, std::optional<size_t>, std::optional<int>>) {
            if(levelZeroToSix)
                return setLogLevel(levelZeroToSix.value());
            else
                return;
        }
    }
    template<typename LogLevelType>
    inline void setLogger([[maybe_unused]] const std::string &name_,
                          [[maybe_unused]] LogLevelType       levelZeroToSix = LogLevel::info,
                          [[maybe_unused]] bool               timestamp      = false) {
        log          = std::make_shared<ManualLogger>();
        log->logName = name_;
        setLogLevel(levelZeroToSix);
    }
#endif

}
