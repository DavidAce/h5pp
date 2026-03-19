#pragma once
#include "h5ppExcept.h"
#include <hdf5.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

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

        private:
        static herr_t release_id(hid_t id) noexcept {
            if(id == 0) return 0;

            htri_t is_valid = H5Iis_valid(id);
            if(is_valid <= 0) return is_valid < 0 ? -1 : 0;

            int ref = H5Iget_ref(id);
            if(ref < 0) return -1;
            if(ref > 1) return H5Idec_ref(id) < 0 ? -1 : 0;

            if constexpr(std::is_same_v<hid_h5x, h5o>) {
                H5I_type_t type = H5Iget_type(id);
                if(type == H5I_DATATYPE) return H5Tclose(id);
                return H5Oclose(id);
            }
            if constexpr(std::is_same_v<hid_h5x, h5d>) return H5Dclose(id);
            if constexpr(std::is_same_v<hid_h5x, h5a>) return H5Aclose(id);
            if constexpr(std::is_same_v<hid_h5x, h5t>) return H5Tclose(id);
            if constexpr(std::is_same_v<hid_h5x, h5s>) return H5Sclose(id);
            if constexpr(std::is_same_v<hid_h5x, h5f>) return H5Fclose(id);
            if constexpr(std::is_same_v<hid_h5x, h5g>) return H5Gclose(id);
            if constexpr(std::is_same_v<hid_h5x, h5p>) return H5Pclose(id);
            if constexpr(std::is_same_v<hid_h5x, h5e>) return H5Eclose_stack(id);
            return -1;
        }

        static void validate_id(hid_t id, bool allowZero = zeroValueIsOK) {
            if(id == 0) {
                if(allowZero) return;
                throw h5pp::runtime_error("Given identifier must be valid");
            }

            htri_t is_valid = H5Iis_valid(id);
            if(is_valid < 0) throw h5pp::runtime_error("Failed to determine validity of identifier");
            if(is_valid == 0) throw h5pp::runtime_error("Given identifier must be valid");

            H5I_type_t type = H5Iget_type(id);
            if(type == H5I_BADID) throw h5pp::runtime_error("Failed to determine HDF5 identifier type");
            if(not hid_h5x::accepts(type, id)) throw h5pp::runtime_error("Wrong HDF5 id type for {}", hid_h5x::tag);
        }

        public:
        ~hid_base() noexcept { (void)release(); }
        hid_base() = default;
        // Use enable_if to avoid implicit conversion from hid_h5x and still have a non-explicit hid_t constructor
        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        hid_base(const T &other) {
            validate_id(other);
            val = other;
        }

        hid_base(const hid_base &other) : val(other.val) {
            validate_id(val, true);
            if(val > 0 and H5Iinc_ref(val) < 0) throw h5pp::runtime_error("H5Iinc_ref failed");
        }
        hid_base(hid_base &&other) noexcept : val(std::exchange(other.val, 0)) {}

        hid_base &operator=(const hid_base &rhs) {
            if(this == &rhs) return *this;
            validate_id(rhs.val, true);
            if(rhs.val > 0 and H5Iinc_ref(rhs.val) < 0) throw h5pp::runtime_error("H5Iinc_ref failed");

            hid_t  old = std::exchange(val, rhs.val);
            herr_t err = release_id(old);
            if(err < 0) {
                val = 0;
                throw h5pp::runtime_error("Failed to close id {} id: {}", hid_h5x::tag, old);
            }
            return *this;
        }

        hid_base &operator=(hid_base &&other) noexcept {
            if(this != &other) {
                (void)release();
                val = std::exchange(other.val, 0);
            }
            return *this;
        }

        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        hid_base &operator=(const T &rhs) {
            if(val == rhs) return *this;
            validate_id(rhs);
            hid_t  old = std::exchange(val, rhs);
            herr_t err = release_id(old);
            if(err < 0) {
                val = 0;
                throw h5pp::runtime_error("Failed to close id {} id: {}", hid_h5x::tag, old);
            }
            return *this;
        }
        herr_t release() noexcept {
            hid_t id = std::exchange(val, 0); // detach wrapper state first
            return release_id(id);
        }
        void close() {
            hid_t id  = val;
            auto err = release();
            if(err < 0) throw h5pp::runtime_error("Failed to close id {} id: {}", hid_h5x::tag, id);
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
            if(val == rhs) return true;
            if(val <= 0 or rhs <= 0) return false;
            if constexpr(std::is_same_v<hid_h5x, h5p>) {
                htri_t eq = H5Pequal(val, rhs);
                if(eq < 0) throw h5pp::runtime_error("H5Pequal failed");
                return eq > 0;
            } else if constexpr(std::is_same_v<hid_h5x, h5t>) {
                htri_t eq = H5Tequal(val, rhs);
                if(eq < 0) throw h5pp::runtime_error("H5Tequal failed");
                return eq > 0;
            } else {
                return false;
            }
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
    };

    // All our safe hid_t wrapper classes
    class h5p final : public hid_base<h5p, true> {
        public:
        using hid_base::hid_base;
        static bool                       accepts(H5I_type_t type, hid_t) noexcept { return type == H5I_GENPROP_LST; }
        static constexpr std::string_view tag = "h5p";
    };

    class h5s final : public hid_base<h5s> {
        public:
        using hid_base::hid_base;
        static bool                       accepts(H5I_type_t type, hid_t) noexcept { return type == H5I_DATASPACE; }
        static constexpr std::string_view tag     = "h5s";
    };

    class h5t final : public hid_base<h5t> {
        public:
        using hid_base::hid_base;
        static bool                       accepts(H5I_type_t type, hid_t) noexcept { return type == H5I_DATATYPE; }
        static constexpr std::string_view tag     = "h5t";
    };

    class h5d final : public hid_base<h5d> {
        public:
        using hid_base::hid_base;
        static bool                       accepts(H5I_type_t type, hid_t) noexcept { return type == H5I_DATASET; }
        static constexpr std::string_view tag     = "h5d";
    };

    class h5g final : public hid_base<h5g> {
        public:
        using hid_base::hid_base;
        static bool                       accepts(H5I_type_t type, hid_t) noexcept { return type == H5I_GROUP; }
        static constexpr std::string_view tag     = "h5g";
    };

    class h5a final : public hid_base<h5a> {
        public:
        using hid_base::hid_base;
        static bool                       accepts(H5I_type_t type, hid_t) noexcept { return type == H5I_ATTR; }
        static constexpr std::string_view tag     = "h5a";
    };

    class h5o final : public hid_base<h5o> {
        public:
        using hid_base::hid_base;
        static bool accepts(H5I_type_t type, hid_t id) noexcept {
            (void)id;
            return type == H5I_GROUP or type == H5I_DATASET or type == H5I_DATATYPE;
        }
        static constexpr std::string_view tag = "h5o";
    };

    class h5f final : public hid_base<h5f> {
        public:
        using hid_base::hid_base;
        static bool                       accepts(H5I_type_t type, hid_t) noexcept { return type == H5I_FILE; }
        static constexpr std::string_view tag     = "h5f";
    };

    class h5e final : public hid_base<h5e, true> {
        public:
        using hid_base::hid_base;
        static bool                       accepts(H5I_type_t type, hid_t) noexcept { return type == H5I_ERROR_STACK; }
        static constexpr std::string_view tag     = "h5e";
    };
}
