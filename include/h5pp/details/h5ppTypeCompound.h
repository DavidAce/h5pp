#pragma once
#include "h5ppError.h"
#include "h5ppHid.h"
#include "h5ppTypeSfinae.h"
#include <complex>
#include <H5Tpublic.h>

namespace h5pp::type::compound {
    template<typename DataType>
    [[nodiscard]] hid::h5t getValueType() {
        //        if(h5type.has_value()) return h5type.value(); // Intercept
        using DecayType = typename std::decay<DataType>::type;
        /* clang-format off */
        if constexpr (std::is_pointer_v<DecayType>)                return getValueType<typename std::remove_pointer<DecayType>::type>();
        else if constexpr (std::is_reference_v<DecayType>)         return getValueType<typename std::remove_reference<DecayType>::type>();
        else if constexpr (std::is_array_v<DecayType>)             return getValueType<typename std::remove_all_extents<DecayType>::type>();
        else if constexpr (std::is_arithmetic_v<DecayType>){
            if constexpr(std::is_same_v<DecayType, short>)                        return H5Tcopy(H5T_NATIVE_SHORT);
            else if constexpr (std::is_same_v<DecayType, int>)                    return H5Tcopy(H5T_NATIVE_INT);
            else if constexpr (std::is_same_v<DecayType, long>)                   return H5Tcopy(H5T_NATIVE_LONG);
            else if constexpr (std::is_same_v<DecayType, long long>)              return H5Tcopy(H5T_NATIVE_LLONG);
            else if constexpr (std::is_same_v<DecayType, unsigned short>)         return H5Tcopy(H5T_NATIVE_USHORT);
            else if constexpr (std::is_same_v<DecayType, unsigned int>)           return H5Tcopy(H5T_NATIVE_UINT);
            else if constexpr (std::is_same_v<DecayType, unsigned long>)          return H5Tcopy(H5T_NATIVE_ULONG);
            else if constexpr (std::is_same_v<DecayType, unsigned long long>)     return H5Tcopy(H5T_NATIVE_ULLONG);
            else if constexpr (std::is_same_v<DecayType, double>)                 return H5Tcopy(H5T_NATIVE_DOUBLE);
            else if constexpr (std::is_same_v<DecayType, long double>)            return H5Tcopy(H5T_NATIVE_LDOUBLE);
            else if constexpr (std::is_same_v<DecayType, bool> )                  return H5Tcopy(H5T_NATIVE_UINT8);
            else if constexpr (std::is_same_v<DecayType, char> )                  return H5Tcopy(H5T_NATIVE_CHAR);
            else if constexpr (std::is_same_v<DecayType, unsigned char> )         return H5Tcopy(H5T_NATIVE_UCHAR);
            else if constexpr (std::is_same_v<DecayType, float>)                  return H5Tcopy(H5T_NATIVE_FLOAT);
            else static_assert(type::sfinae::unrecognized_type_v<DecayType> and "Failed to match this type with a native HDF5 type");
        }
        else if constexpr(sfinae::has_value_type_v<DecayType> or sfinae::has_Scalar_v<DecayType>){
            if constexpr (type::sfinae::has_Scalar_v <DecayType>)                 return getValueType<typename DecayType::Scalar>();
            if constexpr (type::sfinae::has_value_type_v <DecayType>)             return getValueType<typename DataType::value_type>();
        }
        /* clang-format on */
        else
            static_assert(sfinae::invalid_type_v<DecayType> and "This Complex/ScalarN type is not supported");
    }

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

    bool is_complex(const hid::h5t &h5type);
    bool is_scalar2(const hid::h5t &h5type);
    bool is_scalar3(const hid::h5t &h5type);

    template<typename T>
    class H5T_COMPLEX {
        private:
        inline static hid::h5t complex_id;
        inline static hid::h5t value_id;
        static constexpr char  fieldR[5] = "real";
        static constexpr char  fieldI[5] = "imag";
        static void            init() {
            if(not value_id.valid()) value_id = getValueType<T>();
            if(not complex_id.valid()) {
                complex_id  = H5Tcreate(H5T_COMPOUND, sizeof(Complex<T>));
                herr_t errr = H5Tinsert(complex_id, fieldR, HOFFSET(Complex<T>, real), value_id);
                herr_t erri = H5Tinsert(complex_id, fieldI, HOFFSET(Complex<T>, imag), value_id);
                if(errr < 0) throw h5pp::runtime_error("Failed to insert real field to complex type");
                if(erri < 0) throw h5pp::runtime_error("Failed to insert imag field to complex type");
            }
        }

