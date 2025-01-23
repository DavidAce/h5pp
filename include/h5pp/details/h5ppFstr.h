#pragma once
#include "h5ppFloat128.h"
#include "h5ppHid.h"
#include "h5ppLogger.h"
#include "h5ppTypeSfinae.h"
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <H5Tpublic.h>
#include <string>
#include <string_view>

namespace h5pp::type::flen {
    namespace internal {
        static constexpr bool debug_fstr_t = false;
    }

    template<size_t N>
    struct fstr_t {
        private:
        template<typename T>
        std::string               float_to_string(const T &f);
        char                      ptr[N] = {'\0'};
        [[nodiscard]] const char *begin() const;
        [[nodiscard]] const char *end() const;
        char                     *begin();
        char                     *end();

        template<typename T>
        static constexpr bool is_float() {
            return std::is_floating_point_v<T> || type::sfinae::is_std_complex_v<T> //
#if defined(H5PP_USE_QUADMATH) || defined(H5PP_USE_FLOAT128)
                   || std::is_same_v<T, h5pp::fp128>               //
                   || std::is_same_v<T, h5pp::cx128>               //
                   || std::is_same_v<T, std::complex<h5pp::fp128>> //
#endif
                ;
        }

        public:
        using value_type                 = char[N];
        using data_type [[maybe_unused]] = char;
        operator std::string_view() const; // Can be read as std::string_view on-the-fly
        fstr_t() = default;
        fstr_t(const fstr_t &v);
        fstr_t(const char *v);
        fstr_t(std::string_view v);
        fstr_t(fstr_t &&v) noexcept;
        template<typename T, typename = std::enable_if_t<is_float<T>()>>
        fstr_t(const T &v);
        fstr_t &operator=(const fstr_t &v) noexcept;
        fstr_t &operator=(std::string_view v);
        fstr_t &operator=(const char *v);
        fstr_t &operator=(const hvl_t &v) = delete; /*!< inherently unsafe to allocate an unknown type */
        fstr_t &operator=(hvl_t &&v)      = delete; /*!< inherently unsafe to allocate an unknown type */
        template<typename T, typename = std::enable_if_t<is_float<T>()>>
        fstr_t &operator=(const T &v);
        template<typename T, typename = std::enable_if_t<is_float<T>()>>
        fstr_t                   &operator+=(const T &v);
        bool                      operator==(std::string_view v) const;
        bool                      operator!=(std::string_view v) const;
        char                     *data();
        [[nodiscard]] const char *data() const;
        char                     *c_str();
        [[nodiscard]] const char *c_str() const;
        [[nodiscard]] size_t      size() const;
        void                      clear() noexcept;
        [[nodiscard]] bool        empty() const;
        void                      erase(const char *b, const char *e);
        void                      erase(std::string::size_type pos, std::string::size_type n);
        void                      append(const char *v);
        void                      append(const std::string &v);
        void                      append(std::string_view v);

        template<typename T, typename = std::enable_if_t<is_float<T>()>>
        T to_floating_point() const;

        static hid::h5t get_h5type();
#if !defined(FMT_FORMAT_H_) || !defined(H5PP_USE_FMT)
        friend auto operator<<(std::ostream &os, const fstr_t &v) -> std::ostream & {
            std::copy(v.begin(), v.end(), std::ostream_iterator<char>(os, ", "));
            return os;
        }
#endif

        static constexpr bool is_h5pp_fstr_t() { return true; };
    };
    template<size_t N>
    template<typename T>
    std::string fstr_t<N>::float_to_string(const T &val) {
#if defined(H5PP_USE_QUADMATH) || defined(H5PP_USE_FLOAT128)
        if constexpr(std::is_same_v<T, h5pp::fp128>) {
    #if defined(H5PP_USE_QUADMATH)
            auto n = quadmath_snprintf(nullptr, 0, "%.36Qg", val);
            if(n > -1) {
                auto s = std::string();
                s.resize(static_cast<size_t>(n) + 1);
                quadmath_snprintf(s.data(), static_cast<size_t>(n) + 1, "%.36Qg", val);
                return s.c_str(); // We want a null terminated string
            }
            return {};
    #elif defined(H5PP_USE_FLOAT128)
            constexpr auto bufsize      = std::numeric_limits<h5pp::fp128>::max_digits10 + 10;
            char           buf[bufsize] = {0};
            auto result = std::to_chars(buf, buf + bufsize, val, std::chars_format::fixed, std::numeric_limits<h5pp::fp128>::max_digits10);
            if(result.ec != std::errc()) throw std::runtime_error(std::make_error_code(result.ec).message());
            return buf;
    #endif
        } else
#endif
            if constexpr(sfinae::is_std_complex_v<T>) {
            return h5pp::format("({},{})", float_to_string(std::real(val)), float_to_string(std::imag(val)));
        } else if constexpr(std::is_arithmetic_v<T>) {
            constexpr auto bufsize      = std::numeric_limits<T>::max_digits10 + 10;
            char           buf[bufsize] = {0};
            auto           result =
                std::to_chars(buf, buf + bufsize, val, std::chars_format::fixed, static_cast<int>(std::numeric_limits<T>::max_digits10));
            if(result.ec != std::errc()) throw std::runtime_error(std::make_error_code(result.ec).message());
            return buf;
        } else {
            return {};
        }
    }

