#pragma once

#include "h5ppFormat.h"
#include <cstdio>
#include <H5Epublic.h>
#include <H5version.h>
#include <stdexcept>
namespace h5pp {
    template<typename... Args>
    std::runtime_error runtime_error(Args&&... args) {
        H5Eprint(H5E_DEFAULT, stderr);
        return std::runtime_error(h5pp::format("h5pp: {}", std::forward<Args>(args)...));
    }
    template<typename... Args>
    std::logic_error logic_error(Args&&... args) {
        H5Eprint(H5E_DEFAULT, stderr);
        return std::logic_error(h5pp::format("h5pp: {}", std::forward<Args>(args)...));
    }
    template<typename... Args>
    std::logic_error overflow_error(Args&&... args) {
        H5Eprint(H5E_DEFAULT, stderr);
        return std::logic_error(h5pp::format("h5pp: {}", std::forward<Args>(args)...));
    }
}