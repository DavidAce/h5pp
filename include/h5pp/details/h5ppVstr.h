#pragma once
#include "h5ppHid.h"
#include "h5ppLogger.h"
#include "h5ppTypeSfinae.h"
#include <cstdlib>
#include <cstring>
#include <H5Tpublic.h>
#include <string>
#include <string_view>

#if H5PP_USE_QUADMATH
    #include <quadmath.h>
#endif

namespace h5pp::type::vlen {
    namespace internal {
        static constexpr bool debug_vstr_t = false;
    }

    struct vstr_t {
        private:
        char *ptr = nullptr;

        public:
        using value_type = char *;
        using data_type [[maybe_unused]] = char;
        operator std::string_view() const; // Can be read as std::string_view on-the-fly
        vstr_t() = default;
        vstr_t(const vstr_t &v);
        vstr_t(const char *v);
        vstr_t(std::string_view v);
        vstr_t(vstr_t &&v) noexcept;
        template<typename T, typename = std::enable_if_t<!std::is_convertible_v<T, std::string>>>
        vstr_t(const T &v);
        vstr_t &operator=(const vstr_t &v) noexcept;
        vstr_t &operator=(std::string_view v);
        vstr_t &operator=(const hvl_t &v) = delete; /*!< inherently unsafe to allocate an unknown type */
        vstr_t &operator=(hvl_t &&v)      = delete; /*!< inherently unsafe to allocate an unknown type */
        template<typename T, typename = std::enable_if_t<!std::is_convertible_v<T, std::string>>>
        vstr_t &operator=(const T &v);
        template<typename T, typename = std::enable_if_t<!std::is_convertible_v<T, std::string>>>
        vstr_t                   &operator+=(const T &v);
        bool                      operator==(std::string_view v) const;
        bool                      operator!=(std::string_view v) const;
        char                     *data();
        [[nodiscard]] const char *data() const;
        char                     *c_str();
        [[nodiscard]] const char *c_str() const;
        [[nodiscard]] size_t      size() const;
        [[nodiscard]] const char *begin() const;
        [[nodiscard]] const char *end() const;
        char                     *begin();
        char                     *end();
        void                      clear() noexcept;
        [[nodiscard]] bool        empty() const;
        void                      resize(size_t n);
        void                      erase(const char *b, const char *e);
        void                      erase(std::string::size_type pos, std::string::size_type n);
        void                      append(const char *v);
        void                      append(const std::string &v);
        void                      append(std::string_view v);

        template<typename T,
                 typename = std::enable_if_t<std::is_floating_point_v<T> || type::sfinae::is_std_complex_v<T>
#if defined(H5PP_USE_QUADMATH)
                                             || std::is_same_v<T, __float128> || std::is_same_v<T, __complex128> ||
                                             std::is_same_v<T, std::complex<__float128>>
#endif
                                             >>
        T to_floating_point() const;

        static hid::h5t get_h5type();
        ~vstr_t() noexcept;
#if !defined(FMT_FORMAT_H_) || !defined(H5PP_USE_FMT)
        friend auto operator<<(std::ostream &os, const vstr_t &v) -> std::ostream & {
            std::copy(v.begin(), v.end(), std::ostream_iterator<char>(os, ", "));
            return os;
        }
#endif
    };

    inline vstr_t::operator std::string_view() const { return {begin(), size()}; }

    inline vstr_t::vstr_t(const vstr_t &v) {
        if(v.ptr == nullptr) return;
        ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
        strcpy(ptr, v.ptr);
        ptr[v.size()] = '\0';
        if constexpr(internal::debug_vstr_t) h5pp::logger::log->info("vstr_t allocated {}: {}", fmt::ptr(ptr), ptr);
    }
    inline vstr_t::vstr_t(const char *v) {
        if(v == nullptr) return;
        size_t len = strlen(v);
        ptr        = static_cast<char *>(malloc(len + 1 * sizeof(char)));
        strcpy(ptr, v);
        ptr[len] = '\0';
        if constexpr(internal::debug_vstr_t) h5pp::logger::log->info("vstr_t allocated {}: {}", fmt::ptr(ptr), ptr);
    }
    inline vstr_t::vstr_t(std::string_view v) {
        if(v.empty()) return;
        ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
        strcpy(ptr, v.data());
        ptr[v.size()] = '\0';
        if constexpr(internal::debug_vstr_t) h5pp::logger::log->info("vstr_t allocated {}: {}", fmt::ptr(ptr), ptr);
    }

