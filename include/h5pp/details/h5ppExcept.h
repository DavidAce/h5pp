#pragma once

#include "h5ppFormat.h"
#include <cstdio>
#include <H5Epublic.h>
#include <H5version.h>
#include <stdexcept>
namespace h5pp {
    template<typename First, typename... Args>
    std::runtime_error runtime_error(First &&first, Args &&...args) {
        H5Eprint(H5E_DEFAULT, stderr);
        return std::runtime_error("h5pp: " + h5pp::format(h5pp::runtime(std::forward<First>(first)), std::forward<Args>(args)...));
    }
    template<typename First, typename... Args>
    std::logic_error logic_error(First &&first, Args &&...args) {
        H5Eprint(H5E_DEFAULT, stderr);
        return std::logic_error("h5pp: " + h5pp::format(h5pp::runtime(std::forward<First>(first)), std::forward<Args>(args)...));
    }
    template<typename First, typename... Args>
    std::logic_error overflow_error(First &&first, Args &&...args) {
        H5Eprint(H5E_DEFAULT, stderr);
        return std::logic_error("h5pp: " + h5pp::format(h5pp::runtime(std::forward<First>(first)), std::forward<Args>(args)...));
    }
}