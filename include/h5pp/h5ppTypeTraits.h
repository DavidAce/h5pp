#pragma once
#include "h5ppOptional.h"
#include <array>
#include <complex>
#include <concepts>
#include <H5Tpublic.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace h5pp::hid {
    // Forward declarations
    class h5d;
    class h5a;
    class h5o;
    class h5t;
    class h5s;
    class h5f;
    class h5e;
    class h5g;
    class h5p;
}

/*!
 * \brief A collection of type traits and concepts
 */
namespace h5pp::traits {

    // Compile-time diagnostics
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

    template<typename T>
    constexpr auto type_name() {
        std::string_view name, prefix, suffix;
#ifdef __clang__
        name   = __PRETTY_FUNCTION__;
        prefix = "auto h5pp::traits::type_name() [T = ";
        suffix = "]";
#elif defined(__GNUC__)
        name   = __PRETTY_FUNCTION__;
        prefix = "constexpr auto h5pp::traits::type_name() [with T = ";
        suffix = "]";
#elif defined(_MSC_VER)
        name   = __FUNCSIG__;
        prefix = "auto __cdecl h5pp::traits::type_name<";
        suffix = ">(void)";
#endif
        name.remove_prefix(prefix.size());
        name.remove_suffix(suffix.size());
        return name;
    }

    template<typename T1, typename T2>
    concept type_is = std::same_as<std::remove_cvref_t<T1>, T2>;

    template<typename T, typename... Ts>
    concept is_any_v = (type_is<T, Ts> || ...);

    template<typename T, typename... Ts>
    concept are_same_v = (type_is<T, Ts> && ...);

    template<typename T>
    concept is_pointer_type = std::is_pointer_v<T>;

    template<typename FileType>
    concept is_mutable_file_v = not std::is_const_v<FileType>;

    template<typename T>
    concept is_arithmetic_v = std::integral<std::remove_cvref_t<T>> || std::floating_point<std::remove_cvref_t<T>>;

    template<typename T>
    concept is_iterable_v = requires(T m) {
        { m.begin() };
        { m.end() };
    };

    template<typename T>
    concept has_value_type_v = requires { typename T::value_type; };

    template<typename T>
    concept has_vlen_type_v = requires { typename T::vlen_type; };

    template<typename T, typename CheckT>
    concept is_container_of_v = is_iterable_v<T> && has_value_type_v<T> && type_is<typename T::value_type, CheckT>;
    //    static_assert(is_container_of_v<std::vector<double>, double>);
    //    static_assert(!is_container_of_v<std::vector<double>, int>);
    //    static_assert(is_container_of_v<std::string, char>);
    //    static_assert(!is_container_of_v<std::string, int>);

    template<template<class...> class Template, class... Args>
    void is_specialization_impl(const Template<Args...> &);
    template<class T, template<class...> class Template>
    concept is_specialization_v = requires(const T &t) { is_specialization_impl<Template>(t); };

    template<typename T>
    concept is_std_optional_v = is_specialization_v<T, std::optional>;

    template<typename T>
    concept is_std_complex_v = is_specialization_v<T, std::complex>;

    template<typename T>
    concept is_std_vector_v = is_specialization_v<T, std::vector>;

    template<typename T>
    concept is_std_array_v = type_is<T, std::array<std::remove_cvref_t<typename T::value_type>, std::tuple_size<T>::value>>;

    template<typename T, typename T2>
    concept is_pair_v = is_specialization_v<T, std::pair>;

    template<typename T>
    concept has_data_v = requires(T m) {
        { m.data() } -> is_pointer_type;
    };

    template<typename T>
    concept has_size_v = requires(T m) {
        { m.size() } -> std::convertible_to<std::size_t>;
    };

    template<typename T>
    concept has_resize0_v = requires(T m) {
        { m.resize() } -> std::same_as<void>;
    };

    template<typename T>
    concept has_resize_v = requires(T m) {
        { m.resize(0) } -> std::same_as<void>;
    };

    template<typename T>
    concept has_resize2_v = requires(T m) {
        { m.resize(0, 0) } -> std::same_as<void>;
    };

    template<typename T, auto rank>
    concept has_resizeN_v = requires(T m) {
        { m.resize(std::array<long, rank>{}) } -> std::same_as<void>;
    };

    template<typename T>
    concept has_c_str_v = requires(T m) {
        { m.c_str() } -> is_pointer_type;
        { m.c_str() } -> std::same_as<const char *>;
    };

    template<typename T>
    concept has_imag_v = requires(T m) {
        { m.imag() } -> is_arithmetic_v;
    };

    template<typename T>
    concept has_Scalar_v = requires { typename T::Scalar; };

    template<typename T>
    concept has_std_complex_v =
        (has_value_type_v<T> && is_std_complex_v<typename T::value_type>) || (has_Scalar_v<T> && is_std_complex_v<typename T::Scalar>);
    //    static_assert(!has_std_complex_v<std::complex<double>>);
    //    static_assert(has_std_complex_v<std::vector<std::complex<double>>>);
    //    static_assert(has_std_complex_v<Eigen::MatrixXcd>);
    //    static_assert(!has_std_complex_v<Eigen::MatrixXd>);