        public:
        static hid::h5t &h5type() {
            init();
            return complex_id;
        }
        static constexpr std::string_view get_fieldR_name() { return fieldR; }
        static constexpr std::string_view get_fieldI_name() { return fieldI; }
        static bool                       equal(const hid::h5t &other) {
            // Try to compare without initializing complex_id
            if(complex_id.valid() and H5Tequal(complex_id, other) == 1) return true;
            if(H5Tget_size(other) != sizeof(Complex<T>)) return false;
            if(not is_complex(other)) return false;
            if(not value_id.valid()) value_id = getValueType<T>();
            if(not H5Tequal(value_id, hid::h5t(H5Tget_member_type(other, 0)))) return false;
            if(not H5Tequal(value_id, hid::h5t(H5Tget_member_type(other, 1)))) return false;
            // The user seem to be using this type, so we initialize it to speed up later comparisons
            init();
            return true;
        }
        template<typename O>
        constexpr static bool equal() {
            return std::is_same_v<std::complex<T>, std::decay_t<O>>;
        }
        bool operator==(const hid::h5t &other) { return equal(other); }
             operator hid::h5t() { return h5type(); }
    };

    template<typename T>
    class H5T_SCALAR2 {
        private:
        inline static hid::h5t scalar2_id;
        inline static hid::h5t value_id;
        static constexpr char  fieldX[2] = "x";
        static constexpr char  fieldY[2] = "y";
        static void            init() {
            if(not value_id.valid()) value_id = getValueType<T>();
            if(not scalar2_id.valid()) {
                scalar2_id  = H5Tcreate(H5T_COMPOUND, sizeof(Scalar2<T>));
                herr_t errx = H5Tinsert(scalar2_id, "x", HOFFSET(Scalar2<T>, x), value_id);
                herr_t erry = H5Tinsert(scalar2_id, "y", HOFFSET(Scalar2<T>, y), value_id);
                if(errx < 0) throw h5pp::runtime_error("Failed to insert x field to Scalar2 type");
                if(erry < 0) throw h5pp::runtime_error("Failed to insert y field to Scalar2 type");
            }
        }

        public:
        static hid::h5t &h5type() {
            if(not scalar2_id.valid()) init();
            return scalar2_id;
        }
        static constexpr std::string_view get_fieldX_name() { return fieldX; }
        static constexpr std::string_view get_fieldY_name() { return fieldY; }
        static bool                       equal(const hid::h5t &other) {
            // Try to compare without initializing scalar2_id
            if(scalar2_id.valid() and H5Tequal(scalar2_id, other) == 1) return true;
            if(H5Tget_size(other) != sizeof(Scalar2<T>)) return false;
            if(not is_scalar2(other)) return false;
            if(not value_id.valid()) value_id = getValueType<T>();
            if(not H5Tequal(value_id, hid::h5t(H5Tget_member_type(other, 0)))) return false;
            if(not H5Tequal(value_id, hid::h5t(H5Tget_member_type(other, 1)))) return false;
            // The user seem to be using this type, so we initialize it to speed up later comparisons
            init();
            return true;
        }

        template<typename O>
        constexpr static bool equal() {
            return std::is_same_v<h5pp::type::compound::Scalar2<T>, std::decay_t<O>>;
        }
        bool operator==(const hid::h5t &other) { return equal(other); }
             operator hid::h5t() { return h5type(); }
    };
    template<typename T>
    class H5T_SCALAR3 {
        private:
        inline static hid::h5t scalar3_id;
        inline static hid::h5t value_id;
        static constexpr char  fieldX[2] = "x";
        static constexpr char  fieldY[2] = "y";
        static constexpr char  fieldZ[2] = "z";
        static void            init() {
            if(not value_id.valid()) value_id = getValueType<T>();
            if(not scalar3_id.valid()) {
                scalar3_id  = H5Tcreate(H5T_COMPOUND, sizeof(Scalar3<T>));
                herr_t errx = H5Tinsert(scalar3_id, "x", HOFFSET(Scalar3<T>, x), value_id);
                herr_t erry = H5Tinsert(scalar3_id, "y", HOFFSET(Scalar3<T>, y), value_id);
                herr_t errz = H5Tinsert(scalar3_id, "z", HOFFSET(Scalar3<T>, z), value_id);
                if(errx < 0) throw h5pp::runtime_error("Failed to insert x field to Scalar3 type");
                if(erry < 0) throw h5pp::runtime_error("Failed to insert y field to Scalar3 type");
                if(errz < 0) throw h5pp::runtime_error("Failed to insert z field to Scalar3 type");
            }
        }

