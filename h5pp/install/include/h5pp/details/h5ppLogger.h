//
// Created by david on 2019-03-03.
//

#ifndef H5PP_LOGGER_H
#define H5PP_LOGGER_H
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace h5pp{
    namespace Logger{
        void set_logger(std::string name, int level_zero_to_six,bool timestamp = false){
            if (level_zero_to_six < 0 or level_zero_to_six > 6) {
                std::cerr << "ERROR: Expected verbosity level integer in [0-6]. Got: " << level_zero_to_six << std::endl;
                exit(1);
            }
            spdlog::level::level_enum lvl_enum = static_cast<spdlog::level::level_enum>(level_zero_to_six);
            set_logger(name, lvl_enum, timestamp);
        }

        void set_logger(std::string name, spdlog::level::level_enum lvl_enum, bool timestamp = false){
            spdlog::set_default_logger(spdlog::stdout_color_mt(name));
            if (timestamp){
                spdlog::set_pattern("[%Y-%m-%d %H:%M:%S][%n]%^[%=8l]%$ %v");
            }else{
                spdlog::set_pattern("[%n]%^[%=8l]%$ %v");
            }
            // Set console settings
            spdlog::set_level(lvl_enum);
            spdlog::debug("Verbosity level: {}", spdlog::level::to_string_view(lvl_enum));

        }
    }
}

#endif //H5PP_LOGGER_H
