//
// Created by david on 2019-03-03.
//

#ifndef H5PP_LOGGER_H
#define H5PP_LOGGER_H
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/ranges.h>

namespace h5pp{
    namespace Logger{

        inline std::shared_ptr<spdlog::logger> log;
//        inline std::shared_ptr<spdlog::logger> log = spdlog::default_logger();

    inline void setLogLevel(size_t levelZeroToSix){
        if (levelZeroToSix > 6) {
            throw std::runtime_error( "Expected verbosity level integer in [0-6]. Got: " + std::to_string(levelZeroToSix));
        }
        auto lvlEnum = static_cast<spdlog::level::level_enum>(levelZeroToSix);

        // Set console settings
        log->set_level(lvlEnum);
//        log->trace("Verbosity level: {}", spdlog::level::to_string_view(lvlEnum));

    }

        inline void setLogger(std::string name, size_t levelZeroToSix = 2, bool timestamp = false){
            if(spdlog::get(name) == nullptr){
                log = spdlog::stdout_color_mt(name);
            }else{
                log = spdlog::get(name);
            }

            if (timestamp){
                log->set_pattern("[%Y-%m-%d %H:%M:%S][%n]%^[%=8l]%$ %v");
            }else{
                log->set_pattern("[%n]%^[%=8l]%$ %v");
            }
            setLogLevel(levelZeroToSix);
        }
    }
}



#endif //H5PP_LOGGER_H
