#pragma once
#include "h5ppError.h"
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

    // Base class for all the safe "hid_t" wrapper classes. Zero value is the default for H5P and H5E, so it's ok for them to return zero
    template<typename hid_h5x, bool zeroValueIsOK = false>
    class hid_base {
        protected:
        hid_t val = 0;

        public:
        ~hid_base() { close(); }
        hid_base() = default;
        // Use enable_if to avoid implicit conversion from hid_h5x and still have a non-explicit hid_t constructor
        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        hid_base(const T &other) {
            // constructor from hid_t
            if constexpr(zeroValueIsOK) {
                if(other > 0 and not valid(other)) throw h5pp::runtime_error("Given identifier must be valid");
            } else {
                if(not valid(other)) throw h5pp::runtime_error("Given identifier must be valid");
            }
            close(); // Drop current
            val = other;
        }

        hid_base(const hid_base &other) {
            // Copy constructor
            if constexpr(zeroValueIsOK) {
                if(other.val > 0 and not valid(other.val)) throw h5pp::runtime_error("Given identifier must be valid");
            } else {
                if(not valid(other.val)) throw h5pp::runtime_error("Given identifier must be valid");
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
                if(rhs.val > 0 and not valid(rhs.val)) throw h5pp::runtime_error("Given identifier must be valid");
            } else {
                if(not valid(rhs.val)) throw h5pp::runtime_error("Given identifier must be valid");
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

        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        hid_base &operator=(const T &rhs) {
            // Copy assignment from hid_t
            if constexpr(zeroValueIsOK) {
                if(rhs > 0 and not valid(rhs)) throw h5pp::runtime_error("Given identifier must be valid");
            } else {
                if(not valid(rhs)) throw h5pp::runtime_error("Given identifier must be valid");
            }

            close(); // Drop current
            val = rhs;
            return *this;
        }

        void close() {
            if(val == 0 or not valid()) return;
            if(H5Iget_ref(val) > 1) {
                H5Idec_ref(val);
            } else {
                /* clang-format off */
                herr_t err = 0;
                if constexpr(std::is_same_v<hid_h5x,h5d> ) err = H5Dclose(val);
                if constexpr(std::is_same_v<hid_h5x,h5a> ) err = H5Aclose(val);
                if constexpr(std::is_same_v<hid_h5x,h5o> ) err = H5Oclose(val);
                if constexpr(std::is_same_v<hid_h5x,h5t> ) err = H5Tclose(val);
                if constexpr(std::is_same_v<hid_h5x,h5s> ) err = H5Sclose(val);
                if constexpr(std::is_same_v<hid_h5x,h5f> ) err = H5Fclose(val);
                if constexpr(std::is_same_v<hid_h5x,h5g> ) err = H5Gclose(val);
                if constexpr(std::is_same_v<hid_h5x,h5p> ) err = H5Pclose(val);
                if constexpr(std::is_same_v<hid_h5x,h5e> ) err = H5Eclose_stack(val);
                /* clang-format on */
                if(err < 0) throw h5pp::runtime_error("Failed to close id {} id: {}", hid_h5x::tag, val);
            }
        }

        [[nodiscard]] const hid_t &value() const {
            if constexpr(zeroValueIsOK) {
                if(val == 0) return val;
            }
            if(valid()) {
                return val;
            } else {
                H5Eprint(H5E_DEFAULT, stderr);
                throw h5pp::runtime_error("Tried to return an invalid identifier {}: {}", hid_h5x::tag, val);
            }
        }

        [[nodiscard]] auto refcount() const {
            if constexpr(zeroValueIsOK) {
                if(val == 0) return 0;
            }
            if(valid()) {
                auto refc = H5Iget_ref(val);
                if(refc >= 0) {
                    return refc;
                } else {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw h5pp::runtime_error("Could not get reference count of identifier {}: {}", hid_h5x::tag, val);
                }
            } else {
                return 0;
            }
        }

        [[nodiscard]] std::string pretty_print() { return h5pp::format("{}: {} ({})", hid_h5x::tag, val, refcount()); }
        [[nodiscard]] std::string safe_print() { return h5pp::format("{} ({})", val, refcount()); }

        [[nodiscard]] bool valid(const hid_t &other) const {
            auto result = H5Iis_valid(other);
            if(result < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw h5pp::runtime_error("Failed to determine validity of identifier");
            }
            return result > 0;
        }
        [[nodiscard]] bool valid() const { return valid(val); }
        [[nodiscard]] bool valid(const hid_h5x &other) const { return other.valid(); }
        [[nodiscard]] bool equal(const hid_t &rhs) const {
            if constexpr(std::is_same_v<hid_h5x, h5p>) return (val > 0 and rhs > 0 and H5Pequal(val, rhs)) or val == rhs;
            else if constexpr(std::is_same_v<hid_h5x, h5t>) return (valid(val) and valid(rhs) and H5Tequal(val, rhs)) or val == rhs;
            else return val == rhs;
        }

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
        [[nodiscard]]      operator hid_t() const { return value(); }            // Class can be used as an actual hid_t
        explicit           operator bool() const { return valid() and val > 0; } // Test if set with syntax if(a)
        explicit           operator std::string() const { return h5pp::format("{}: {}", hid_h5x::tag, val); }
        explicit           operator std::string_view() const { return h5pp::format("{}: {}", hid_h5x::tag, val); }
    };

    // All our safe hid_t wrapper classes
    class h5p final : public hid_base<h5p, true> {
        public:
        using hid_base::hid_base;
        static constexpr std::string_view tag = "h5p";
    };

    class h5s final : public hid_base<h5s> {
        public:
        using hid_base::hid_base;
        static constexpr std::string_view tag = "h5s";
    };

    class h5t final : public hid_base<h5t> {
        public:
        using hid_base::hid_base;
        static constexpr std::string_view tag = "h5t";
    };

    class h5d final : public hid_base<h5d> {
        public:
        using hid_base::hid_base;
        static constexpr std::string_view tag = "h5d";
    };

    class h5g final : public hid_base<h5g> {
        public:
        using hid_base::hid_base;
        static constexpr std::string_view tag = "h5g";
    };

    class h5a final : public hid_base<h5a> {
        public:
        using hid_base::hid_base;
        static constexpr std::string_view tag = "h5a";
    };

    class h5o final : public hid_base<h5o> {
        public:
        using hid_base::hid_base;
        static constexpr std::string_view tag = "h5o";
    };

    class h5f final : public hid_base<h5f> {
        public:
        using hid_base::hid_base;
        static constexpr std::string_view tag = "h5f";
    };

    class h5e final : public hid_base<h5e, true> {
        public:
        using hid_base::hid_base;
        static constexpr std::string_view tag = "h5e";
    };
}