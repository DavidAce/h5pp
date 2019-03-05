//
// Created by david on 2019-03-03.
//

#ifndef H5PP_LOGGER_H
#define H5PP_LOGGER_H
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace h5pp{
    namespace Logger{

    inline void setLogLevel(size_t level_zero_to_six){
        if (level_zero_to_six > 6) {
            throw std::runtime_error( "ERROR: Expected verbosity level integer in [0-6]. Got: " + std::to_string(level_zero_to_six));
        }
        spdlog::level::level_enum lvl_enum = static_cast<spdlog::level::level_enum>(level_zero_to_six);

        // Set console settings
        spdlog::set_level(lvl_enum);
        spdlog::debug("Verbosity level: {}", spdlog::level::to_string_view(lvl_enum));

    }

        inline void setLogger(std::string name, size_t level_zero_to_six, bool timestamp = false){
            spdlog::set_default_logger(spdlog::stdout_color_mt(name));
            if (timestamp){
                spdlog::set_pattern("[%Y-%m-%d %H:%M:%S][%n]%^[%=8l]%$ %v");
            }else{
                spdlog::set_pattern("[%n]%^[%=8l]%$ %v");
            }
            setLogLevel(level_zero_to_six);
        }
    }
}



#endif //H5PP_LOGGER_H