    template<size_t N>
    inline fstr_t<N>::operator std::string_view() const {
        return {begin(), size()};
    }

    template<size_t N>
    inline fstr_t<N>::fstr_t(const fstr_t &v) {
        if(v.ptr == nullptr) return;
        strncpy(ptr, v.ptr, N);
        ptr[N - 1] = '\0';
        if constexpr(internal::debug_fstr_t) h5pp::logger::log->info("fstr_t copied into {}: {}", fmt::ptr(ptr), ptr);
    }
    template<size_t N>
    inline fstr_t<N>::fstr_t(const char *v) {
        if(v == nullptr) return;
        strncpy(ptr, v, N);
        ptr[N - 1] = '\0';
        if constexpr(internal::debug_fstr_t) h5pp::logger::log->info("fstr_t copied into {}: {}", fmt::ptr(ptr), ptr);
    }
    template<size_t N>
    inline fstr_t<N>::fstr_t(std::string_view v) {
        if(v.empty()) return;
        strncpy(ptr, v.data(), N);
        ptr[N - 1] = '\0';
        if constexpr(internal::debug_fstr_t) h5pp::logger::log->info("fstr_t copied into {}: {}", fmt::ptr(ptr), ptr);
    }

    template<size_t N>
    inline fstr_t<N>::fstr_t(fstr_t &&v) noexcept {
        if(v.ptr == nullptr) return;
        strncpy(ptr, v.ptr, N);
        ptr[N - 1] = '\0';
        if constexpr(internal::debug_fstr_t) h5pp::logger::log->info("fstr_t copied into {}: {}", fmt::ptr(ptr), ptr);
    }
    template<size_t N>
    template<typename T, typename>
    fstr_t<N>::fstr_t(const T &v) {
        *this = float_to_string(v);
    }
    template<size_t N>
    inline fstr_t<N> &fstr_t<N>::operator=(const fstr_t &v) noexcept {
        if(this != &v and ptr != v.ptr) {
            clear();
            strncpy(ptr, v.ptr, N);
            ptr[N - 1] = '\0';
            if constexpr(internal::debug_fstr_t) h5pp::logger::log->info("fstr_t assigned into {}: {}", fmt::ptr(ptr), ptr);
        }
        return *this;
    }
    template<size_t N>
    inline fstr_t<N> &fstr_t<N>::operator=(std::string_view v) {
        clear();
        strncpy(ptr, v.data(), N);
        ptr[N - 1] = '\0';
        if constexpr(internal::debug_fstr_t) h5pp::logger::log->info("fstr_t assigned into {}: {}", fmt::ptr(ptr), ptr);
        return *this;
    }
    template<size_t N>
    inline fstr_t<N> &fstr_t<N>::operator=(const char *v) {
        return this->operator=(std::string_view(v));
    }
    template<size_t N>
    template<typename T, typename>
    fstr_t<N> &fstr_t<N>::operator=(const T &v) {
        *this = float_to_string(v);
        return *this;
    }

    template<size_t N>
    template<typename T, typename>
    fstr_t<N> &fstr_t<N>::operator+=(const T &v) {
        *this = to_floating_point<T>() + v;
        return *this;
    }
    template<size_t N>
    inline bool fstr_t<N>::operator==(std::string_view v) const {
        if(size() != v.size()) return false;
        return std::equal(begin(), end(), v.begin());
    }

