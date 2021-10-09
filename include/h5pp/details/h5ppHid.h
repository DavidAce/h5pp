#pragma once
#include <hdf5.h>
#include <stdexcept>
#include <string>

namespace h5pp::hid {
    class h5d;
    class h5a;
    class h5o;
    class h5t;
    class h5s;
    class h5f;
    class h5e;
    class h5g;
    class h5p;

    // Base class for all the safe "hid_t" wrapper classes. Zero value is the default for H5P and H5E, so it's ok for them to return it zero
    template<typename hid_h5x, bool zeroValueIsOK = false>
    class hid_base {
        protected:
        hid_t val = 0;

        public:
        virtual ~hid_base() = default;
        hid_base()          = default;
        // Use enable_if to avoid implicit conversion from hid_h5x and still have a non-explicit hid_t constructor
        template<typename T, typename = std::enable_if_t<std::is_same_v<T, hid_t>>>
        hid_base(const T &other) {
            // constructor from hid_t
            if constexpr(zeroValueIsOK) {
                if(other > 0 and not valid(other)) throw std::runtime_error("Given identifier must be valid");
            } else {
                if(not valid(other)) throw std::runtime_error("Given identifier must be valid");
            }
            // Why do we not increment the counter here?
            // The destructor will decrement the reference counter and possibly close this id.
            // Should this object manage
            close(); // Drop current
            val = other;
        }

        hid_base(const hid_base &other) {
            // Copy constructor
            if constexpr(zeroValueIsOK) {
                if(other.val > 0 and not valid(other.val)) throw std::runtime_error("Given identifier must be valid");
            } else {
                if(not valid(other.val)) throw std::runtime_error("Given identifier must be valid");
            }
            close(); // Drop current
            val = other.val;
            if(val > 0) H5Iinc_ref(val); // Increment reference counter of identifier
        }

        hid_base(hid_base &&other) noexcept {
            // Move constructor
            close(); // Drop current
            val = other.val;
            if(val > 0) H5Iinc_ref(val); // Increment reference counter of identifier
        }

        hid_base &operator=(const hid_base &rhs) {
            if(this == &rhs) return *this;
            // Copy assignment
            if constexpr(zeroValueIsOK) {
                if(rhs.val > 0 and not valid(rhs.val)) throw std::runtime_error("Given identifier must be valid");
            } else {
                if(not valid(rhs.val)) throw std::runtime_error("Given identifier must be valid");
            }
            close(); // Drop current
            val = rhs.val;
            if(val > 0) H5Iinc_ref(val); // Increment reference counter of identifier
            return *this;
        }

        hid_base &operator=(hid_base &&rhs) noexcept {
            if(this == &rhs) return *this;
            // Move assignment
            close(); // Drop current
            val = rhs.val;
            if(val > 0) H5Iinc_ref(val); // Increment reference counter of identifier
            return *this;
        }

        template<typename T, typename = std::enable_if_t<std::is_same_v<T, hid_t>>>
        hid_base &operator=(const T &rhs) {
            // Copy assignment from hid_t
            if constexpr(zeroValueIsOK) {
                if(rhs > 0 and not valid(rhs)) throw std::runtime_error("Given identifier must be valid");
            } else {
                if(not valid(rhs)) throw std::runtime_error("Given identifier must be valid");
            }

            close(); // Drop current
            val = rhs;
            return *this;
        }

        void close() {
            if(val == 0 or not valid()) return;
            if(H5Iget_ref(val) > 1)
                H5Idec_ref(val);
            else {
                /* clang-format off */
                if constexpr(std::is_same_v<hid_h5x,h5d> )if(H5Dclose(val) < 0)       {H5Eprint(H5E_DEFAULT, stderr); throw std::runtime_error("Failed to close h5d id " + std::to_string(val));}
                if constexpr(std::is_same_v<hid_h5x,h5a> )if(H5Aclose(val) < 0)       {H5Eprint(H5E_DEFAULT, stderr); throw std::runtime_error("Failed to close h5a id " + std::to_string(val));}
                if constexpr(std::is_same_v<hid_h5x,h5o> )if(H5Oclose(val) < 0)       {H5Eprint(H5E_DEFAULT, stderr); throw std::runtime_error("Failed to close h5o id " + std::to_string(val));}
                if constexpr(std::is_same_v<hid_h5x,h5t> )if(H5Tclose(val) < 0)       {H5Eprint(H5E_DEFAULT, stderr); throw std::runtime_error("Failed to close h5t id " + std::to_string(val));}
                if constexpr(std::is_same_v<hid_h5x,h5s> )if(H5Sclose(val) < 0)       {H5Eprint(H5E_DEFAULT, stderr); throw std::runtime_error("Failed to close h5s id " + std::to_string(val));}
                if constexpr(std::is_same_v<hid_h5x,h5f> )if(H5Fclose(val) < 0)       {H5Eprint(H5E_DEFAULT, stderr); throw std::runtime_error("Failed to close h5f id " + std::to_string(val));}
                if constexpr(std::is_same_v<hid_h5x,h5g> )if(H5Gclose(val) < 0)       {H5Eprint(H5E_DEFAULT, stderr); throw std::runtime_error("Failed to close h5g id " + std::to_string(val));}
                if constexpr(std::is_same_v<hid_h5x,h5p> )if(H5Pclose(val) < 0)       {H5Eprint(H5E_DEFAULT, stderr); throw std::runtime_error("Failed to close h5p id " + std::to_string(val));}
                if constexpr(std::is_same_v<hid_h5x,h5e> )if(H5Eclose_stack(val) < 0) {H5Eprint(H5E_DEFAULT, stderr); throw std::runtime_error("Failed to close h5e id " + std::to_string(val));}
                /* clang-format on */
            }
        }

