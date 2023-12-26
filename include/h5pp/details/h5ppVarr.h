#pragma once
#include "h5ppFormat.h"
#include "h5ppFstr.h"
#include "h5ppTypeCompound.h"
#include "h5ppTypeCustom.h"
#include "h5ppVstr.h"
#include <complex>
#include <cstdlib>
#include <cstring>
#include <H5Tpublic.h>
#include <vector>

namespace h5pp::type::vlen {
    template<typename T>
    struct varr_t {
        private:
        hvl_t vl = {0, nullptr};

        public:
        using value_type = hvl_t;
        using data_type  = T;
        operator std::vector<T>() const; // Can be copied to vector on-the-fly
        varr_t() noexcept;
        varr_t(const varr_t &v);
        varr_t(varr_t &&v) noexcept;
        template<typename V>
        varr_t(const V &v);
        varr_t(std::initializer_list<T> vals);
        varr_t(size_t n, const T &val);

        varr_t<T> &operator=(const varr_t &v);
        varr_t<T> &operator=(varr_t &&v) noexcept;
        template<typename V>
        varr_t<T>           &operator=(const V &v);
        varr_t<T>           &operator=(std::initializer_list<T> v);
        varr_t<T>           &operator=(const hvl_t &v) = delete; /*!< inherently unsafe to allocate an unknown type */
        varr_t<T>           &operator=(hvl_t &&v)      = delete; /*!< inherently unsafe to allocate an unknown type */
        T                   &operator[](size_t n);
        const T             &operator[](size_t n) const;
        T                   &at(size_t n);
        const T             &at(size_t n) const;
        bool                 operator==(const varr_t<T> &v) const;
        bool                 operator!=(const varr_t<T> &v) const;
        bool                 operator==(const std::vector<T> &v) const;
        bool                 operator!=(const std::vector<T> &v) const;
        hvl_t               *vlen_data();
        const hvl_t         *vlen_data() const;
        [[nodiscard]] size_t vlen_size() const;
        [[nodiscard]] size_t size() const;
        [[nodiscard]] size_t length() const;
        const T             *begin() const;
        const T             *end() const;
        T                   *begin();
        T                   *end();
        bool                 empty() const;

        template<typename V,
                 typename = std::enable_if_t<(std::is_constructible_v<T, std::string> || sfinae::is_vstr_v<T> || sfinae::is_fstr_v<T>) &&(
                     std::is_floating_point_v<V>
#if defined(H5PP_USE_QUADMATH)
                     || std::is_same_v<V, __float128> || std::is_same_v<T, __complex128> || std::is_same_v<T, std::complex<__float128>>
#endif
                     )>>
        varr_t<V> to_floating_point() const;

        static hid::h5t get_h5type();
        void            clear() noexcept;
        ~varr_t() noexcept;

#if !defined(FMT_FORMAT_H_) || !defined(H5PP_USE_FMT)
        friend auto operator<<(std::ostream &os, const varr_t &v) -> std::ostream & {
            if(v.empty()) {
                os << "[]";
            } else {
                os << "[";
                std::copy(v.begin(), v.end(), std::ostream_iterator<T>(os, ", "));
                os << "\b\b]";
            }
            return os;
        }
#endif
    };

    template<typename T>
    varr_t<T>::varr_t() noexcept : vl(hvl_t{0, nullptr}) {}

    template<typename T>
    varr_t<T>::varr_t(const varr_t &v) {
        if(v.vl.p == nullptr) return;
        vl.len = v.size();
        vl.p   = calloc(v.size(), sizeof(T));
        std::copy(v.begin(), v.end(), begin());
    }
    template<typename T>
    varr_t<T>::varr_t(varr_t<T> &&v) noexcept {
        vl.len   = v.vl.len;
        vl.p     = v.vl.p;
        v.vl.len = 0;
        v.vl.p   = nullptr;
    }

