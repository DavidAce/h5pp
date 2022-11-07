#pragma once
#include <cstdlib>
#include <cstring>
#include <H5Tpublic.h>
#include <string>
#include <string_view>

namespace h5pp::type::vlen {
    struct vstr_t {
        private:
        char *ptr = nullptr;

        public:
        using value_type = char *;
        using data_type  = char;
        operator std::string_view() const; // Can be read as std::string_view on-the-fly
        vstr_t() = default;
        vstr_t(const vstr_t &v);
        vstr_t(const char *v);
        vstr_t(std::string_view v);
        vstr_t(vstr_t &&other) noexcept;
        vstr_t              &operator=(const vstr_t &v) noexcept;
        vstr_t              &operator=(std::string_view v);
        vstr_t              &operator=(const hvl_t &v) = delete; /*!< inherently unsafe to allocate an unknown type */
        vstr_t              &operator=(hvl_t &&v)      = delete; /*!< inherently unsafe to allocate an unknown type */
        bool                 operator==(std::string_view v) const;
        bool                 operator!=(std::string_view v) const;
        char                *data();
        const char          *data() const;
        char                *c_str();
        const char          *c_str() const;
        [[nodiscard]] size_t size() const;
        const char          *begin() const;
        const char          *end() const;
        char                *begin();
        char                *end();
        void                 clear();
        bool                 empty() const;
        void                 resize(size_t n);
        void                 erase(const char *b, const char *e);
        void                 erase(std::string::size_type pos, std::string::size_type n);
        void                 append(const char *v);
        void                 append(const std::string &v);
        void                 append(std::string_view v);
        static hid::h5t      get_h5type();
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
        ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
        strcpy(ptr, v.ptr);
        ptr[v.size()] = '\0';
    }
    inline vstr_t::vstr_t(const char *v) {
        size_t len = strlen(v);
        ptr        = static_cast<char *>(malloc(len + 1 * sizeof(char)));
        strcpy(ptr, v);
        ptr[len] = '\0';
    }
    inline vstr_t::vstr_t(std::string_view v) {
        ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
        strcpy(ptr, v.data());
        ptr[v.size()] = '\0';
    }

    inline vstr_t::vstr_t(vstr_t &&v) noexcept {
        ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
        strcpy(ptr, v.ptr);
        ptr[v.size()] = '\0';
    }
    inline vstr_t &vstr_t::operator=(const vstr_t &v) noexcept {
        if(this != &v and ptr != v.ptr) {
            free(ptr);
            ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
            strcpy(ptr, v.ptr);
            ptr[v.size()] = '\0';
        }
        return *this;
    }

    inline vstr_t &vstr_t::operator=(std::string_view v) {
        free(ptr);
        ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
        strcpy(ptr, v.data());
        ptr[v.size()] = '\0';
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

    inline void vstr_t::clear() {
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
    }
    inline void vstr_t::erase(std::string::size_type pos, std::string::size_type n) { *this = std::string(*this).erase(pos, n); }
    inline void vstr_t::erase(const char *b, const char *e) {
        auto pos = static_cast<std::string::size_type>(b - ptr);
        auto n   = static_cast<std::string::size_type>(e - ptr);
        erase(pos, n);
    }
    inline void vstr_t::append(const char *v) {
        size_t oldlen = size();
        resize(oldlen + strlen(v));
        strcpy(ptr + oldlen, v);
        ptr[size()] = '\0';
    }
    inline void vstr_t::append(const std::string &v) { append(v.c_str()); }
    inline void vstr_t::append(std::string_view v) { append(v.data()); }
    inline bool vstr_t::empty() const { return size() == 0; }

    inline hid::h5t vstr_t::get_h5type() {
        hid::h5t h5type = H5Tcopy(H5T_C_S1);
        H5Tset_size(h5type, H5T_VARIABLE);
        H5Tset_strpad(h5type, H5T_STR_NULLTERM);
        H5Tset_cset(h5type, H5T_CSET_UTF8);
        return h5type;
    }

    inline vstr_t::~vstr_t() noexcept { free(ptr); }

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