        [[nodiscard]] virtual std::string tag() const = 0;
        [[nodiscard]] const hid_t        &value() const {
            if constexpr(zeroValueIsOK)
                if(val == 0) return val;
            if(valid())
                return val;
            else {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Tried to return an invalid identifier " + tag() + ":" + std::to_string(val));
            }
        }

        [[nodiscard]] auto refcount() const {
            if constexpr(zeroValueIsOK)
                if(val == 0) return 0;
            if(valid()) {
                auto refc = H5Iget_ref(val);
                if(refc >= 0)
                    return refc;
                else {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Could not get reference count of identifier " + tag() + ":" + std::to_string(val));
                }
            } else {
                return 0;
            }
        }

        [[nodiscard]] std::string pretty_print() { return tag() + ":" + std::to_string(val) + "(" + std::to_string(refcount()) + ")"; }
        [[nodiscard]] std::string safe_print() { return std::to_string(val) + "(" + std::to_string(refcount()) + ")"; }

        [[nodiscard]] bool valid(const hid_t &other) const {
            auto result = H5Iis_valid(other);
            if(result < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to determine validity of identifier");
            }
            return result > 0;
        }
        [[nodiscard]] bool valid() const { return valid(val); }
        [[nodiscard]] bool valid(const hid_h5x &other) const { return other.valid(); }

        // hid_t operators
        [[nodiscard]] virtual bool equal(const hid_t &rhs) const = 0;

        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        bool operator==(const T &rhs) const {
            return equal(rhs);
        }
        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        bool operator!=(const T &rhs) const {
            return not equal(rhs);
        }
        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        bool operator<=(const T &rhs) const {
            return val <= rhs;
        }
        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        bool operator>=(const T &rhs) const {
            return val >= rhs;
        }
        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        bool operator<(const T &rhs) const {
            return val < rhs;
        }
        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        bool operator>(const T &rhs) const {
            return val > rhs;
        }

        // hid_h5x operators
        [[nodiscard]] bool equal(const hid_h5x &rhs) const { return equal(rhs.value()); }
        bool               operator==(const hid_h5x &rhs) const { return equal(rhs); }
        bool               operator!=(const hid_h5x &rhs) const { return not equal(rhs); }
        bool               operator<=(const hid_h5x &rhs) const { return val <= rhs.value(); }
        bool               operator>=(const hid_h5x &rhs) const { return val >= rhs.value(); }
        bool               operator<(const hid_h5x &rhs) const { return val < rhs.value(); }
        bool               operator>(const hid_h5x &rhs) const { return val > rhs.value(); }

        [[nodiscard]] operator hid_t() const { return value(); } // Class can be used as an actual hid_t

        explicit             operator bool() const { return valid() and val > 0; } // Test if set with syntax if(a)
        explicit             operator std::string() const { return tag() + ":" + std::to_string(val); }
        explicit             operator std::string_view() const { return tag() + ":" + std::string_view(val); }
        friend std::ostream &operator<<(std::ostream &os, const hid_h5x &t) { return os << t.tag() << ":" << t.val; }
    };

    // All our safe hid_t wrapper classes
    class h5p final : public hid_base<h5p, true> {
        public:
        using hid_base::hid_base;
        ~h5p() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5p"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return (val > 0 and rhs > 0 and H5Pequal(val, rhs)) or val == rhs; }
    };

    class h5s final : public hid_base<h5s> {
        public:
        using hid_base::hid_base;
        ~h5s() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5s"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
    };

    class h5t final : public hid_base<h5t> {
        public:
        using hid_base::hid_base;
        ~h5t() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5t"; }
        [[nodiscard]] bool equal(const hid_t &rhs) const final { return (valid(val) and valid(rhs) and H5Tequal(val, rhs)) or val == rhs; }
    };

    class h5d final : public hid_base<h5d> {
        public:
        using hid_base::hid_base;
        ~h5d() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5d"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
    };

    class h5g final : public hid_base<h5g> {
        public:
        using hid_base::hid_base;
        ~h5g() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5g"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
    };

    class h5a final : public hid_base<h5a> {
        public:
        using hid_base::hid_base;
        ~h5a() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5a"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
    };

    class h5o final : public hid_base<h5o> {
        public:
        using hid_base::hid_base;
        ~h5o() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5o"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
    };

    class h5f final : public hid_base<h5f> {
        public:
        using hid_base::hid_base;
        ~h5f() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5f"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
    };

    class h5e final : public hid_base<h5e, true> {
        public:
        using hid_base::hid_base;
        ~h5e() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5e"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
    };
}