    inline vstr_t::vstr_t(vstr_t &&v) noexcept {
        if(v.ptr == nullptr) return;
        ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
        strcpy(ptr, v.ptr);
        ptr[v.size()] = '\0';
        if constexpr(internal::debug_vstr_t) h5pp::logger::log->info("vstr_t allocated {}: {}", fmt::ptr(ptr), ptr);
    }
    template<typename T, typename>
    vstr_t::vstr_t(const T &v) {
#if defined(H5PP_USE_QUADMATH)
        auto flt128_to_string = [](__float128 &val) -> std::string {
            auto n = quadmath_snprintf(nullptr, 0, "%.36Qg", val);
            if(n > -1) {
                auto s = std::string();
                s.resize(static_cast<size_t>(n) + 1);
                quadmath_snprintf(s.data(), static_cast<size_t>(n) + 1, "%.36Qg", val);
                return s;
            }
            return {};
        };
        if constexpr(std::is_same_v<std::decay_t<T>, __float128>) {
            *this = flt128_to_string(v);
        } else if constexpr(std::is_same_v<std::decay_t<T>, std::complex<__float128>>) {
            *this = h5pp::format("({},{})", flt128_to_string(v.real()), flt128_to_string(v.imag()));
        } else if constexpr(std::is_same_v<std::decay_t<T>, __complex128>) {
            *this = h5pp::format("({},{})", flt128_to_string(crealq(v)), flt128_to_string(cimagq(v)));
        } else
#endif
        {
            *this = h5pp::format("{}", v);
        }
    }

    inline vstr_t &vstr_t::operator=(const vstr_t &v) noexcept {
        if(this != &v and ptr != v.ptr) {
            clear();
            ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
            strcpy(ptr, v.ptr);
            ptr[v.size()] = '\0';
            if constexpr(internal::debug_vstr_t) h5pp::logger::log->info("vstr_t allocated {}: {}", fmt::ptr(ptr), ptr);
        }
        return *this;
    }

    inline vstr_t &vstr_t::operator=(std::string_view v) {
        clear();
        ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
        strcpy(ptr, v.data());
        ptr[v.size()] = '\0';
        if constexpr(internal::debug_vstr_t) h5pp::logger::log->info("vstr_t allocated {}: {}", fmt::ptr(ptr), ptr);
        return *this;
    }
    template<typename T, typename>
    vstr_t &vstr_t::operator=(const T &v) {
#if defined(H5PP_USE_QUADMATH)
        auto flt128_to_string = [](const __float128 &val) -> std::string {
            auto n = quadmath_snprintf(nullptr, 0, "%.36Qg", val);
            if(n > -1) {
                auto s = std::string();
                s.resize(static_cast<size_t>(n) + 1);
                quadmath_snprintf(s.data(), static_cast<size_t>(n) + 1, "%.36Qg", val);
                return s.c_str(); // We want a null terminated string
            }
            return {};
        };
        if constexpr(std::is_same_v<std::decay_t<T>, __float128>) {
            *this = flt128_to_string(v);
        } else if constexpr(std::is_same_v<std::decay_t<T>, std::complex<__float128>>) {
            *this = h5pp::format("({},{})", flt128_to_string(v.real()), flt128_to_string(v.imag()));
        } else if constexpr(std::is_same_v<std::decay_t<T>, __complex128>) {
            *this = h5pp::format("({},{})", flt128_to_string(crealq(v)), flt128_to_string(cimagq(v)));
        } else
#endif
        {
            *this = h5pp::format("{}", v);
        }
        return *this;
    }
    template<typename T, typename>
    vstr_t &vstr_t::operator+=(const T &v) {
        *this = to_floating_point<T>() + v;
        return *this;
    }
    inline bool vstr_t::operator==(std::string_view v) const {
        if(size() != v.size()) return false;
        return std::equal(begin(), end(), v.begin());
    }

    inline bool vstr_t::operator!=(std::string_view v) const { return !(static_cast<const vstr_t &>(*this) == v); }

    inline const char *vstr_t::data() const { return ptr; }

    inline char *vstr_t::data() { return ptr; }

    inline const char *vstr_t::c_str() const { return ptr; }

    inline char *vstr_t::c_str() { return ptr; }

    inline size_t vstr_t::size() const {
        if(ptr != nullptr) return strlen(ptr);
        else return 0;
    }

    inline const char *vstr_t::begin() const { return ptr; }

    inline const char *vstr_t::end() const { return ptr + size(); }

    inline char *vstr_t::begin() { return ptr; }

    inline char *vstr_t::end() { return ptr + size(); }

