#pragma once
#include "h5ppLogger.h"
#include "h5ppStdIsDetected.h"
#include "h5ppTextra.h"
#include "h5ppTypeCompound.h"
#include <type_traits>

namespace h5pp::Type::Scan {

    template<typename T>
    using Data_t = decltype(std::declval<T>().data());
    template<typename T>
    using Size_t = decltype(std::declval<T>().size());
    template<typename T>
    using Cstr_t = decltype(std::declval<T>().c_str());
    template<typename T>
    using Imag_t = decltype(std::declval<T>().imag());

    template<typename T>
    using NumI_t = decltype(std::declval<T>().NumIndices);

    template<typename T>
    using Dims_t = decltype(std::declval<T>().dimensions());

    template<typename T>
    using Scal_t = typename T::Scalar;

    template<typename T>
    using Valt_t = typename T::value_type;

    template<typename T>
    using x_t = decltype(std::declval<T>().x);
    template<typename T>
    using y_t = decltype(std::declval<T>().y);
    template<typename T>
    using z_t = decltype(std::declval<T>().z);

    template<typename T>
    using hasMember_data = std::experimental::is_detected<Data_t, T>;
    template<typename T>
    using hasMember_size = std::experimental::is_detected<Size_t, T>;
    template<typename T>
    using hasMember_c_str = std::experimental::is_detected<Cstr_t, T>;
    template<typename T>
    using hasMember_imag = std::experimental::is_detected<Imag_t, T>;
    template<typename T>
    using hasMember_Scalar = std::experimental::is_detected<Scal_t, T>;
    template<typename T>
    using hasMember_value_type = std::experimental::is_detected<Valt_t, T>;
    template<typename T>
    using hasMember_NumIndices = std::experimental::is_detected<NumI_t, T>;

    template<typename T>
    using hasMember_dimensions = std::experimental::is_detected<Dims_t, T>;

    template<typename T>
    using hasMember_x = std::experimental::is_detected<x_t, T>;
    template<typename T>
    using hasMember_y = std::experimental::is_detected<y_t, T>;
    template<typename T>
    using hasMember_z = std::experimental::is_detected<z_t, T>;

    template<typename Test, template<typename...> class Ref>
    struct is_specialization : std::false_type {};
    template<template<typename...> class Ref, typename... Args>
    struct is_specialization<Ref<Args...>, Ref> : std::true_type {};

    template<typename T>
    struct is_std_vector : public std::false_type {};
    template<typename T>
    struct is_std_vector<std::vector<T>> : public std::true_type {};
    template<typename T>
    struct is_std_array : public std::false_type {};
    template<typename T, auto N>
    struct is_std_array<std::array<T, N>> : public std::true_type {};

    template<typename DataType>
    constexpr bool is_text() {
        using DecayType = typename std::decay<DataType>::type;
        if constexpr(std::is_array_v<DecayType>) return is_text<typename std::remove_all_extents<DecayType>::type>();
        if constexpr(std::is_pointer_v<DecayType>) return is_text<typename std::remove_pointer<DecayType>::type>();
        if constexpr(std::is_const_v<DecayType>) return is_text<typename std::remove_cv<DecayType>::type>();
        if constexpr(std::is_reference_v<DecayType>) return is_text<typename std::remove_reference<DecayType>::type>();
        if constexpr(h5pp::Type::Scan::hasMember_value_type<DecayType>::value) return is_text<typename DecayType::value_type>();
        return std::is_same<DecayType, char>::value; // No support for wchar_t, char16_t and char32_t
    }

    template<typename...>
    struct print_type_and_exit_compile_time;

