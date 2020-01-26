#pragma once
#include "h5ppEigen.h"
#include "h5ppLogger.h"
#include "h5ppTypeCompound.h"
#include <type_traits>

/*!
 * \brief A collection of type-detection and type-analysis utilities using SFINAE
 */
namespace h5pp::type::sfinae {

    // SFINAE detection

    template<typename T, typename = std::void_t<>>
    struct has_size : public std::false_type {};
    template<typename T>
    struct has_size<T, std::void_t<decltype(std::declval<T>().size())>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_size_v = has_size<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_resize : public std::false_type {};
    template<typename T>
    struct has_resize<T, std::void_t<decltype(std::declval<T>().resize(0))>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_resize_v = has_resize<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_resize2 : public std::false_type {};
    template<typename T>
    struct has_resize2<T, std::void_t<decltype(std::declval<T>().resize(0, 0))>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_resize2_v = has_resize2<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_data : public std::false_type {};
    template<typename T>
    struct has_data<T, std::void_t<decltype(std::declval<T>().data())>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_data_v = has_data<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_value_type : public std::false_type {};
    template<typename T>
    struct has_value_type<T, std::void_t<typename T::value_type>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_value_type_v = has_value_type<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_c_str : public std::false_type {};
    template<typename T>
    struct has_c_str<T, std::void_t<decltype(std::declval<T>().c_str())>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_c_str_v = has_c_str<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_imag : public std::false_type {};
    template<typename T>
    struct has_imag<T, std::void_t<decltype(std::declval<T>().imag())>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_imag_v = has_imag<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_Scalar : public std::false_type {};
    template<typename T>
    struct has_Scalar<T, std::void_t<typename T::Scalar>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_Scalar_v = has_Scalar<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_NumIndices : public std::false_type {};
    template<typename T>
    struct has_NumIndices<T, std::void_t<decltype(std::declval<T>().NumIndices)>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_NumIndices_v = has_NumIndices<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_dimensions : public std::false_type {};
    template<typename T>
    struct has_dimensions<T, std::void_t<decltype(std::declval<T>().dimensions())>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_dimensions_v = has_dimensions<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_x : public std::false_type {};
    template<typename T>
    struct has_x<T, std::void_t<decltype(std::declval<T>().x)>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_x_v = has_x<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_y : public std::false_type {};
    template<typename T>
    struct has_y<T, std::void_t<decltype(std::declval<T>().y)>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_y_v = has_y<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_z : public std::false_type {};
    template<typename T>
    struct has_z<T, std::void_t<decltype(std::declval<T>().z)>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_z_v = has_z<T>::value;

    template<typename Test, template<typename...> class Ref>
    struct is_specialization : std::false_type {};
    template<template<typename...> class Ref, typename... Args>
    struct is_specialization<Ref<Args...>, Ref> : std::true_type {};

    template<typename T>
    struct is_std_vector : public std::false_type {};
    template<typename T>
    struct is_std_vector<std::vector<T>> : public std::true_type {};
    template<typename T>
    inline constexpr bool is_std_vector_v = is_std_vector<T>::value;

    template<typename T>
    struct is_std_array : public std::false_type {};
    template<typename T, auto N>
    struct is_std_array<std::array<T, N>> : public std::true_type {};
    template<typename T>
    inline constexpr bool is_std_array_v = is_std_array<T>::value;

    template<typename T>
    struct is_text {
        private:
        template<typename U>
        static constexpr bool test() {
            using DecayType = typename std::decay<U>::type;
            if constexpr(std::is_array_v<DecayType>) return test<typename std::remove_all_extents_t<DecayType>>();
            if constexpr(std::is_pointer_v<DecayType>) return test<typename std::remove_pointer_t<DecayType>>();
            if constexpr(std::is_const_v<DecayType>) return test<typename std::remove_cv_t<DecayType>>();
            if constexpr(std::is_reference_v<DecayType>) return test<typename std::remove_reference_t<DecayType>>();
            if constexpr(h5pp::type::sfinae::has_value_type_v<DecayType>) return test<typename DecayType::value_type>();
            return std::is_same<DecayType, char>::value; // No support for wchar_t, char16_t and char32_t
        }