    template<typename T>
    template<typename V>
    varr_t<T>::varr_t(const V &v) {
        static_assert(sfinae::has_size_v<V>);
        static_assert(sfinae::is_iterable_v<V> or sfinae::has_data_v<V>);
        vl.len = v.size();
        vl.p   = calloc(v.size(), sizeof(T));
        if constexpr(sfinae::is_iterable_v<V>) std::copy(v.begin(), v.end(), begin());
        else if constexpr(sfinae::has_data_v<V>) std::copy(v.data(), v.data() + v.size(), begin());
    }

    template<typename T>
    varr_t<T>::varr_t(std::initializer_list<T> v) {
        vl.len = v.size();
        vl.p   = calloc(v.size(), sizeof(T));
        std::copy(v.begin(), v.end(), begin());
    }

    template<typename T>
    varr_t<T>::operator std::vector<T>() const {
        return std::vector<T>(begin(), end());
    }

    template<typename T>
    varr_t<T>::varr_t(size_t n, const T &val) {
        vl.len = n;
        vl.p   = calloc(n, sizeof(T));
        std::fill(begin(), end(), val);
    }

    template<typename T>
    varr_t<T> &varr_t<T>::operator=(const varr_t &v) {
        if(this == &v) return *this;
        clear();
        if(v.vl.p == nullptr) return *this;
        vl.len = v.size();
        vl.p   = calloc(v.size(), sizeof(T));
        std::copy(v.begin(), v.end(), begin());
        return *this;
    }

    template<typename T>
    varr_t<T> &varr_t<T>::operator=(varr_t<T> &&v) noexcept {
        if(vl.p == v.vl.p) return *this;
        clear();
        vl.len   = v.vl.len;
        vl.p     = v.vl.p;
        v.vl.len = 0;
        v.vl.p   = nullptr;
        return *this;
    }

    template<typename T>
    template<typename V>
    varr_t<T> &varr_t<T>::operator=(const V &v) {
        static_assert(h5pp::type::sfinae::has_size_v<V>);
        static_assert(sfinae::is_iterable_v<V> or sfinae::has_data_v<V>);
        clear();
        vl.len = v.size();
        vl.p   = calloc(v.size(), sizeof(T));
        if constexpr(sfinae::is_iterable_v<V>) std::copy(v.begin(), v.end(), begin());
        else if constexpr(sfinae::has_data_v<V>) std::copy(v.data(), v.data() + v.size(), begin());
        return *this;
    }
    template<typename T>
    varr_t<T> &varr_t<T>::operator=(std::initializer_list<T> v) {
        clear();
        vl.len = v.size();
        vl.p   = calloc(v.size(), sizeof(T));
        std::copy(v.begin(), v.end(), begin());
        return *this;
    }

    template<typename T>
    T &varr_t<T>::operator[](size_t n) {
        return *(static_cast<T *>(vl.p) + n);
    }
    template<typename T>
    const T &varr_t<T>::operator[](size_t n) const {
        return *(static_cast<const T *>(vl.p) + n);
    }
    template<typename T>
    T &varr_t<T>::at(size_t n) {
        if(n < vl.len) return operator[](n);
        throw h5pp::runtime_error("varr_t::at({}) out of bounds (length {})", n, vl.len);
    }
    template<typename T>
    const T &varr_t<T>::at(size_t n) const {
        if(n < vl.len) return operator[](n);
        throw h5pp::runtime_error("varr_t::at({}) out of bounds (length {})", n, vl.len);
    }
    template<typename T>
    bool varr_t<T>::operator==(const varr_t<T> &v) const {
        if(vl.len != v.vl.len) return false;
        return std::equal(begin(), end(), v.begin());
    }
    template<typename T>
    bool varr_t<T>::operator!=(const varr_t<T> &v) const {
        return !(static_cast<const varr_t<T> &>(*this) == v);
    }
    template<typename T>
    bool varr_t<T>::operator==(const std::vector<T> &v) const {
        if(vl.len != v.size()) return false;
        return std::equal(begin(), end(), v.begin());
    }
    template<typename T>
    bool varr_t<T>::operator!=(const std::vector<T> &v) const {
        return !(static_cast<const varr_t<T> &>(*this) == v);
    }

