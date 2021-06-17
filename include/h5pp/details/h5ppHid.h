#pragma once
#include <hdf5.h>
#include <stdexcept>
#include <string>

namespace h5pp::hid {

    // Base class for all the safe "hid_t" wrapper classes. Zero value is the default for H5P and H5E so it's ok for them to return it zero
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
            val = other;
        }

        hid_base(const hid_base &other) {
            // Copy constructor
            if constexpr(zeroValueIsOK) {
                if(other.val > 0 and not valid(other.val)) throw std::runtime_error("Given identifier must be valid");
            } else {
                if(not valid(other.val)) throw std::runtime_error("Given identifier must be valid");
            }
            val = other.val;
            if(valid(other.val)) H5Iinc_ref(val); // Increment reference counter of identifier
        }

        hid_base(hid_base &&other) noexcept {
            // Move constructor
            val = other.val; // Checks that we got a valid identifier through .value() (throws)
            H5Iinc_ref(val); // Increment reference counter of identifier
        }

        hid_base &operator=(const hid_base &rhs) {
            if(this == &rhs) return *this;
            // Copy assignment
            if constexpr(zeroValueIsOK) {
                if(rhs.val > 0 and not valid(rhs.val)) throw std::runtime_error("Given identifier must be valid");
            } else {
                if(not valid(rhs.val)) throw std::runtime_error("Given identifier must be valid");
            }
            if(not equal(rhs.val)) close(); // Drop current
            val = rhs.val;
            if(valid(val)) H5Iinc_ref(val); // Increment reference counter of identifier
            return *this;
        }

        hid_base &operator=(hid_base &&rhs) noexcept {
            if(this == &rhs) return *this;
            // Move assignment
            if(not equal(rhs.val)) close(); // Drop current
            val = rhs.val;
            H5Iinc_ref(val); // Increment reference counter of identifier
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

            if(not equal(rhs)) close(); // Drop current
            val = rhs;
            H5Iinc_ref(val); // Increment reference counter of identifier
            return *this;
        }

        virtual void                      close()     = 0;
        [[nodiscard]] virtual std::string tag() const = 0;
        [[nodiscard]] const hid_t &       value() const {
            if(valid() or (val == 0 and zeroValueIsOK))
                return val;
            else {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Tried to return an invalid identifier " + tag() + ":" + std::to_string(val));
            }
        }

        [[nodiscard]] auto refcount() const {
            if(zeroValueIsOK and val == 0) return 0;
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
        void                      close() final {
            if(valid()) {
                if(H5Iget_ref(val) > 1)
                    H5Idec_ref(val);
                else if(H5Pclose(val) < 0)
                    H5Eprint(H5E_DEFAULT, stderr);
            }
        }
    };

    class h5s final : public hid_base<h5s> {
        public:
        using hid_base::hid_base;
        ~h5s() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5s"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
        void                      close() final {
            if(valid()) {
                if(H5Iget_ref(val) > 1)
                    H5Idec_ref(val);
                else if(H5Sclose(val) < 0)
                    H5Eprint(H5E_DEFAULT, stderr);
            }
        }
    };

    class h5t final : public hid_base<h5t> {
        public:
        using hid_base::hid_base;
        ~h5t() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5t"; }
        [[nodiscard]] bool equal(const hid_t &rhs) const final { return (valid(val) and valid(rhs) and H5Tequal(val, rhs)) or val == rhs; }
        void               close() final {
            if(valid()) {
                if(H5Iget_ref(val) > 1)
                    H5Idec_ref(val);
                else if(H5Tclose(val) < 0)
                    H5Eprint(H5E_DEFAULT, stderr);
            }
        }
    };

    class h5d final : public hid_base<h5d> {
        public:
        using hid_base::hid_base;
        ~h5d() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5d"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
        void                      close() final {
            if(valid()) {
                if(H5Iget_ref(val) > 1)
                    H5Idec_ref(val);
                else if(H5Dclose(val) < 0)
                    H5Eprint(H5E_DEFAULT, stderr);
            }
        }
    };

    class h5g final : public hid_base<h5g> {
        public:
        using hid_base::hid_base;
        ~h5g() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5g"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
        void                      close() final {
            if(valid()) {
                if(H5Iget_ref(val) > 1)
                    H5Idec_ref(val);
                else if(H5Gclose(val) < 0)
                    H5Eprint(H5E_DEFAULT, stderr);
            }
        }
    };

    class h5a final : public hid_base<h5a> {
        public:
        using hid_base::hid_base;
        ~h5a() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5a"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
        void                      close() final {
            if(valid()) {
                if(H5Iget_ref(val) > 1)
                    H5Idec_ref(val);
                else if(H5Aclose(val) < 0)
                    H5Eprint(H5E_DEFAULT, stderr);
            }
        }
    };

    class h5o final : public hid_base<h5o> {
        public:
        using hid_base::hid_base;
        ~h5o() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5o"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
        void                      close() final {
            if(valid()) {
                if(H5Iget_ref(val) > 1)
                    H5Idec_ref(val);
                else if(H5Oclose(val) < 0)
                    H5Eprint(H5E_DEFAULT, stderr);
            }
        }
    };

    class h5f final : public hid_base<h5f> {
        public:
        using hid_base::hid_base;
        ~h5f() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5f"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
        void                      close() final {
            if(H5Iget_ref(val) > 1)
                H5Idec_ref(val);
            else if(H5Fclose(val) < 0)
                H5Eprint(H5E_DEFAULT, stderr);
        }
    };

    class h5e final : public hid_base<h5e, true> {
        public:
        using hid_base::hid_base;
        ~h5e() final { close(); }
        [[nodiscard]] std::string tag() const final { return "h5e"; }
        [[nodiscard]] bool        equal(const hid_t &rhs) const final { return val == rhs; }
        void                      close() final {
            if(valid()) {
                if(H5Iget_ref(val) > 1)
                    H5Idec_ref(val);
                else
                    H5Eclose_stack(val);
            }
        }
    };
}