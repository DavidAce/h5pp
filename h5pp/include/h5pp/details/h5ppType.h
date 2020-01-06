#pragma once
#include "h5ppTypeCheck.h"
#include "h5ppTypeComplex.h"

namespace h5pp::Type {
    template<typename DataType> constexpr hid_t getDataType() {
        namespace tc = h5pp::Type::Check;
        /* clang-format off */
        using DecayType = typename std::decay<DataType>::type;
        if constexpr (std::is_same<DecayType, int>::value)                                              return H5Tcopy(H5T_NATIVE_INT);
        if constexpr (std::is_same<DecayType, long>::value)                                             return H5Tcopy(H5T_NATIVE_LONG);
        if constexpr (std::is_same<DecayType, long long>::value)                                        return H5Tcopy(H5T_NATIVE_LLONG);
        if constexpr (std::is_same<DecayType, unsigned int>::value)                                     return H5Tcopy(H5T_NATIVE_UINT);
        if constexpr (std::is_same<DecayType, unsigned long>::value)                                    return H5Tcopy(H5T_NATIVE_ULONG);
        if constexpr (std::is_same<DecayType, unsigned long long >::value)                              return H5Tcopy(H5T_NATIVE_ULLONG);
        if constexpr (std::is_same<DecayType, double>::value)                                           return H5Tcopy(H5T_NATIVE_DOUBLE);
        if constexpr (std::is_same<DecayType, float>::value)                                            return H5Tcopy(H5T_NATIVE_FLOAT);
        if constexpr (std::is_same<DecayType, bool>::value)                                             return H5Tcopy(H5T_NATIVE_HBOOL);
        if constexpr (std::is_same<DecayType, std::string>::value)                                      return H5Tcopy(H5T_C_S1);
        if constexpr (std::is_same<DecayType, char>::value)                                             return H5Tcopy(H5T_C_S1);
        if constexpr (std::is_same<DecayType, std::complex<int>>::value)                                return H5Tcopy(Complex::H5T_COMPLEX_INT);
        if constexpr (std::is_same<DecayType, std::complex<long>>::value)                               return H5Tcopy(Complex::H5T_COMPLEX_LONG);
        if constexpr (std::is_same<DecayType, std::complex<long long>>::value)                          return H5Tcopy(Complex::H5T_COMPLEX_LLONG);
        if constexpr (std::is_same<DecayType, std::complex<unsigned int>>::value)                       return H5Tcopy(Complex::H5T_COMPLEX_UINT);
        if constexpr (std::is_same<DecayType, std::complex<unsigned long>>::value)                      return H5Tcopy(Complex::H5T_COMPLEX_ULONG);
        if constexpr (std::is_same<DecayType, std::complex<unsigned long long>>::value)                 return H5Tcopy(Complex::H5T_COMPLEX_ULLONG);
        if constexpr (std::is_same<DecayType, std::complex<double>>::value)                             return H5Tcopy(Complex::H5T_COMPLEX_DOUBLE);
        if constexpr (std::is_same<DecayType, std::complex<float>>::value)                              return H5Tcopy(Complex::H5T_COMPLEX_FLOAT);
        if constexpr (tc::is_Scalar2_of_type<DecayType, int>())                                         return H5Tcopy(Complex::H5T_SCALAR2_INT);
        if constexpr (tc::is_Scalar2_of_type<DecayType, long>())                                        return H5Tcopy(Complex::H5T_SCALAR2_LONG);
        if constexpr (tc::is_Scalar2_of_type<DecayType, long long>())                                   return H5Tcopy(Complex::H5T_SCALAR2_LLONG);
        if constexpr (tc::is_Scalar2_of_type<DecayType, unsigned int>())                                return H5Tcopy(Complex::H5T_SCALAR2_UINT);
        if constexpr (tc::is_Scalar2_of_type<DecayType, unsigned long>())                               return H5Tcopy(Complex::H5T_SCALAR2_ULONG);
        if constexpr (tc::is_Scalar2_of_type<DecayType, unsigned long long>())                          return H5Tcopy(Complex::H5T_SCALAR2_ULLONG);
        if constexpr (tc::is_Scalar2_of_type<DecayType, double>())                                      return H5Tcopy(Complex::H5T_SCALAR2_DOUBLE);
        if constexpr (tc::is_Scalar2_of_type<DecayType, float>())                                       return H5Tcopy(Complex::H5T_SCALAR2_FLOAT);
        if constexpr (tc::is_Scalar3_of_type<DecayType, int>())                                         return H5Tcopy(Complex::H5T_SCALAR3_INT);
        if constexpr (tc::is_Scalar3_of_type<DecayType, long>())                                        return H5Tcopy(Complex::H5T_SCALAR3_LONG);
        if constexpr (tc::is_Scalar3_of_type<DecayType, long long>())                                   return H5Tcopy(Complex::H5T_SCALAR3_LLONG);
        if constexpr (tc::is_Scalar3_of_type<DecayType, unsigned int>())                                return H5Tcopy(Complex::H5T_SCALAR3_UINT);
        if constexpr (tc::is_Scalar3_of_type<DecayType, unsigned long>())                               return H5Tcopy(Complex::H5T_SCALAR3_ULONG);
        if constexpr (tc::is_Scalar3_of_type<DecayType, unsigned long long>())                          return H5Tcopy(Complex::H5T_SCALAR3_ULLONG);
        if constexpr (tc::is_Scalar3_of_type<DecayType, double>())                                      return H5Tcopy(Complex::H5T_SCALAR3_DOUBLE);
        if constexpr (tc::is_Scalar3_of_type<DecayType, float>())                                       return H5Tcopy(Complex::H5T_SCALAR3_FLOAT);
        if constexpr (std::is_same<DecayType, Complex::H5T_COMPLEX_STRUCT<int>>::value)                 return H5Tcopy(Complex::H5T_COMPLEX_INT);
        if constexpr (std::is_same<DecayType, Complex::H5T_COMPLEX_STRUCT<long>>::value)                return H5Tcopy(Complex::H5T_COMPLEX_LONG);
        if constexpr (std::is_same<DecayType, Complex::H5T_COMPLEX_STRUCT<long long>>::value)           return H5Tcopy(Complex::H5T_COMPLEX_LLONG);
        if constexpr (std::is_same<DecayType, Complex::H5T_COMPLEX_STRUCT<unsigned int>>::value)        return H5Tcopy(Complex::H5T_COMPLEX_UINT);
        if constexpr (std::is_same<DecayType, Complex::H5T_COMPLEX_STRUCT<unsigned long>>::value)       return H5Tcopy(Complex::H5T_COMPLEX_ULONG);
        if constexpr (std::is_same<DecayType, Complex::H5T_COMPLEX_STRUCT<unsigned long long>>::value)  return H5Tcopy(Complex::H5T_COMPLEX_ULLONG);
        if constexpr (std::is_same<DecayType, Complex::H5T_COMPLEX_STRUCT<double>>::value)              return H5Tcopy(Complex::H5T_COMPLEX_DOUBLE);
        if constexpr (std::is_same<DecayType, Complex::H5T_COMPLEX_STRUCT<float>>::value)               return H5Tcopy(Complex::H5T_COMPLEX_FLOAT);
        if constexpr (std::is_array<DataType>::value)                                                   return getDataType<typename std::remove_all_extents<DataType>::type>();
        if constexpr (std::is_array<DecayType>::value)                                                  return getDataType<typename std::remove_all_extents<DataType>::type>();
        if constexpr (tc::is_eigen_type<DataType>::value)                                               return getDataType<typename DataType::Scalar>();
        if constexpr (tc::is_std_vector<DataType>::value)                                               return getDataType<typename DataType::value_type>();
        if constexpr (tc::hasMember_Scalar <DataType>::value)                                           return getDataType<typename DataType::Scalar>();
        if constexpr (tc::hasMember_value_type <DataType>::value)                                       return getDataType<typename DataType::value_type>();
        /* clang-format on */
        spdlog::critical("getDataType could not match the type provided: {}", Type::Check::type_name<DataType>());
        throw std::logic_error("getDataType could not match the type provided: " + std::string(Type::Check::type_name<DataType>()));
    }