    template<size_t N>
    inline bool fstr_t<N>::operator!=(std::string_view v) const {
        return !(static_cast<const fstr_t &>(*this) == v);
    }

    template<size_t N>
    inline const char *fstr_t<N>::data() const {
        return ptr;
    }

    template<size_t N>
    inline char *fstr_t<N>::data() {
        return ptr;
    }

    template<size_t N>
    inline const char *fstr_t<N>::c_str() const {
        return ptr;
    }

    template<size_t N>
    inline char *fstr_t<N>::c_str() {
        return ptr;
    }

    template<size_t N>
    inline size_t fstr_t<N>::size() const {
        return strnlen(ptr, N);
    }

    template<size_t N>
    inline const char *fstr_t<N>::begin() const {
        return ptr;
    }

    template<size_t N>
    inline const char *fstr_t<N>::end() const {
        return ptr + size();
    }

    template<size_t N>
    inline char *fstr_t<N>::begin() {
        return ptr;
    }

    template<size_t N>
    inline char *fstr_t<N>::end() {
        return ptr + size();
    }

    template<size_t N>
    inline void fstr_t<N>::clear() noexcept {
        memset(ptr, 0, sizeof(ptr));
    }

    template<size_t N>
    inline void fstr_t<N>::erase(std::string::size_type pos, std::string::size_type n) {
        *this = std::string(*this).erase(pos, n);
    }

    template<size_t N>
    inline void fstr_t<N>::erase(const char *b, const char *e) {
        auto pos = static_cast<std::string::size_type>(b - ptr);
        auto n   = static_cast<std::string::size_type>(e - ptr);
        erase(pos, n);
    }

    template<size_t N>
    inline void fstr_t<N>::append(const char *v) {
        if(v == nullptr) return;
        size_t oldlen = size();
        strncpy(ptr + oldlen, v, N - oldlen);
        ptr[N - 1] = '\0';
        if constexpr(internal::debug_fstr_t) h5pp::logger::log->info("fstr_t appended to {} | {} -> {}", fmt::ptr(ptr), v, ptr);
    }

    template<size_t N>
    inline void fstr_t<N>::append(const std::string &v) {
        append(v.c_str());
    }

    template<size_t N>
    inline void fstr_t<N>::append(std::string_view v) {
        append(v.data());
    }

    template<size_t N>
    inline bool fstr_t<N>::empty() const {
        return size() == 0;
    }

