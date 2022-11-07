#pragma once

#include "h5ppFormat.h"

#if !defined(H5PP_NO_COMPLEX_FMT) && defined(FMT_FORMAT_H_) && (defined(H5PP_USE_FMT) || defined(H5PP_USE_SPDLOG))
#include <complex>
template<typename T>
struct fmt::formatter<std::complex<T>, char, std::enable_if_t<std::is_arithmetic_v<T>>>
    : fmt::formatter<typename std::complex<T>::value_type> {
    template<typename FormatContext>
    auto format(const std::complex<T> &number, FormatContext &ctx) const {
        return fmt::format_to(ctx.out(), "{0}{1:+}i", number.real(), number.imag());
    }
};
#endif