    namespace Complex {
        template<typename T> hid_t createComplexType() {
            hid_t NEW_COMPLEX_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(H5T_COMPLEX_STRUCT<T>));
            H5Tinsert(NEW_COMPLEX_TYPE, "real", HOFFSET(H5T_COMPLEX_STRUCT<T>, real), getDataType<T>());
            H5Tinsert(NEW_COMPLEX_TYPE, "imag", HOFFSET(H5T_COMPLEX_STRUCT<T>, imag), getDataType<T>());
            return H5Tcopy(NEW_COMPLEX_TYPE);
        }

        template<typename T> hid_t createScalar2Type() {
            hid_t NEW_SCALAR2_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(H5T_SCALAR2<T>));
            H5Tinsert(NEW_SCALAR2_TYPE, "x", HOFFSET(H5T_SCALAR2<T>, x), getDataType<T>());
            H5Tinsert(NEW_SCALAR2_TYPE, "y", HOFFSET(H5T_SCALAR2<T>, y), getDataType<T>());
            return H5Tcopy(NEW_SCALAR2_TYPE);
        }

        template<typename T> hid_t createScalar3Type() {
            hid_t NEW_SCALAR3_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(H5T_SCALAR3<T>));
            H5Tinsert(NEW_SCALAR3_TYPE, "x", HOFFSET(H5T_SCALAR3<T>, x), getDataType<T>());
            H5Tinsert(NEW_SCALAR3_TYPE, "y", HOFFSET(H5T_SCALAR3<T>, y), getDataType<T>());
            H5Tinsert(NEW_SCALAR3_TYPE, "z", HOFFSET(H5T_SCALAR3<T>, z), getDataType<T>());
            return H5Tcopy(NEW_SCALAR3_TYPE);
        }

        inline void initTypes() {
            H5T_COMPLEX_INT    = createComplexType<int>();
            H5T_COMPLEX_LONG   = createComplexType<long>();
            H5T_COMPLEX_LLONG  = createComplexType<long long>();
            H5T_COMPLEX_UINT   = createComplexType<unsigned int>();
            H5T_COMPLEX_ULONG  = createComplexType<unsigned long>();
            H5T_COMPLEX_ULLONG = createComplexType<unsigned long long>();
            H5T_COMPLEX_DOUBLE = createComplexType<double>();
            H5T_COMPLEX_FLOAT  = createComplexType<float>();

            H5T_SCALAR2_INT    = createScalar2Type<int>();
            H5T_SCALAR2_LONG   = createScalar2Type<long>();
            H5T_SCALAR2_LLONG  = createScalar2Type<long long>();
            H5T_SCALAR2_UINT   = createScalar2Type<unsigned int>();
            H5T_SCALAR2_ULONG  = createScalar2Type<unsigned long>();
            H5T_SCALAR2_ULLONG = createScalar2Type<unsigned long long>();
            H5T_SCALAR2_DOUBLE = createScalar2Type<double>();
            H5T_SCALAR2_FLOAT  = createScalar2Type<float>();

            H5T_SCALAR3_INT    = createScalar3Type<int>();
            H5T_SCALAR3_LONG   = createScalar3Type<long>();
            H5T_SCALAR3_LLONG  = createScalar3Type<long long>();
            H5T_SCALAR3_UINT   = createScalar3Type<unsigned int>();
            H5T_SCALAR3_ULONG  = createScalar3Type<unsigned long>();
            H5T_SCALAR3_ULLONG = createScalar3Type<unsigned long long>();
            H5T_SCALAR3_DOUBLE = createScalar3Type<double>();
            H5T_SCALAR3_FLOAT  = createScalar3Type<float>();
        }

        inline void closeTypes() {
            H5Tclose(H5T_COMPLEX_INT);
            H5Tclose(H5T_COMPLEX_LONG);
            H5Tclose(H5T_COMPLEX_LLONG);
            H5Tclose(H5T_COMPLEX_UINT);
            H5Tclose(H5T_COMPLEX_ULONG);
            H5Tclose(H5T_COMPLEX_ULLONG);
            H5Tclose(H5T_COMPLEX_DOUBLE);
            H5Tclose(H5T_COMPLEX_FLOAT);

            H5Tclose(H5T_SCALAR2_INT);
            H5Tclose(H5T_SCALAR2_LONG);
            H5Tclose(H5T_SCALAR2_LLONG);
            H5Tclose(H5T_SCALAR2_UINT);
            H5Tclose(H5T_SCALAR2_ULONG);
            H5Tclose(H5T_SCALAR2_ULLONG);
            H5Tclose(H5T_SCALAR2_DOUBLE);
            H5Tclose(H5T_SCALAR2_FLOAT);

            H5Tclose(H5T_SCALAR3_INT);
            H5Tclose(H5T_SCALAR3_LONG);
            H5Tclose(H5T_SCALAR3_LLONG);
            H5Tclose(H5T_SCALAR3_UINT);
            H5Tclose(H5T_SCALAR3_ULONG);
            H5Tclose(H5T_SCALAR3_ULLONG);
            H5Tclose(H5T_SCALAR3_DOUBLE);
            H5Tclose(H5T_SCALAR3_FLOAT);
        }
    }
}
