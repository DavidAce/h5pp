#pragma once
#include "h5ppHid.h"
#include "h5ppOptional.h"
#include <array>
#include <complex>
#include <H5Tpublic.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
/*!
 * \brief A collection of type-detection utilities using SFINAE
 */
namespace h5pp::type::sfinae {

    // SFINAE detection
    template<typename...>
    struct print_type_and_quit_compile_time;

    template<typename T>
    [[deprecated]] inline constexpr void print_type_and_continue_compile_time([[maybe_unused]] const char *msg = nullptr) {}

    // helper constant for static asserts
    template<class>
    inline constexpr bool always_false_v = false;
    template<class>
    inline constexpr bool unrecognized_type_v = false;
    template<class>
    inline constexpr bool invalid_type_v = false;

    template<class T, class... Ts>
    struct is_any : std::disjunction<std::is_same<T, Ts>...> {};
    template<class T, class... Ts>
    inline constexpr bool is_any_v = is_any<T, Ts...>::value;

    template<class T, class... Ts>
    struct are_same : std::conjunction<std::is_same<T, Ts>...> {};
    template<class T, class... Ts>
    inline constexpr bool are_same_v = are_same<T, Ts...>::value;

    template<typename T, typename = std::void_t<>>
    struct has_size : public std::false_type {};
    template<typename T>
    struct has_size<T, std::void_t<decltype(std::declval<T>().size())>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_size_v = has_size<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_resize0 : public std::false_type {};
    template<typename T>
    struct has_resize0<T, std::void_t<decltype(std::declval<T>().resize())>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_resize0_v = has_resize0<T>::value;

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

    template<typename T, auto rank, typename = std::void_t<>>
    struct has_resizeN : public std::false_type {};
    template<typename T, auto rank>
    struct has_resizeN<T, rank, std::void_t<decltype(std::declval<T>().resize(std::declval<std::array<long, rank>>()))>>
        : public std::true_type {};
    template<typename T, auto rank>
    inline constexpr bool has_resizeN_v = has_resizeN<T, rank>::value;

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
    struct has_vlen_type : public std::false_type {};
    template<typename T>
    struct has_vlen_type<T, std::void_t<typename T::vlen_type>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_vlen_type_v = has_vlen_type<T>::value;

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
    struct has_NumIndices<T, std::void_t<decltype(T::NumIndices)>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_NumIndices_v = has_NumIndices<T>::value;

    template<typename T, typename = std::void_t<>>
    struct has_rank : public std::false_type {};
    template<typename T>
    struct has_rank<T, std::void_t<decltype(std::declval<T>().rank())>> : public std::true_type {};
    template<typename T>
    inline constexpr bool has_rank_v = has_rank<T>::value;

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
    template<typename T, template<typename...> class Ref>
    inline constexpr bool is_specialization_v = is_specialization<T, Ref>::value;

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

    template<typename T, typename = std::void_t<>>
    struct is_iterable : public std::false_type {};
    template<typename T>
    struct is_iterable<T, std::void_t<decltype(std::begin(std::declval<T>())), decltype(std::end(std::declval<T>()))>>
        : public std::true_type {};
    template<typename T>
    inline constexpr bool is_iterable_v = is_iterable<T>::value;

    template<typename T>
    struct is_integral_iterable {
        private:
        template<typename U>
        static constexpr bool test() {
            if constexpr(is_iterable_v<U> and has_value_type_v<U>) return std::is_integral_v<typename T::value_type>;
            else return false;
        }

        public:
        static constexpr bool value = test<T>();
    };
    template<typename T>
    inline constexpr bool is_integral_iterable_v = is_integral_iterable<T>::value;

    template<typename T>
    struct is_integral_iterable_or_num {
        private:
        template<typename U>
        static constexpr bool test() {
            if constexpr(is_integral_iterable_v<T>) return true;
            return std::is_integral_v<T>;
        }

        public:
        static constexpr bool value = test<T>();
    };

    template<typename T>
    inline constexpr bool is_integral_iterable_or_num_v = is_integral_iterable_or_num<T>::value;

    template<typename T>
    inline constexpr bool is_iterable_or_num_v = is_iterable_v<T> or std::is_arithmetic_v<T>;

    template<typename T>
    inline constexpr bool is_integral_iterable_num_or_nullopt_v = is_integral_iterable_or_num_v<T> or std::is_same_v<T, std::nullopt_t>;

    template<typename T>
    using enable_if_is_integral_iterable = std::enable_if_t<is_integral_iterable_v<T>>;

    template<typename T>
    using enable_if_is_integral_iterable_or_num = std::enable_if_t<is_integral_iterable_or_num_v<T>>;

