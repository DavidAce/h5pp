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
        operator std::string() const;      // Can be copied to vector on-the-fly
        operator std::string_view() const; // Can be copied to vector on-the-fly
        vstr_t() = default;
        vstr_t(const vstr_t &v);
        vstr_t(const char *v);
        vstr_t(std::string_view v);
        vstr_t(vstr_t &&other) noexcept;
        vstr_t              &operator=(const vstr_t &v) noexcept;
        vstr_t              &operator=(std::string_view v);
        vstr_t              &operator=(const hvl_t &v) = delete; /*!< inherently unsafe to allocate an unknown type */
        vstr_t              &operator=(hvl_t &&v)      = delete; /*!< inherently unsafe to allocate an unknown type */
        bool                 operator==(const vstr_t &v) const;
        bool                 operator!=(const vstr_t &v) const;
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
//        void                 erase(std::string::const_iterator itr, size_t pos);
//        void                 erase(size_t index, size_t pos);
        void                 erase(const char *b, const char *e);
        void                 erase(std::string::size_type pos, std::string::size_type n);
        void                 erase(std::string::const_iterator pos);
        void                 erase(std::string::const_iterator begin, std::string::const_iterator end);
        void                 append(const char *v);
        void                 append(const std::string &v);
        void                 append(std::string_view v);
        static hid::h5t      get_h5type();
        ~vstr_t() noexcept;
    };

    vstr_t::operator std::string() const { return {begin(), size()}; }
    vstr_t::operator std::string_view() const { return {begin(), size()}; }

    vstr_t::vstr_t(const vstr_t &v) {
        ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
        strcpy(ptr, v.ptr);
        ptr[v.size()] = '\0';
    }
    vstr_t::vstr_t(const char *v) {
        size_t len = strlen(v);
        ptr        = static_cast<char *>(malloc(len + 1 * sizeof(char)));
        strcpy(ptr, v);
        ptr[len] = '\0';
    }
    vstr_t::vstr_t(std::string_view v) {
        ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
        strcpy(ptr, v.data());
        ptr[v.size()] = '\0';
    }

    vstr_t::vstr_t(vstr_t &&v) noexcept {
        ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
        strcpy(ptr, v.ptr);
        ptr[v.size()] = '\0';
    }
    vstr_t &vstr_t::operator=(const vstr_t &v) noexcept {
        if(this != &v and ptr != v.ptr) {
            free(ptr);
            ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
            strcpy(ptr, v.ptr);
            ptr[v.size()] = '\0';
        }
        return *this;
    }

    vstr_t &vstr_t::operator=(std::string_view v) {
        free(ptr);
        ptr = static_cast<char *>(malloc(v.size() + 1 * sizeof(char)));
        strcpy(ptr, v.data());
        ptr[v.size()] = '\0';
        return *this;
    }

    bool vstr_t::operator==(const vstr_t &v) const {
        if(size() != v.size()) return false;
        return std::equal(begin(), end(), v.begin());
    }

    bool vstr_t::operator!=(const vstr_t &v) const { return !(static_cast<const vstr_t &>(*this) == v); }

    bool vstr_t::operator==(std::string_view v) const {
        if(size() != v.size()) return false;
        return std::equal(begin(), end(), v.begin());
    }

    bool vstr_t::operator!=(std::string_view v) const { return !(static_cast<const vstr_t &>(*this) == v); }

    const char *vstr_t::data() const { return ptr; }

    char *vstr_t::data() { return ptr; }

    const char *vstr_t::c_str() const { return ptr; }

    char *vstr_t::c_str() { return ptr; }

    size_t vstr_t::size() const {
        if(ptr != nullptr)
            return strlen(ptr);
        else
            return 0;
    }

    const char *vstr_t::begin() const { return ptr; }

    const char *vstr_t::end() const { return ptr + size(); }

    char *vstr_t::begin() { return ptr; }

    char *vstr_t::end() { return ptr + size(); }

    void vstr_t::clear() {
        free(ptr);
        ptr = nullptr;
    }
    void vstr_t::resize(size_t newlen) {
        char *newptr = static_cast<char *>(realloc(ptr, newlen + 1 * sizeof(char)));
        if(newptr == nullptr) {
            std::fprintf(stderr, "vstr_t: failed to resize");
            throw;
        }
        ptr = newptr;
    }
    void vstr_t::erase(std::string::size_type pos, std::string::size_type n) { *this = std::string(*this).erase(pos, n); }
    void vstr_t::erase(const char *b, const char *e) {
        std::string::size_type pos = b-ptr;
        std::string::size_type n = e-ptr;
        erase(pos,n);
    }
    void vstr_t::append(const char *v) {
        size_t oldlen = size();
        resize(oldlen + strlen(v));
        strcpy(ptr + oldlen, v);
        ptr[size()] = '\0';
    }
    void vstr_t::append(const std::string &v) { append(v.c_str()); }
    void vstr_t::append(std::string_view v) { append(v.data()); }
    bool vstr_t::empty() const { return size() == 0; }

    hid::h5t vstr_t::get_h5type() {
        hid::h5t h5type = H5Tcopy(H5T_C_S1);
        H5Tset_size(h5type, H5T_VARIABLE);
        H5Tset_strpad(h5type, H5T_STR_NULLTERM);
        H5Tset_cset(h5type, H5T_CSET_UTF8);
        return h5type;
    }

    vstr_t::~vstr_t() noexcept { free(ptr); }

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
            if constexpr(is_iterable_v<T> and has_value_type_v<T>) {
                return is_vstr_v<typename T::value_type> or has_vlen_type_v<typename T::value_type>;
            }
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