        public:
        static hid::h5t &h5type() {
            if(not scalar3_id.valid()) init();
            return scalar3_id;
        }
        static constexpr std::string_view get_fieldX_name() { return fieldX; }
        static constexpr std::string_view get_fieldY_name() { return fieldY; }
        static constexpr std::string_view get_fieldZ_name() { return fieldZ; }
        static bool                       equal(const hid::h5t &other) {
            // Try to compare without initializing scalar2_id
            if(scalar3_id.valid() and H5Tequal(scalar3_id, other) == 1) return true;
            if(H5Tget_size(other) != sizeof(Scalar3<T>)) return false;
            if(not is_scalar3(other)) return false;
            if(not value_id.valid()) value_id = getValueType<T>();
            if(not H5Tequal(value_id, hid::h5t(H5Tget_member_type(other, 0)))) return false;
            if(not H5Tequal(value_id, hid::h5t(H5Tget_member_type(other, 1)))) return false;
            if(not H5Tequal(value_id, hid::h5t(H5Tget_member_type(other, 2)))) return false;
            // The user seem to be using this type, so we initialize it to speed up later comparisons
            init();
            return true;
        }

        template<typename O>
        constexpr static bool equal() {
            return std::is_same_v<h5pp::type::compound::Scalar2<T>, std::decay_t<O>>;
        }
        bool operator==(const hid::h5t &other) { return equal(other); }
             operator hid::h5t() { return h5type(); }
    };

    inline bool is_complex(const hid::h5t &h5type) {
        if(H5Tget_class(h5type) != H5T_class_t::H5T_COMPOUND) return false;
        if(H5Tget_nmembers(h5type) != 2) return false;
        {
            auto field = H5Tget_member_name(h5type, 0);
            bool match = std::string_view(H5T_COMPLEX<void>::get_fieldR_name()) == std::string_view(field);
            H5free_memory(field);
            if(not match) return false;
        }
        {
            auto field = H5Tget_member_name(h5type, 1);
            bool match = std::string_view(H5T_COMPLEX<void>::get_fieldI_name()) == std::string_view(field);
            H5free_memory(field);
            if(not match) return false;
        }
        return true;
    }

    inline bool is_scalar2(const hid::h5t &h5type) {
        if(H5Tget_class(h5type) != H5T_class_t::H5T_COMPOUND) return false;
        if(H5Tget_nmembers(h5type) != 2) return false;
        {
            auto field = H5Tget_member_name(h5type, 0);
            bool match = std::string_view(H5T_SCALAR2<void>::get_fieldX_name()) == std::string_view(field);
            H5free_memory(field);
            if(not match) return false;
        }
        {
            auto field = H5Tget_member_name(h5type, 1);
            bool match = std::string_view(H5T_SCALAR2<void>::get_fieldY_name()) == std::string_view(field);
            H5free_memory(field);
            if(not match) return false;
        }
        return true;
    }

    inline bool is_scalar3(const hid::h5t &h5type) {
        if(H5Tget_class(h5type) != H5T_class_t::H5T_COMPOUND) return false;
        if(H5Tget_nmembers(h5type) != 3) return false;
        {
            auto field = H5Tget_member_name(h5type, 0);
            bool match = std::string_view(H5T_SCALAR3<void>::get_fieldX_name()) == std::string_view(field);
            H5free_memory(field);
            if(not match) return false;
        }
        {
            auto field = H5Tget_member_name(h5type, 1);
            bool match = std::string_view(H5T_SCALAR3<void>::get_fieldY_name()) == std::string_view(field);
            H5free_memory(field);
            if(not match) return false;
        }
        {
            auto field = H5Tget_member_name(h5type, 2);
            bool match = std::string_view(H5T_SCALAR3<void>::get_fieldZ_name()) == std::string_view(field);
            H5free_memory(field);
            if(not match) return false;
        }
        return true;
    }
}