    template<size_t N>
    template<typename T, typename>
    T fstr_t<N>::to_floating_point() const {
        if(ptr == nullptr) return 0;
        if(size() == 0) return 0;
        if constexpr(std::is_same_v<std::decay_t<T>, float>) {
            return strtof(c_str(), nullptr);
        } else if constexpr(std::is_same_v<std::decay_t<T>, double>) {
            return strtod(c_str(), nullptr);
        } else if constexpr(std::is_same_v<std::decay_t<T>, long double>) {
            return strtold(c_str(), nullptr);
        }
#if defined(H5PP_USE_QUADMATH)
        else if constexpr(std::is_same_v<std::decay_t<T>, h5pp::fp128>) {
            return strtoflt128(c_str(), nullptr);
        }
#elif defined(H5PP_USE_FLOAT128)
        else if constexpr(std::is_same_v<std::decay_t<T>, h5pp::fp128>) {
            T    rval     = std::numeric_limits<T>::quiet_NaN();
            auto s        = std::string_view(c_str(), N);
            auto pfx      = s.rfind('(', 0) == 0 ? 1ul : 0ul;
            auto rstr     = s.substr(pfx);
            auto [rp, re] = std::from_chars(rstr.begin(), rstr.end(), rval);
        }
#endif
        else {
            // T is probably complex.
            auto strtocomplex = [](std::string_view s, auto strtox) -> T {
                auto  pfx  = s.rfind('(', 0) == 0 ? 1ul : 0ul;
                auto  rstr = s.substr(pfx);
                char *rem;
                auto  real = strtox(rstr.data(), &rem);
                auto  imag = 0.0;
                auto  dlim = std::string_view(rem).find_first_not_of(",+-");
                if(dlim != std::string_view::npos) {
                    auto istr = std::string_view(rem).substr(dlim);
                    imag      = strtox(istr.data(), nullptr);
                }
                return T{real, imag};
            };
            auto complex_from_chars = [](std::string_view s) -> T {
                static_assert(sfinae::is_std_complex_v<T>);
                using V       = typename T::value_type;
                V    rval     = std::numeric_limits<V>::quiet_NaN();
                V    ival     = std::numeric_limits<V>::quiet_NaN();
                auto pfx      = s.rfind('(', 0) == 0 ? 1ul : 0ul;
                auto rstr     = s.substr(pfx);
                auto [rp, re] = std::from_chars(rstr.begin(), rstr.end(), rval);
                //  if(re != std::errc()) {
                //      throw h5pp::runtime_error("h5pp::fstr_t::to_floating_point(): std::from_chars(): {}",
                //                                std::make_error_code(re).message());
                // }

                auto dlim = std::string_view(rp).find_first_not_of(",+-");
                if(dlim != std::string_view::npos) {
                    auto istr     = std::string_view(rp).substr(dlim);
                    auto [ip, ie] = std::from_chars(istr.begin(), istr.end(), ival);
                    // if(re != std::errc()) {
                    //     throw h5pp::runtime_error("h5pp::fstr_t::to_floating_point(): std::from_chars(): {}",
                    //                               std::make_error_code(ie).message());
                    // }
                }
                return T{rval, ival};
            };
            if constexpr(std::is_same_v<std::decay_t<T>, std::complex<float>>) {
                return strtocomplex(c_str(), strtof);
            } else if constexpr(std::is_same_v<std::decay_t<T>, std::complex<double>>) {
                return strtocomplex(c_str(), strtod);
            } else if constexpr(std::is_same_v<std::decay_t<T>, std::complex<long double>>) {
                return strtocomplex(c_str(), strtold);
            }
#if defined(H5PP_USE_QUADMATH)
            else if constexpr(std::is_same_v<std::decay_t<T>, h5pp::cx128> or std::is_same_v<std::decay_t<T>, std::complex<h5pp::fp128>>) {
                return strtocomplex(c_str(), strtoflt128);
            }
#elif defined(H5PP_USE_FLOAT128)
            else if constexpr(std::is_same_v<std::decay_t<T>, h5pp::cx128> or std::is_same_v<std::decay_t<T>, std::complex<h5pp::fp128>>) {
                return complex_from_chars(c_str());
            }
#endif
        }
        return T(0);
    }

    template<size_t N>
    inline hid::h5t fstr_t<N>::get_h5type() {
        hid::h5t h5type = H5Tcopy(H5T_C_S1);
        H5Tset_size(h5type, N);
        H5Tset_strpad(h5type, H5T_STR_NULLTERM);
        H5Tset_cset(h5type, H5T_CSET_UTF8);
        return h5type;
    }
}
namespace h5pp {
    using h5pp::type::flen::fstr_t;
}

namespace h5pp::type::sfinae {

    template<typename T, typename = std::void_t<>>
    struct is_fstr_t : public std::false_type {};
    template<typename T>
    struct is_fstr_t<T, std::void_t<decltype(T::is_h5pp_fstr_t())>> : public std::true_type {};
    template<typename T>
    inline constexpr bool is_fstr_v = is_fstr_t<T>::value;

    template<typename T>
    struct has_fstr_t {
        private:
        static constexpr bool test() {
            if constexpr(is_iterable_v<T> and has_value_type_v<T>) return is_fstr_v<typename T::value_type>;
            return false;
        }

        public:
        static constexpr bool value = test();
    };

    template<typename T>
    inline constexpr bool has_fstr_v = has_fstr_t<T>::value;

    template<typename T>
    inline constexpr bool is_or_has_fstr_v = is_fstr_v<T> or has_fstr_v<T>;
}

#if defined(H5PP_USE_FMT) && defined(FMT_FORMAT_H_) && defined(FMT_VERSION)
    // Add a custom fmt::formatter for h5pp::fstr_t
    #if FMT_VERSION >= 90000
template<size_t N>
struct fmt::formatter<h5pp::fstr_t<N>> : formatter<std::string_view> {
    auto format(const h5pp::fstr_t<N> &f, format_context &ctx) const { return fmt::formatter<string_view>::format(f.c_str(), ctx); }
};
    #else
template<size_t N>
struct fmt::formatter<h5pp::fstr_t<N>> : formatter<std::string_view> {
    auto format(const h5pp::fstr_t<N> &f, format_context &ctx) { return fmt::formatter<string_view>::format(f.c_str(), ctx); }
};
    #endif
#endif