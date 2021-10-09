#pragma once
#include "h5ppHid.h"
#include "h5ppType.h"
#include "h5ppTypeSfinae.h"
#include <complex>
#include <hdf5.h>
namespace h5pp::type::compound {

    template<typename T>
    struct Complex {
        T real, imag; // real,imag parts
        using value_type = T;
        using Scalar     = T;
        Complex()        = default;
        explicit Complex(const std::complex<T> &in) {
            real = in.real();
            imag = in.imag();
        }
        Complex &operator=(const std::complex<T> &rhs) {
            real = rhs.real();
            imag = rhs.imag();
            return *this;
        }
    };

    template<typename T>
    struct Scalar2 {
        T x, y;
        using value_type = T;
        using Scalar     = T;
        Scalar2()        = default;
        template<typename Scalar2Type>
        explicit Scalar2(const Scalar2Type &in) {
            static_assert(std::is_same_v<T, decltype(in.x)>);
            static_assert(std::is_same_v<T, decltype(in.y)>);
            x = in.x;
            y = in.y;
        }
        template<typename Scalar2Type>
        Scalar2 &operator=(const Scalar2Type &rhs) {
            static_assert(std::is_same_v<T, decltype(rhs.x)>);
            static_assert(std::is_same_v<T, decltype(rhs.y)>);
            x = rhs.x;
            y = rhs.y;
            return *this;
        }
    };

    template<typename T>
    struct Scalar3 {
        T x, y, z;
        using value_type = T;
        using Scalar     = T;
        Scalar3()        = default;
        template<typename Scalar3Type>
        explicit Scalar3(const Scalar3Type &in) {
            static_assert(std::is_same_v<T, decltype(in.x)>);
            static_assert(std::is_same_v<T, decltype(in.y)>);
            static_assert(std::is_same_v<T, decltype(in.z)>);
            x = in.x;
            y = in.y;
            z = in.z;
        }
        template<typename Scalar3Type>
        Scalar3 &operator=(const Scalar3Type &rhs) {
            static_assert(std::is_same_v<T, decltype(rhs.x)>);
            static_assert(std::is_same_v<T, decltype(rhs.y)>);
            static_assert(std::is_same_v<T, decltype(rhs.z)>);
            x = rhs.x;
            y = rhs.y;
            z = rhs.z;
            return *this;
        }
    };

    template<typename T, typename = std::enable_if_t<not std::is_same_v<T, std::false_type>>>
    class H5T_COMPLEX {
        private:
        inline static hid::h5t complex_id;
        inline static hid::h5t native_id;
        static constexpr char  fieldR[5] = "real";
        static constexpr char  fieldI[5] = "imag";
        static void            init() {
            if(complex_id.valid() and native_id.valid()) return;
            complex_id  = H5Tcreate(H5T_COMPOUND, sizeof(Complex<T>));
            native_id   = h5pp::type::getH5NativeType<T>();
            herr_t errr = H5Tinsert(complex_id, fieldR, HOFFSET(Complex<T>, real), native_id);
            herr_t erri = H5Tinsert(complex_id, fieldI, HOFFSET(Complex<T>, imag), native_id);
            if(errr < 0) throw h5pp::runtime_error("Failed to insert real field to complex type");
            if(erri < 0) throw h5pp::runtime_error("Failed to insert imag field to complex type");
        }

        public:
        static hid::h5t &h5type() {
            if(not complex_id.valid()) init();
            if constexpr(std::is_same_v<T, float>) {
                if(H5Tget_size(complex_id) != 8) throw h5pp::runtime_error("");
            }
            if constexpr(std::is_same_v<T, std::complex<float>>) { throw h5pp::runtime_error(""); }
            return complex_id;
        }
        static bool equal(const hid::h5t &other) {
            if(H5Tequal(h5type(), other)) return true;
            if(H5Tget_class(other) != H5T_class_t::H5T_COMPOUND) return false;
            if(H5Tget_size(other) != H5Tget_size(h5type())) return false;
            if(H5Tget_nmembers(other) != 2) return false;
            if(not H5Tequal(native_id, hid::h5t(H5Tget_member_type(other, 0)))) return false;
            if(not H5Tequal(native_id, hid::h5t(H5Tget_member_type(other, 1)))) return false;
            auto field0 = H5Tget_member_name(other, 0);
            auto field1 = H5Tget_member_name(other, 1);
            bool matchR = std::string_view(fieldR) == std::string_view(field0);
            bool matchI = std::string_view(fieldI) == std::string_view(field1);
            H5free_memory(field0);
            H5free_memory(field1);
            return matchR and matchI;
        }
        template<typename O>
        constexpr static bool equal() {
            return std::is_same_v<std::complex<T>, std::decay_t<O>>;
        }
        bool operator==(const hid::h5t &other) { return equal(other); }
             operator hid::h5t() { return h5type(); }
    };