    template<typename T>
    constexpr auto type_name() {
        std::string_view name, prefix, suffix;
#ifdef __clang__
        name   = __PRETTY_FUNCTION__;
        prefix = "auto h5pp::Type::Scan::type_name() [T = ";
        suffix = "]";
#elif defined(__GNUC__)
        name   = __PRETTY_FUNCTION__;
        prefix = "constexpr auto h5pp::Type::Scan::type_name() [with T = ";
        suffix = "]";
#elif defined(_MSC_VER)
        name   = __FUNCSIG__;
        prefix = "auto __cdecl h5pp::Type::Scan::type_name<";
        suffix = ">(void)";
#endif
        name.remove_prefix(prefix.size());
        name.remove_suffix(suffix.size());
        return name;
    }

    template<typename T>
    constexpr bool is_StdComplex() {
        return is_specialization<T, std::complex>::value;
    }

    template<typename T>
    constexpr bool hasStdComplex() {
        if constexpr(is_StdComplex<T>()) return false;
        if constexpr(hasMember_Scalar<T>::value) return is_StdComplex<typename T::Scalar>();
        if constexpr(hasMember_value_type<T>::value) return is_StdComplex<typename T::value_type>();
        return false;
    }

    template<typename T>
    constexpr bool is_Scalar2() {
        if constexpr(hasMember_x<T>::value and hasMember_y<T>::value) {
            constexpr size_t t_size = sizeof(T);
            constexpr size_t x_size = sizeof(T::x);
            constexpr size_t y_size = sizeof(T::y);
            return t_size == x_size + y_size;
        } else {
            return false;
        }
    }

    template<typename T1, typename T2>
    constexpr bool is_Scalar2_of_type() {
        if constexpr(is_Scalar2<T1>()) return std::is_same<decltype(T1::x), T2>::value;
        return false;
    }

    template<typename T>
    constexpr bool hasScalar2() {
        if constexpr(is_Scalar2<T>()) return false;
        if constexpr(hasMember_Scalar<T>::value) return is_Scalar2<typename T::Scalar>();
        if constexpr(hasMember_value_type<T>::value) return is_Scalar2<typename T::value_type>();
        return false;
    }

    // Scalar 3
    template<typename T>
    constexpr bool is_Scalar3() {
        if constexpr(hasMember_x<T>::value and hasMember_y<T>::value and hasMember_z<T>::value) {
            constexpr size_t t_size = sizeof(T);
            constexpr size_t x_size = sizeof(T::x);
            constexpr size_t y_size = sizeof(T::y);
            constexpr size_t z_size = sizeof(T::z);
            return t_size == x_size + y_size + z_size;
        } else {
            return false;
        }
    }

    template<typename T>
    constexpr bool is_ScalarN() {
        return is_Scalar2<T>() or is_Scalar3<T>();
    }

    template<typename T1, typename T2>
    constexpr bool is_Scalar3_of_type() {
        if constexpr(is_Scalar3<T1>())
            return std::is_same<decltype(T1::x), T2>::value;
        else
            return false;
    }

    template<typename T>
    constexpr bool hasScalar3() {
        if constexpr(is_Scalar3<T>()) return false;
        if constexpr(hasMember_Scalar<T>::value) return is_Scalar3<typename T::Scalar>();
        if constexpr(hasMember_value_type<T>::value) return is_Scalar3<typename T::value_type>();
        return false;
    }

    template<typename T>
    constexpr bool is_H5T_COMPLEX_STRUCT() {
        return is_specialization<T, Compound::H5T_COMPLEX_STRUCT>::value;
    }
    template<typename T>
    constexpr bool is_H5T_SCALAR2() {
        return is_specialization<T, Compound::H5T_SCALAR2>::value;
    }
    template<typename T>
    constexpr bool is_H5T_SCALAR3() {
        return is_specialization<T, Compound::H5T_SCALAR3>::value;
    }
    template<typename T>
    constexpr bool isVectorOf_H5T_COMPLEX_STRUCT() {
        if constexpr(is_std_vector<T>::value and is_H5T_SCALAR2<T::value_type>())
            return true;
        else
            return false;
    }

#ifdef H5PP_EIGEN3
    template<typename T>
    struct is_eigen_tensor : public std::false_type {};
    template<typename Scalar, int rank, int storage, typename IndexType>
    struct is_eigen_tensor<Eigen::Tensor<Scalar, rank, storage, IndexType>> : public std::true_type {};
    template<typename Derived>
    struct is_eigen_tensor<Eigen::TensorMap<Derived>> : public std::true_type {};
    template<typename Derived>
    struct is_eigen_tensor<Eigen::TensorBase<Derived, Eigen::ReadOnlyAccessors>> : public std::true_type {};