    template<typename T>
    using enable_if_is_integral_iterable_num_or_nullopt = std::enable_if_t<is_integral_iterable_num_or_nullopt_v<T>>;

    template<typename T>
    using enable_if_is_iterable_or_nullopt = std::enable_if_t<is_iterable_v<T> or std::is_same_v<T, std::nullopt_t>>;

    // Reminder: enable_if is used to help overload resolution. static_assert to constrain types in template contexts

    template<typename T>
    inline constexpr bool is_h5pp_loc_id = is_any_v<T, hid::h5f, hid::h5g, hid::h5o>;

    template<typename T>
    inline constexpr bool is_hdf5_loc_id = is_any_v<T, hid::h5f, hid::h5g, hid::h5o, hid_t>;

    template<typename T>
    inline constexpr bool is_hdf5_obj_id = is_any_v<T, hid::h5f, hid::h5g, hid::h5d, hid::h5o, hid_t>; // To call H5Iget_file_id

    template<typename T>
    inline constexpr bool is_h5pp_link_id = is_any_v<T, hid::h5d, hid::h5g, hid::h5o>;

    template<typename T>
    inline constexpr bool is_hdf5_link_id = is_any_v<T, hid::h5d, hid::h5g, hid::h5o, hid_t>;

    template<typename T>
    inline constexpr bool is_h5pp_type_id = std::is_same_v<T, hid::h5t>;

    template<typename T>
    inline constexpr bool is_hdf5_type_id = is_any_v<T, hid::h5t, hid_t>;

    template<typename T>
    inline constexpr bool is_h5pp_space_id = std::is_same_v<T, hid::h5s>;

    template<typename T>
    inline constexpr bool is_hdf5_space_id = is_any_v<T, hid::h5s, hid_t>;

    template<typename T>
    inline constexpr bool is_h5pp_id =
        is_any_v<T, hid::h5d, hid::h5g, hid::h5o, hid::h5a, hid::h5s, hid::h5t, hid::h5f, hid::h5p, hid::h5e>;

    template<typename T>
    inline constexpr bool is_hdf5_id =
        is_any_v<T, hid::h5d, hid::h5g, hid::h5o, hid::h5a, hid::h5s, hid::h5t, hid::h5f, hid::h5p, hid::h5e, hid_t>;

    template<typename T>
    struct is_text {
        private:
        template<typename U>
        static constexpr bool test() {
            using DecayType = typename std::decay<U>::type;
            // No support for wchar_t, char16_t and char32_t
            if constexpr(has_c_str_v<DecayType>) return true;
            if constexpr(std::is_same_v<DecayType, std::string>) return true;
            if constexpr(std::is_same_v<DecayType, std::string_view>) return true;
            if constexpr(std::is_same_v<DecayType, const char *>) return true;
            if constexpr(std::is_same_v<DecayType, const char[]>) return true;
            if constexpr(std::is_same_v<DecayType, char *>) return true;
            if constexpr(std::is_same_v<DecayType, char[]>) return true;
            if constexpr(std::is_same_v<DecayType, char>) return true;
            else return false;
        }

        public:
        static constexpr bool value = test<T>();
    };

    template<typename T>
    inline constexpr bool is_text_v = is_text<T>::value;

    template<typename T>
    struct has_text {
        private:
        template<typename U>
        static constexpr bool test() {
            using DecayType = typename std::decay<U>::type;
            if constexpr(is_text_v<U>) return false;
            if constexpr(std::is_array_v<DecayType>) return is_text_v<typename std::remove_all_extents_t<DecayType>>;
            if constexpr(std::is_pointer_v<DecayType>) return is_text_v<typename std::remove_pointer_t<DecayType>>;
            if constexpr(has_value_type_v<DecayType>) return is_text_v<typename DecayType::value_type>;
            return false;
        }

        public:
        static constexpr bool value = test<T>();
    };
    template<typename T>
    inline constexpr bool has_text_v = has_text<T>::value;

    template<typename Outer, typename Inner>
    struct is_container_of {
        private:
        template<typename O, typename I>
        static constexpr bool test() {
            if constexpr(is_iterable_v<O>) {
                if constexpr(has_value_type_v<O>) {
                    using I_lhs = typename std::decay_t<typename O::value_type>;
                    using I_rhs = typename std::decay_t<I>;
                    return std::is_same_v<I_lhs, I_rhs>;
                }
            }
            return false;
        }

        public:
        static constexpr bool value = test<Outer, Inner>();
    };

    template<typename Outer, typename Inner>
    inline constexpr bool is_container_of_v = is_container_of<Outer, Inner>::value;

