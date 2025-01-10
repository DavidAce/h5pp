#pragma once

#include <complex>

#if defined(H5PP_USE_QUADMATH)
    #include <quadmath.h>

namespace h5pp {
    using fp128 = __float128;
    using cx128 = __complex128;
}

#elif defined(H5PP_USE_FLOAT128)
    #if __cplusplus < 202302L
        #error "H5PP_USE_FLOAT128: std::float128_t requires C++23"
    #endif
    #if !__has_include(<stdfloat>)
        #error "H5PP_USE_FLOAT128: std::float128_t requires the header <stdfloat>"
    #endif
    #include <stdfloat>
    #if __STDCPP_FLOAT128_T__ != 1
        #error "H5PP_USE_FLOAT128: Could not find std::float128_t macro [__STDCPP_FLOAT128_T__] in header <stdfloat>"
    #endif

namespace h5pp {
    using fp128 = std::float128_t;
    using cx128 = std::complex<fp128>;
}
#endif