        public:
        static constexpr bool value = test<T>();
    };
    template<typename T>
    inline constexpr bool is_text_v = is_text<T>::value;

    template<typename...>
    struct print_type_and_exit_compile_time;

    template<typename T>
    constexpr auto type_name() {
        std::string_view name, prefix, suffix;
#ifdef __clang__
        name   = __PRETTY_FUNCTION__;
        prefix = "auto h5pp::type::sfinae::type_name() [T = ";
        suffix = "]";
#elif defined(__GNUC__)
        name   = __PRETTY_FUNCTION__;
        prefix = "constexpr auto h5pp::type::sfinae::type_name() [with T = ";
        suffix = "]";
#elif defined(_MSC_VER)
        name   = __FUNCSIG__;
        prefix = "auto __cdecl h5pp::type::sfinae::type_name<";
        suffix = ">(void)";
#endif
        name.remove_prefix(prefix.size());
        name.remove_suffix(suffix.size());
        return name;
    }

    template<typename T>
    struct is_std_complex : public std::false_type {};
    template<typename T>
    struct is_std_complex<std::complex<T>> : public std::true_type {};
    template<typename T>
    inline constexpr bool is_std_complex_v = is_std_complex<T>::value;

    template<typename T>
    struct is_Scalar2 {
        private:
        static constexpr bool test() {
            if constexpr(has_x_v<T> and has_y_v<T> and not has_z_v<T>) {
                constexpr size_t t_size = sizeof(T);
                constexpr size_t x_size = sizeof(T::x);
                constexpr size_t y_size = sizeof(T::y);
                return t_size == x_size + y_size;
            } else {
                return false;
            }
        }

        public:
        static constexpr bool value = test();
    };
    template<typename T>
    inline constexpr bool is_Scalar2_v = is_Scalar2<T>::value;

    template<typename T>
    struct is_Scalar3 {
        private:
        static constexpr bool test() {
            if constexpr(has_x_v<T> and has_y_v<T> and has_z_v<T>) {
                constexpr size_t t_size = sizeof(T);
                constexpr size_t x_size = sizeof(T::x);
                constexpr size_t y_size = sizeof(T::y);
                constexpr size_t z_size = sizeof(T::z);
                return t_size == x_size + y_size + z_size;
            } else {
                return false;
            }
        }

        public:
        static constexpr bool value = test();
    };
    template<typename T>
    inline constexpr bool is_Scalar3_v = is_Scalar3<T>::value;

    template<typename T1, typename T2>
    constexpr bool is_Scalar2_of_type() {
        if constexpr(is_Scalar2_v<T1>) return std::is_same<decltype(T1::x), T2>::value;
        return false;
    }

    template<typename T>
    constexpr bool is_ScalarN() {
        return is_Scalar2_v<T> or is_Scalar3_v<T>;
    }

    template<typename T1, typename T2>
    constexpr bool is_Scalar3_of_type() {
        if constexpr(is_Scalar3_v<T1>)
            return std::is_same<decltype(T1::x), T2>::value;
        else
            return false;
    }

#ifdef H5PP_EIGEN3