    template<typename T>
    struct is_std_complex : public std::false_type {};
    template<typename T>
    struct is_std_complex<std::complex<T>> : public std::true_type {};
    template<typename T>
    inline constexpr bool is_std_complex_v = is_std_complex<T>::value;

    template<typename T>
    struct has_std_complex {
        private:
        static constexpr bool test() {
            if constexpr(h5pp::type::sfinae::has_value_type_v<T>) return h5pp::type::sfinae::is_std_complex_v<typename T::value_type>;
            else if constexpr(h5pp::type::sfinae::has_Scalar_v<T>) return h5pp::type::sfinae::is_std_complex_v<typename T::Scalar>;
            else return false;
        }
        public:
        static constexpr bool value = test();
    };

    template<typename T>
    inline constexpr bool has_std_complex_v = has_std_complex<T>::value;


    template<typename T>
    struct is_Scalar2 {
        private:
        static constexpr bool test() {
            if constexpr(has_x_v<T> and has_y_v<T> and not has_z_v<T>) {
                constexpr size_t t_size    = sizeof(T);
                constexpr size_t x_size    = sizeof(T::x);
                constexpr size_t y_size    = sizeof(T::y);
                constexpr bool   same_type = std::is_same_v<decltype(T::x), decltype(T::y)>;
                return same_type and t_size == x_size + y_size;
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
    using get_Scalar2_t = std::conditional_t<is_Scalar2_v<T>, decltype(T::x), std::false_type>;

    template<typename T1, typename T2>
    constexpr bool is_Scalar2_of_type() {
        if constexpr(is_Scalar2_v<T1>) return std::is_same<decltype(T1::x), T2>::value;
        return false;
    }
    template<typename T>
    struct is_Scalar3 {
        private:
        static constexpr bool test() {
            if constexpr(has_x_v<T> and has_y_v<T> and has_z_v<T>) {
                constexpr size_t t_size = sizeof(T);
                constexpr size_t x_size = sizeof(T::x);
                constexpr size_t y_size = sizeof(T::y);
                constexpr size_t z_size = sizeof(T::z);
                constexpr bool   same_type =
                    std::is_same_v<decltype(T::x), decltype(T::y)> and std::is_same_v<decltype(T::x), decltype(T::z)>;
                return same_type and t_size == x_size + y_size + z_size;
            } else {
                return false;
            }
        }

        public:
        static constexpr bool value = test();
    };

    template<typename T>
    inline constexpr bool is_Scalar3_v = is_Scalar3<T>::value;

    template<typename T>
    using get_Scalar3_t = std::conditional_t<is_Scalar3_v<T>, decltype(T::x), std::false_type>;

    template<typename T>
    constexpr bool is_ScalarN_v = is_Scalar2_v<T> or is_Scalar3_v<T>;

    template<typename T>
    using get_ScalarN_t = std::conditional_t<is_ScalarN_v<T>, decltype(T::x), std::false_type>;

    template<typename T1, typename T2>
    constexpr bool is_Scalar3_of_type() {
        if constexpr(is_Scalar3_v<T1>) return std::is_same<decltype(T1::x), T2>::value;
        else return false;
    }

    template<typename T>
    struct has_Scalar2 {
        private:
        static constexpr bool test() {
            if constexpr(h5pp::type::sfinae::has_value_type_v<T>) return h5pp::type::sfinae::is_Scalar2_v<typename T::value_type>;
            else if constexpr(h5pp::type::sfinae::has_Scalar_v<T>) return h5pp::type::sfinae::is_Scalar2_v<typename T::Scalar>;
            else return false;
        }

        public:
        static constexpr bool value = test();
    };

    template<typename T>
    inline constexpr bool has_Scalar2_v = has_Scalar2<T>::value;

    template<typename T>
    struct has_Scalar3 {
        private:
        static constexpr bool test() {
            if constexpr(h5pp::type::sfinae::has_value_type_v<T>) return h5pp::type::sfinae::is_Scalar3_v<typename T::value_type>;
            else if constexpr(h5pp::type::sfinae::has_Scalar_v<T>) return h5pp::type::sfinae::is_Scalar3_v<typename T::Scalar>;
            else return false;
        }

        public:
        static constexpr bool value = test();
    };

    template<typename T>
    inline constexpr bool has_Scalar3_v = has_Scalar3<T>::value;

    template<typename T>
    inline constexpr bool has_ScalarN_v = has_Scalar2<T>::value or has_Scalar3<T>::value;

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
    constexpr auto value_type_name() {
        if constexpr(has_value_type_v<T>) return type_name<typename T::value_type>();
        else if constexpr(has_Scalar_v<T>) return type_name<typename T::Scalar>();
        else return type_name<T>();
    }
}
