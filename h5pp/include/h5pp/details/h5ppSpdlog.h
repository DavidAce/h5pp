#pragma once

#include "h5ppFormat.h"


#if __has_include(<spdlog/spdlog.h>) && __has_include(<spdlog/sinks/stdout_color_sinks.h>)
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#else
#pragma message("h5pp warning: could not find header <spdlog/spdlog.h>: A hand-made replacement logger will be used instead. Consider using spdlog for maximum performance")
#include <iostream>
#include <string>
#include <memory>
#endif
