#pragma once

#include "h5ppFormat.h"

#if defined(H5PP_USE_FMT) && defined(FMT_FORMAT_H_) && defined(FMT_VERSION) && !defined(H5PP_NO_COMPLEX_FMT)
    #if !defined(FMT_USE_COMPLEX) && FMT_VERSION > 90000
        #define FMT_USE_COMPLEX 1
        #include <complex>
        #include <type_traits>

template<typename T, typename Char>
struct fmt::formatter<std::complex<T>, Char> : fmt::formatter<T, Char> {
    private:
    typedef fmt::formatter<T, Char>         base;
    fmt::detail::dynamic_format_specs<Char> specs_;

    public:
    template<typename FormatCtx>
    auto format(const std::complex<T> &x, FormatCtx &ctx) const -> decltype(ctx.out()) {
        base::format(x.real(), ctx);
        if(x.imag() >= 0 && specs_.sign != sign::plus) fmt::format_to(ctx.out(), "+");
        base::format(x.imag(), ctx);
        return fmt::format_to(ctx.out(), "i");
    }
};
    #endif
#endif