    template<typename T>
    using is_eigen_matrix = std::is_base_of<Eigen::MatrixBase<std::decay_t<T>>, std::decay_t<T>>;
    template<typename T>
    inline constexpr bool is_eigen_matrix_v = is_eigen_matrix<T>::value;
    template<typename T>
    using is_eigen_array = std::is_base_of<Eigen::ArrayBase<std::decay_t<T>>, std::decay_t<T>>;
    template<typename T>
    inline constexpr bool is_eigen_array_v = is_eigen_array<T>::value;
    template<typename T>
    using is_eigen_tensor = std::is_base_of<Eigen::TensorBase<std::decay_t<T>, Eigen::ReadOnlyAccessors>, std::decay_t<T>>;
    template<typename T>
    inline constexpr bool is_eigen_tensor_v = is_eigen_tensor<T>::value;
    template<typename T>
    using is_eigen_dense = std::is_base_of<Eigen::DenseBase<std::decay_t<T>>, std::decay_t<T>>;
    template<typename T>
    inline constexpr bool is_eigen_dense_v = is_eigen_dense<T>::value;
    template<typename T>
    using is_eigen_map = std::is_base_of<Eigen::MapBase<std::decay_t<T>, Eigen::ReadOnlyAccessors>, std::decay_t<T>>;
    template<typename T>
    inline constexpr bool is_eigen_map_v = is_eigen_map<T>::value;
    template<typename T>
    using is_eigen_plain = std::is_base_of<Eigen::PlainObjectBase<std::decay_t<T>>, std::decay_t<T>>;
    template<typename T>
    inline constexpr bool is_eigen_plain_v = is_eigen_plain<T>::value;
    template<typename T>
    using is_eigen_base = std::is_base_of<Eigen::EigenBase<std::decay_t<T>>, std::decay_t<T>>;
    template<typename T>
    inline constexpr bool is_eigen_base_v = is_eigen_base<T>::value;
    template<typename T>
    struct is_eigen_core : public std::false_type {};
    template<typename T, int rows, int cols, int StorageOrder>
    struct is_eigen_core<Eigen::Matrix<T, rows, cols, StorageOrder>> : public std::true_type {};
    template<typename T, int rows, int cols, int StorageOrder>
    struct is_eigen_core<Eigen::Array<T, rows, cols, StorageOrder>> : public std::true_type {};
    template<typename T>
    inline constexpr bool is_eigen_core_v = is_eigen_core<T>::value;
    template<typename T>
    struct is_eigen_any {
        static constexpr bool value = is_eigen_base<T>::value or is_eigen_tensor<T>::value;
    };
    template<typename T>
    inline constexpr bool is_eigen_any_v = is_eigen_any<T>::value;
    template<typename T>
    struct is_eigen_contiguous {
        static constexpr bool value = is_eigen_any<T>::value and has_data<T>::value;
    };
    template<typename T>
    inline constexpr bool is_eigen_contiguous_v = is_eigen_contiguous<T>::value;
    template<typename T>
    class is_eigen_1d {
        private:
        template<typename U>
        static constexpr auto test() {
            if constexpr(is_eigen_map<U>::value) return test<typename U::PlainObject>();
            if constexpr(is_eigen_dense<U>::value) return U::RowsAtCompileTime == 1 or U::ColsAtCompileTime == 1;
            if constexpr(is_eigen_tensor<U>::value and has_NumIndices<U>::value)
                return U::NumIndices == 1;
            else
                return false;
        }

        public:
        static constexpr bool value = test<T>();
    };
    template<typename T>
    inline constexpr bool is_eigen_1d_v = is_eigen_1d<T>::value;

    template<typename T>
    class is_eigen_colmajor {
        template<typename U>
        static constexpr bool test() {
            if constexpr(is_eigen_base<U>::value) return not U::IsRowMajor;
            if constexpr(is_eigen_tensor<U>::value)
                return Eigen::ColMajor == static_cast<Eigen::StorageOptions>(U::Layout);
            else
                return false;
        }

        public:
        static constexpr bool value = test<T>();
    };
    template<typename T>
    inline constexpr bool is_eigen_colmajor_v = is_eigen_colmajor<T>::value;

    template<typename T>
    class is_eigen_rowmajor {
        template<typename U>
        static constexpr bool test() {
            if constexpr(is_eigen_base<U>::value) return U::IsRowMajor;
            if constexpr(is_eigen_tensor<U>::value)
                return Eigen::RowMajor == static_cast<Eigen::StorageOptions>(U::Layout);
            else
                return false;
        }

        public:
        static constexpr bool value = test<T>();
    };
    template<typename T>
    inline constexpr bool is_eigen_rowmajor_v = is_eigen_rowmajor<T>::value;

#endif

}