    template<typename T>
    struct is_eigen_matrix : public std::false_type {};
    template<typename T, int rows, int cols, int StorageOrder>
    struct is_eigen_matrix<Eigen::Matrix<T, rows, cols, StorageOrder>> : public std::true_type {};

    template<typename T>
    struct is_eigen_array : public std::false_type {};
    template<typename T, int rows, int cols, int StorageOrder>
    struct is_eigen_array<Eigen::Array<T, rows, cols, StorageOrder>> : public std::true_type {};

    template<typename T>
    struct is_eigen_core : public std::false_type {};
    template<typename T, int rows, int cols, int StorageOrder>
    struct is_eigen_core<Eigen::Matrix<T, rows, cols, StorageOrder>> : public std::true_type {};
    template<typename T, int rows, int cols, int StorageOrder>
    struct is_eigen_core<Eigen::Array<T, rows, cols, StorageOrder>> : public std::true_type {};

    template<typename T>
    struct is_eigen_type : public std::false_type {};
    template<typename T, int rows, int cols, int StorageOrder>
    struct is_eigen_type<Eigen::Matrix<T, rows, cols, StorageOrder>> : public std::true_type {};
    template<typename T, int rows, int cols, int StorageOrder>
    struct is_eigen_type<Eigen::Array<T, rows, cols, StorageOrder>> : public std::true_type {};
    template<typename Scalar, int rank, int storage, typename IndexType>
    struct is_eigen_type<Eigen::Tensor<Scalar, rank, storage, IndexType>> : public std::true_type {};

    template<typename T>
    struct is_eigen_1d : public std::false_type {};
    template<typename T, int cols, int StorageOrder>
    struct is_eigen_1d<Eigen::Matrix<T, 1, cols, StorageOrder>> : public std::true_type {};
    template<typename T, int rows, int StorageOrder>
    struct is_eigen_1d<Eigen::Matrix<T, rows, 1, StorageOrder>> : public std::true_type {};
    template<typename T, int cols, int StorageOrder>
    struct is_eigen_1d<Eigen::Array<T, 1, cols, StorageOrder>> : public std::true_type {};
    template<typename T, int rows, int StorageOrder>
    struct is_eigen_1d<Eigen::Array<T, rows, 1, StorageOrder>> : public std::true_type {};
    template<typename Scalar, int storage, typename IndexType>
    struct is_eigen_1d<Eigen::Tensor<Scalar, 1, storage, IndexType>> : public std::true_type {};
#endif