    template<typename T>
    const hvl_t *varr_t<T>::vlen_data() const {
        return &vl;
    }
    template<typename T>
    hvl_t *varr_t<T>::vlen_data() {
        return &vl;
    }
    template<typename T>
    size_t varr_t<T>::vlen_size() const {
        return vl.len;
    }
    template<typename T>
    size_t varr_t<T>::size() const {
        return vl.len;
    }
    template<typename T>
    size_t varr_t<T>::length() const {
        return vl.len;
    }
    template<typename T>
    const T *varr_t<T>::begin() const {
        return static_cast<const T *>(vl.p);
    }
    template<typename T>
    const T *varr_t<T>::end() const {
        return static_cast<const T *>(vl.p) + vl.len;
    }
    template<typename T>
    T *varr_t<T>::begin() {
        return static_cast<T *>(vl.p);
    }
    template<typename T>
    T *varr_t<T>::end() {
        return static_cast<T *>(vl.p) + vl.len;
    }
    template<typename T>
    bool varr_t<T>::empty() const {
        return vl.len == 0 or vl.p == nullptr;
    }
    template<typename T>
    void varr_t<T>::clear() noexcept {
        if(vl.p == nullptr) return;
        if constexpr(std::is_destructible_v<T>)
            for(auto &v : *this) v.~T();

        free(vl.p);
        vl.p   = nullptr;
        vl.len = 0;
    }

    template<typename T>
    varr_t<T>::~varr_t() noexcept {
        if(vl.p == nullptr) return;
        clear();
    }

    template<typename T>
    template<typename V, typename>
    varr_t<V> varr_t<T>::to_floating_point() const {
        auto v = std::vector<V>();
        for(const auto &t : *this) v.emplace_back(t.template to_floating_point<V>());
        return v;
    }