    template<typename T>
    concept has_NumIndices_v = requires { T::NumIndices; };

    template<typename T>
    concept has_rank_v = requires(T m) {
        { m.rank() } -> std::integral;
    };

    template<typename T>
    concept has_dimensions_v = requires(T m) {
        { m.dimensions() } -> is_std_array_v;
    };

    template<typename T>
    concept has_x_v = requires(T m) {
        { m.x };
    };
    template<typename T>
    concept has_y_v = requires(T m) {
        { m.y };
    };
    template<typename T>
    concept has_z_v = requires(T m) {
        { m.z };
    };

    template<typename T>
    concept is_integral_iterable_v = is_iterable_v<T> && std::integral<typename T::value_type>;

    template<typename T>
    concept is_integral_iterable_or_num_v = is_integral_iterable_v<T> || std::integral<T>;
    //    static_assert(is_integral_iterable_or_num_v<std::vector<int>>);
    //    static_assert(is_integral_iterable_or_num_v<int>);
    //    static_assert(!is_integral_iterable_or_num_v<double>);
    //    static_assert(!is_integral_iterable_or_num_v<std::vector<double>>);

    template<typename T>
    concept is_iterable_or_num_v = is_iterable_v<T> || std::integral<typename T::value_type>;

    template<typename T>
    concept is_integral_iterable_num_or_nullopt_v = is_integral_iterable_or_num_v<T> || type_is<T, std::nullopt_t>;

    template<typename T>
    concept is_text_v = has_c_str_v<T> || std::convertible_to<T, std::string_view> || std::same_as<T, char>;
    //    static_assert(is_text_v<std::string>);
    //    static_assert(is_text_v<std::string_view>);
    //    static_assert(is_text_v<char *>);
    //    static_assert(is_text_v<char []>);
    //    static_assert(is_text_v<char>);

    template<typename T>
    concept has_text_v = !is_text_v<T> && (is_text_v<typename std::remove_all_extents_t<T>> ||
                                           is_text_v<typename std::remove_pointer_t<T>> || is_text_v<typename T::value_type>);
    //    static_assert(has_text_v<std::vector<std::string>>);
    //    static_assert(has_text_v<std::array<std::string_view, 0>>);
    //    static_assert(has_text_v<std::string_view[]>);

    //    template<typename T>
    //    concept is_Scalar2_v = has_x_v<T> && has_y_v<T> && (sizeof(T) == sizeof(T::x) + sizeof(T::y));

    template<typename T>
    concept is_Scalar2_v = requires(T m) {
        { m.x } -> is_arithmetic_v;
        { m.y } -> is_arithmetic_v;
        requires type_is<decltype(T::x), decltype(T::y)>;
        requires sizeof(T) == sizeof(T::x) + sizeof(T::y);
    };
    template<typename T>
    concept is_Scalar3_v = requires(T m) {
        { m.x } -> is_arithmetic_v;
        { m.y } -> is_arithmetic_v;
        { m.z } -> is_arithmetic_v;
        requires type_is<decltype(m.x), decltype(m.y)>;
        requires type_is<decltype(m.x), decltype(m.z)>;
        requires sizeof(T) == sizeof(T::x) + sizeof(T::y) + sizeof(T::z);
    };
    //    struct Field2 {
    //        double x, y;
    //    };
    //    struct Field3 {
    //        double x, y, z;
    //    };
    //    static_assert(is_Scalar2_v<Field2>);
    //    static_assert(!is_Scalar3_v<Field2>);
    //    static_assert(!is_Scalar2_v<Field3>);
    //    static_assert(is_Scalar3_v<Field3>);

    template<typename O, typename I>
    concept is_Scalar2_of_type = is_Scalar2_v<O> && std::same_as<I, decltype(O::x)>;

    template<typename O, typename I>
    concept is_Scalar3_of_type = is_Scalar3_v<O> && std::same_as<I, decltype(O::x)>;

    template<typename T>
    concept is_ScalarN_v = is_Scalar2_v<T> || is_Scalar3_v<T>;

    template<typename T>
    concept has_Scalar2_v = is_iterable_v<T> && has_value_type_v<T> && is_Scalar2_v<typename T::value_type>;
    template<typename T>
    concept has_Scalar3_v = is_iterable_v<T> && has_value_type_v<T> && is_Scalar3_v<typename T::value_type>;

    template<typename T>
    concept has_ScalarN_v = has_Scalar2_v<T> || has_Scalar3_v<T>;

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
    constexpr auto value_type_name() {
        if constexpr(has_value_type_v<T>) return type_name<typename T::value_type>();
        else if constexpr(has_Scalar_v<T>) return type_name<typename T::Scalar>();
        else return type_name<T>();
    }

    template<typename T>
    using get_Scalar2_t = std::conditional_t<is_Scalar2_v<T>, decltype(T::x), std::false_type>;

    template<typename T>
    using get_Scalar3_t = std::conditional_t<is_Scalar3_v<T>, decltype(T::x), std::false_type>;

    template<typename T>
    using get_ScalarN_t = std::conditional_t<is_ScalarN_v<T>, decltype(T::x), std::false_type>;

}