    template<typename DataType>
    [[nodiscard]] Hid::h5t getH5DataType() {
        namespace tc = h5pp::Type::Scan;
        /* clang-format off */
        using DecayType    = typename std::decay<DataType>::type;
        if constexpr (std::is_pointer<DecayType>::value)                                                return getH5DataType<typename std::remove_pointer<DecayType>::type>();
        if constexpr (std::is_reference<DecayType>::value)                                              return getH5DataType<typename std::remove_reference<DecayType>::type>();
        if constexpr (std::is_array<DecayType>::value)                                                  return getH5DataType<typename std::remove_all_extents<DecayType>::type>();
        if constexpr (tc::is_std_vector<DecayType>::value)                                              return getH5DataType<typename DecayType::value_type>();

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
        if constexpr (std::is_same<DecayType, std::complex<int>>::value)                                return H5Tcopy(h5pp::Type::Compound::H5T_COMPLEX_INT);
        if constexpr (std::is_same<DecayType, std::complex<long>>::value)                               return H5Tcopy(h5pp::Type::Compound::H5T_COMPLEX_LONG);
        if constexpr (std::is_same<DecayType, std::complex<long long>>::value)                          return H5Tcopy(h5pp::Type::Compound::H5T_COMPLEX_LLONG);
        if constexpr (std::is_same<DecayType, std::complex<unsigned int>>::value)                       return H5Tcopy(h5pp::Type::Compound::H5T_COMPLEX_UINT);
        if constexpr (std::is_same<DecayType, std::complex<unsigned long>>::value)                      return H5Tcopy(h5pp::Type::Compound::H5T_COMPLEX_ULONG);
        if constexpr (std::is_same<DecayType, std::complex<unsigned long long>>::value)                 return H5Tcopy(h5pp::Type::Compound::H5T_COMPLEX_ULLONG);
        if constexpr (std::is_same<DecayType, std::complex<double>>::value)                             return H5Tcopy(h5pp::Type::Compound::H5T_COMPLEX_DOUBLE);
        if constexpr (std::is_same<DecayType, std::complex<float>>::value)                              return H5Tcopy(h5pp::Type::Compound::H5T_COMPLEX_FLOAT);
        if constexpr (tc::is_Scalar2_of_type<DecayType, int>())                                         return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR2_INT);
        if constexpr (tc::is_Scalar2_of_type<DecayType, long>())                                        return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR2_LONG);
        if constexpr (tc::is_Scalar2_of_type<DecayType, long long>())                                   return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR2_LLONG);
        if constexpr (tc::is_Scalar2_of_type<DecayType, unsigned int>())                                return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR2_UINT);
        if constexpr (tc::is_Scalar2_of_type<DecayType, unsigned long>())                               return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR2_ULONG);
        if constexpr (tc::is_Scalar2_of_type<DecayType, unsigned long long>())                          return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR2_ULLONG);
        if constexpr (tc::is_Scalar2_of_type<DecayType, double>())                                      return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR2_DOUBLE);
        if constexpr (tc::is_Scalar2_of_type<DecayType, float>())                                       return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR2_FLOAT);
        if constexpr (tc::is_Scalar3_of_type<DecayType, int>())                                         return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR3_INT);
        if constexpr (tc::is_Scalar3_of_type<DecayType, long>())                                        return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR3_LONG);
        if constexpr (tc::is_Scalar3_of_type<DecayType, long long>())                                   return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR3_LLONG);
        if constexpr (tc::is_Scalar3_of_type<DecayType, unsigned int>())                                return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR3_UINT);
        if constexpr (tc::is_Scalar3_of_type<DecayType, unsigned long>())                               return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR3_ULONG);
        if constexpr (tc::is_Scalar3_of_type<DecayType, unsigned long long>())                          return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR3_ULLONG);
        if constexpr (tc::is_Scalar3_of_type<DecayType, double>())                                      return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR3_DOUBLE);
        if constexpr (tc::is_Scalar3_of_type<DecayType, float>())                                       return H5Tcopy(h5pp::Type::Compound::H5T_SCALAR3_FLOAT);
        if constexpr (tc::hasMember_Scalar <DecayType>::value)                                          return getH5DataType<typename DecayType::Scalar>();
        if constexpr (tc::hasMember_value_type <DecayType>::value)                                      return getH5DataType<typename DataType::value_type>();

        /* clang-format on */
        h5pp::Logger::log->critical("getH5DataType could not match the type provided: {}", Type::Scan::type_name<DataType>());
        throw std::logic_error("getH5DataType could not match the type provided: " + std::string(Type::Scan::type_name<DataType>()) + " " +
                               std::string(Type::Scan::type_name<DecayType>()));
        return hid_t(0);
    }
}