    template<typename T>
    hid::h5t varr_t<T>::get_h5type() {
        /* clang-format off */
        if constexpr      (std::is_same_v<T, short>)                    return H5Tvlen_create(H5T_NATIVE_SHORT);
        else if constexpr (std::is_same_v<T, int>)                      return H5Tvlen_create(H5T_NATIVE_INT);
        else if constexpr (std::is_same_v<T, long>)                     return H5Tvlen_create(H5T_NATIVE_LONG);
        else if constexpr (std::is_same_v<T, long long>)                return H5Tvlen_create(H5T_NATIVE_LLONG);
        else if constexpr (std::is_same_v<T, unsigned short>)           return H5Tvlen_create(H5T_NATIVE_USHORT);
        else if constexpr (std::is_same_v<T, unsigned int>)             return H5Tvlen_create(H5T_NATIVE_UINT);
        else if constexpr (std::is_same_v<T, unsigned long>)            return H5Tvlen_create(H5T_NATIVE_ULONG);
        else if constexpr (std::is_same_v<T, unsigned long long >)      return H5Tvlen_create(H5T_NATIVE_ULLONG);
        else if constexpr (std::is_same_v<T, float>)                    return H5Tvlen_create(H5T_NATIVE_FLOAT);
        else if constexpr (std::is_same_v<T, double>)                   return H5Tvlen_create(H5T_NATIVE_DOUBLE);
        else if constexpr (std::is_same_v<T, long double>)              return H5Tvlen_create(H5T_NATIVE_LDOUBLE);
        else if constexpr (std::is_same_v<T, int8_t>)                   return H5Tvlen_create(H5T_NATIVE_INT8);
        else if constexpr (std::is_same_v<T, int16_t>)                  return H5Tvlen_create(H5T_NATIVE_INT16);
        else if constexpr (std::is_same_v<T, int32_t>)                  return H5Tvlen_create(H5T_NATIVE_INT32);
        else if constexpr (std::is_same_v<T, int64_t>)                  return H5Tvlen_create(H5T_NATIVE_INT64);
        else if constexpr (std::is_same_v<T, uint8_t>)                  return H5Tvlen_create(H5T_NATIVE_UINT8);
        else if constexpr (std::is_same_v<T, uint16_t>)                 return H5Tvlen_create(H5T_NATIVE_UINT16);
        else if constexpr (std::is_same_v<T, uint32_t>)                 return H5Tvlen_create(H5T_NATIVE_UINT32);
        else if constexpr (std::is_same_v<T, uint64_t>)                 return H5Tvlen_create(H5T_NATIVE_UINT64);
        else if constexpr (std::is_same_v<T, bool>)                     return H5Tvlen_create(H5T_NATIVE_UINT8);
        else if constexpr (sfinae::is_vstr_v<T>)                        return H5Tvlen_create(T::get_h5type());
        else if constexpr (sfinae::is_fstr_v<T>)                        return H5Tvlen_create(T::get_h5type());
        #if defined(H5PP_USE_FLOAT128)
        else if constexpr (std::is_same_v<T, __float128>)               return type::custom::H5T_FLOAT<__float128>::h5type();
        #endif
        else if constexpr (type::sfinae::is_std_complex_v<T>)           return H5Tvlen_create(type::compound::H5T_COMPLEX<typename T::value_type>::h5type());
        else if constexpr (type::sfinae::is_Scalar2_v<T>)               return H5Tvlen_create(type::compound::H5T_SCALAR2<type::sfinae::get_Scalar2_t<T>>::h5type());
        else if constexpr (type::sfinae::is_Scalar3_v<T>)               return H5Tvlen_create(type::compound::H5T_SCALAR3<type::sfinae::get_Scalar3_t<T>>::h5type());
        else static_assert(type::sfinae::invalid_type_v<T> and "h5pp could not match the given C++ type to an variable-length HDF5 type.");
        /* clang-format on */
    }

}
namespace h5pp {
    using h5pp::type::vlen::varr_t;
}

namespace h5pp::type::sfinae {
    template<typename T>
    struct is_varr_t : public std::false_type {};
    template<typename T>
    struct is_varr_t<h5pp::varr_t<T>> : public std::true_type {};
    template<typename T>
    inline constexpr bool is_varr_v = is_varr_t<T>::value;

    template<typename T>
    struct has_varr_t {
        private:
        static constexpr bool test() {
            if constexpr(is_iterable_v<T> and has_value_type_v<T>)
                return is_specialization_v<typename T::value_type, h5pp::varr_t> or has_vlen_type_v<typename T::value_type>;
            return has_vlen_type_v<T>;
        }

        public:
        static constexpr bool value = test();
    };

    template<typename T>
    inline constexpr bool has_varr_v = has_varr_t<T>::value;

    template<typename T>
    inline constexpr bool is_or_has_varr_v = is_varr_v<T> or has_varr_v<T>;
}

#if defined(H5PP_USE_FMT) && defined(FMT_FORMAT_H_)

// Add a custom fmt::formatter for h5pp::varr_t
template<typename T, typename Char>
struct fmt::formatter<h5pp::varr_t<T>, Char, typename ::std::enable_if_t<std::is_arithmetic_v<T>>::value>
    : fmt::formatter<std::vector<T>, Char> {
    // template <typename T, typename=std::enable_if_t<std::is_arithmetic_v<T>>::value>
    // struct fmt::formatter<h5pp::varr_t<T>>: formatter<std::vector<T>> {
    auto format(const h5pp::varr_t<T> &v, format_context &ctx) const {
        return fmt::formatter<std::vector<T>>::format(std::vector<T>(v.begin(), v.end()), ctx);
    }
};
#endif