    inline void vstr_t::clear() noexcept {
        if constexpr(internal::debug_vstr_t) {
            if(ptr != nullptr) h5pp::logger::log->info("vstr_t clearing {} | {}", fmt::ptr(ptr), ptr);
        }
        free(ptr);
        ptr = nullptr;
    }
    inline void vstr_t::resize(size_t newlen) {
        char *newptr = static_cast<char *>(realloc(ptr, newlen + 1 * sizeof(char)));
        if(newptr == nullptr) {
            std::fprintf(stderr, "vstr_t: failed to resize");
            throw;
        }
        ptr = newptr;
        if constexpr(internal::debug_vstr_t) h5pp::logger::log->info("vstr_t realloc {} | {}", fmt::ptr(ptr), ptr);
    }
    inline void vstr_t::erase(std::string::size_type pos, std::string::size_type n) { *this = std::string(*this).erase(pos, n); }
    inline void vstr_t::erase(const char *b, const char *e) {
        auto pos = static_cast<std::string::size_type>(b - ptr);
        auto n   = static_cast<std::string::size_type>(e - ptr);
        erase(pos, n);
    }
    inline void vstr_t::append(const char *v) {
        if(v == nullptr) return;
        size_t oldlen = size();
        resize(oldlen + strlen(v));
        strcpy(ptr + oldlen, v);
        ptr[size()] = '\0';
        if constexpr(internal::debug_vstr_t) h5pp::logger::log->info("vstr_t appended to {} | {} -> {}", fmt::ptr(ptr), v, ptr);
    }
    inline void vstr_t::append(const std::string &v) { append(v.c_str()); }
    inline void vstr_t::append(std::string_view v) { append(v.data()); }
    inline bool vstr_t::empty() const { return size() == 0; }

    template<typename T, typename>
    T vstr_t::to_floating_point() const {
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
        else if constexpr(std::is_same_v<std::decay_t<T>, __float128>) {
            return strtoflt128(c_str(), nullptr);
        }
#endif
        else {
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
            if constexpr(std::is_same_v<std::decay_t<T>, std::complex<float>>) {
                return strtocomplex(c_str(), strtof);
            } else if constexpr(std::is_same_v<std::decay_t<T>, std::complex<double>>) {
                return strtocomplex(c_str(), strtod);
            } else if constexpr(std::is_same_v<std::decay_t<T>, std::complex<long double>>) {
                return strtocomplex(c_str(), strtold);
            }
#if defined(H5PP_USE_QUADMATH)
            else if constexpr(std::is_same_v<std::decay_t<T>, __complex128> or std::is_same_v<std::decay_t<T>, std::complex<__float128>>) {
                return strtocomplex(c_str(), strtoflt128);
            }
#endif
        }
        return 0;
    }

    inline hid::h5t vstr_t::get_h5type() {
        hid::h5t h5type = H5Tcopy(H5T_C_S1);
        H5Tset_size(h5type, H5T_VARIABLE);
        H5Tset_strpad(h5type, H5T_STR_NULLTERM);
        H5Tset_cset(h5type, H5T_CSET_UTF8);
        return h5type;
    }

    inline vstr_t::~vstr_t() noexcept { clear(); }

}
namespace h5pp {
    using h5pp::type::vlen::vstr_t;
}

namespace h5pp::type::sfinae {

    template<typename T>
    constexpr bool is_vstr_v = std::is_same_v<T, h5pp::vstr_t>;
    template<typename T>
    struct has_vstr_t {
        private:
        static constexpr bool test() {
            if constexpr(is_iterable_v<T> and has_value_type_v<T>)
                return is_vstr_v<typename T::value_type> or has_vlen_type_v<typename T::value_type>;
            return has_vlen_type_v<T>;
        }

        public:
        static constexpr bool value = test();
    };

    template<typename T>
    inline constexpr bool has_vstr_v = has_vstr_t<T>::value;

    template<typename T>
    inline constexpr bool is_or_has_vstr_v = is_vstr_v<T> or has_vstr_v<T>;
}


#if defined(H5PP_USE_FMT) && defined(FMT_FORMAT_H_) && defined(FMT_VERSION)
// Add a custom fmt::formatter for h5pp::vstr_t
#if FMT_VERSION >= 90000
template <> struct fmt::formatter<h5pp::vstr_t>: formatter<std::string_view>  {
  auto format(const h5pp::vstr_t &s, format_context& ctx) const{
        return fmt::formatter<string_view>::format(s.c_str(), ctx);
  }
};
#else
template <> struct fmt::formatter<const h5pp::vstr_t>: formatter<std::string_view> {
  auto format(const h5pp::vstr_t &s, format_context& ctx){
        return fmt::formatter<string_view>::format(s.c_str(), ctx);
  }
};
#endif
#endif