    template<typename T, typename = std::enable_if_t<not std::is_same_v<T, std::false_type>>>
    class H5T_SCALAR2 {
        private:
        inline static hid::h5t scalar2_id;
        inline static hid::h5t native_id;
        static constexpr char  fieldX[2] = "x";
        static constexpr char  fieldY[2] = "y";
        static void            init() {
            scalar2_id  = H5Tcreate(H5T_COMPOUND, sizeof(Scalar2<T>));
            native_id   = h5pp::type::getH5NativeType<T>();
            herr_t errx = H5Tinsert(scalar2_id, "x", HOFFSET(Scalar2<T>, x), native_id);
            herr_t erry = H5Tinsert(scalar2_id, "y", HOFFSET(Scalar2<T>, y), native_id);
            if(errx < 0) throw h5pp::runtime_error("Failed to insert x field to Scalar2 type");
            if(erry < 0) throw h5pp::runtime_error("Failed to insert y field to Scalar2 type");
        }

        public:
        static hid::h5t &h5type() {
            if(not scalar2_id.valid()) init();
            return scalar2_id;
        }
        static bool equal(const hid::h5t &other) {
            if(H5Tequal(h5type(), other)) return true;
            if(H5Tget_class(other) != H5T_class_t::H5T_COMPOUND) return false;
            if(H5Tget_size(other) != H5Tget_size(h5type())) return false;
            if(H5Tget_nmembers(other) != 2) return false;
            if(not H5Tequal(native_id, hid::h5t(H5Tget_member_type(other, 0)))) return false;
            if(not H5Tequal(native_id, hid::h5t(H5Tget_member_type(other, 1)))) return false;
            auto field0 = H5Tget_member_name(other, 0);
            auto field1 = H5Tget_member_name(other, 1);
            bool matchX = std::string_view(fieldX) == std::string_view(field0);
            bool matchY = std::string_view(fieldY) == std::string_view(field1);
            H5free_memory(field0);
            H5free_memory(field1);
            return matchX and matchY;
        }

        template<typename O>
        constexpr static bool equal() {
            return std::is_same_v<h5pp::type::compound::Scalar2<T>, std::decay_t<O>>;
        }
        bool operator==(const hid::h5t &other) { return equal(other); }
             operator hid::h5t() { return h5type(); }
    };
    template<typename T, typename = std::enable_if_t<not std::is_same_v<T, std::false_type>>>
    class H5T_SCALAR3 {
        private:
        inline static hid::h5t scalar3_id;
        inline static hid::h5t native_id;
        static constexpr char  fieldX[2] = "x";
        static constexpr char  fieldY[2] = "y";
        static constexpr char  fieldZ[2] = "z";
        static void            init() {
            scalar3_id  = H5Tcreate(H5T_COMPOUND, sizeof(Scalar3<T>));
            native_id   = h5pp::type::getH5NativeType<T>();
            herr_t errx = H5Tinsert(scalar3_id, "x", HOFFSET(Scalar3<T>, x), native_id);
            herr_t erry = H5Tinsert(scalar3_id, "y", HOFFSET(Scalar3<T>, y), native_id);
            herr_t errz = H5Tinsert(scalar3_id, "z", HOFFSET(Scalar3<T>, z), native_id);
            if(errx < 0) throw h5pp::runtime_error("Failed to insert x field to Scalar3 type");
            if(erry < 0) throw h5pp::runtime_error("Failed to insert y field to Scalar3 type");
            if(errz < 0) throw h5pp::runtime_error("Failed to insert z field to Scalar3 type");
        }

        public:
        static hid::h5t &h5type() {
            if(not scalar3_id.valid()) init();
            return scalar3_id;
        }
        static bool equal(const hid::h5t &other) {
            if(H5Tequal(h5type(), other)) return true;
            if(H5Tget_class(other) != H5T_class_t::H5T_COMPOUND) return false;
            if(H5Tget_size(other) != H5Tget_size(h5type())) return false;
            if(H5Tget_nmembers(other) != 3) return false;
            if(not H5Tequal(native_id, hid::h5t(H5Tget_member_type(other, 0)))) return false;
            if(not H5Tequal(native_id, hid::h5t(H5Tget_member_type(other, 1)))) return false;
            auto field0 = H5Tget_member_name(other, 0);
            auto field1 = H5Tget_member_name(other, 1);
            auto field2 = H5Tget_member_name(other, 2);
            bool matchX = std::string_view(fieldX) == std::string_view(field0);
            bool matchY = std::string_view(fieldY) == std::string_view(field1);
            bool matchZ = std::string_view(fieldZ) == std::string_view(field2);
            H5free_memory(field0);
            H5free_memory(field1);
            H5free_memory(field2);
            return matchX and matchY and matchZ;
        }

        template<typename O>
        constexpr static bool equal() {
            return std::is_same_v<h5pp::type::compound::Scalar2<T>, std::decay_t<O>>;
        }
        bool operator==(const hid::h5t &other) { return equal(other); }
             operator hid::h5t() { return h5type(); }
    };
}
