#pragma once
#include "h5ppExcept.h"
#include "h5ppTypeSfinae.h"
#include <type_traits>

namespace h5pp::type {
    template<typename tgt_t, typename src_t>
    constexpr tgt_t safe_cast(src_t src) {
        static_assert(std::is_integral_v<src_t> and std::is_integral_v<tgt_t>, "safe_cast: only integral types are supported");
        if constexpr(std::is_same_v<src_t, tgt_t>) {
            return src;
        } else if constexpr(std::is_integral_v<src_t> and std::is_integral_v<tgt_t>) {
            constexpr src_t srcMax = std::numeric_limits<src_t>::max();
            constexpr tgt_t tgtMax = std::numeric_limits<tgt_t>::max();
            constexpr tgt_t tgtMin = std::numeric_limits<tgt_t>::min();
            constexpr src_t srcMin = std::numeric_limits<src_t>::min();
            using big_t            = std::conditional_t<(sizeof(src_t) > sizeof(tgt_t)), src_t, tgt_t>;
            using ubig_t           = std::make_unsigned_t<big_t>;

            if constexpr(std::is_unsigned_v<src_t> and std::is_unsigned_v<tgt_t>) {
                if constexpr(static_cast<big_t>(srcMax) <= static_cast<big_t>(tgtMax)) return static_cast<tgt_t>(src);
                if(src > static_cast<big_t>(tgtMax)) {
                    throw h5pp::overflow_error("integral overflow during cast: src [{}]={} > tgt [{}]={} (max)",
                                               h5pp::type::sfinae::type_name<src_t>(),
                                               src,
                                               h5pp::type::sfinae::type_name<tgt_t>(),
                                               tgtMax);
                }
            } else if constexpr(std::is_signed_v<src_t> and std::is_signed_v<tgt_t>) {
                if constexpr(static_cast<big_t>(srcMax) <= static_cast<big_t>(tgtMax) and
                             static_cast<big_t>(srcMin) >= static_cast<big_t>(tgtMin)) {
                    return static_cast<tgt_t>(src);
                } else if(src > static_cast<big_t>(tgtMax)) {
                    throw h5pp::overflow_error("integral overflow during cast: src [{}]={} > tgt [{}]={} (max)",
                                               h5pp::type::sfinae::type_name<src_t>(),
                                               src,
                                               h5pp::type::sfinae::type_name<tgt_t>(),
                                               tgtMax);
                } else if(src < static_cast<big_t>(tgtMin)) {
                    throw h5pp::overflow_error("integral sign error during cast: src [{}]={} < tgt [{}]={} (min)",
                                               h5pp::type::sfinae::type_name<src_t>(),
                                               src,
                                               h5pp::type::sfinae::type_name<tgt_t>(),
                                               tgtMin);
                }
            } else if constexpr(std::is_signed_v<src_t> and std::is_unsigned_v<tgt_t>) {
                if(src < 0) {
                    throw h5pp::overflow_error("integral sign error during cast: src [{}]={} < tgt [{}]={} (min)",
                                               h5pp::type::sfinae::type_name<src_t>(),
                                               src,
                                               h5pp::type::sfinae::type_name<tgt_t>(),
                                               tgtMin);
                } else if(static_cast<ubig_t>(src) > static_cast<ubig_t>(tgtMax)) {
                    throw h5pp::overflow_error("integral overflow during cast: src [{}]={} > tgt [{}]={} (max)",
                                               h5pp::type::sfinae::type_name<src_t>(),
                                               src,
                                               h5pp::type::sfinae::type_name<tgt_t>(),
                                               tgtMax);
                }
            } else if constexpr(std::is_unsigned_v<src_t> and std::is_signed_v<tgt_t>) {
                if(static_cast<ubig_t>(src) > static_cast<ubig_t>(tgtMax)) {
                    throw h5pp::overflow_error("integral overflow during cast: src [{}]={} > tgt [{}]={} (max)",
                                               h5pp::type::sfinae::type_name<src_t>(),
                                               src,
                                               h5pp::type::sfinae::type_name<tgt_t>(),
                                               tgtMax);
                }
            }
        }
        return static_cast<tgt_t>(src);
    }
}
