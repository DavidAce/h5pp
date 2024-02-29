#pragma once
#include "h5ppTypeSfinae.h"
#include <string_view>
#include <variant>
#include <vector>

namespace h5pp {
    struct Indices {
        private:
        std::vector<size_t> data;

        public:
        Indices()                          = default;
        explicit Indices(char)             = delete;
        explicit Indices(const char *)     = delete;
        explicit Indices(std::string_view) = delete;
        explicit Indices(std::string &&)   = delete;
        template<typename U, typename = std::enable_if_t<h5pp::type::sfinae::is_integral_iterable_or_num_v<U>>>
        Indices(const U &num) {
            if constexpr(h5pp::type::sfinae::is_integral_iterable_v<U>) {
                if constexpr(std::is_assignable_v<std::vector<size_t>, U>) data = num;
                else if constexpr(std::is_constructible_v<std::vector<size_t>, U>) data = std::vector<size_t>(num);
                else data = std::vector<size_t>(std::begin(num), std::end(num));
            } else if constexpr(std::is_integral_v<U>) {
                data = {type::safe_cast<size_t>(num)};
            } else {
                static_assert(h5pp::type::sfinae::invalid_type_v<U>, "Unrecognized index type");
                throw h5pp::runtime_error("Could not identify index type: {}", h5pp::type::sfinae::type_name<U>());
            }
        }
        template<typename U, typename = std::enable_if_t<std::is_integral_v<U>>>
        Indices(std::initializer_list<U> &&il) : data(il) {}

        operator std::vector<size_t> &() { return data; }
        operator const std::vector<size_t> &() const { return data; }

        [[nodiscard]] auto empty() const { return data.empty(); }
    };
#if !defined(NDEBUG)
    static_assert(std::is_constructible_v<Indices, std::vector<long>>);
    static_assert(std::is_constructible_v<Indices, std::initializer_list<int>>);
    static_assert(std::is_constructible_v<Indices, std::array<size_t, 3>>);
#endif

    struct Names {
        private:
        std::vector<std::string> data;

        public:
        Names() = default;
        template<typename U, typename = std::enable_if_t<h5pp::type::sfinae::is_text_v<U> or h5pp::type::sfinae::has_text_v<U>>>
        Names(const U &str) {
            static_assert(h5pp::type::sfinae::is_text_v<U> or h5pp::type::sfinae::has_text_v<U>);
            if constexpr(h5pp::type::sfinae::is_text_v<U>) {
                data = {std::string(str)};
            } else if constexpr(std::is_assignable_v<std::vector<std::string>, U>) {
                data = str;
            } else if constexpr(std::is_constructible_v<std::vector<std::string>, U>) {
                data = std::vector<std::string>(str);
            } else if constexpr(h5pp::type::sfinae::has_text_v<U> and h5pp::type::sfinae::is_iterable_v<U>) {
                data = std::vector<std::string>(std::begin(str), std::end(str));
            } else {
                static_assert(h5pp::type::sfinae::invalid_type_v<U>, "Unrecognized text list type");
                throw h5pp::runtime_error("Could not identify text list type: {}", h5pp::type::sfinae::type_name<U>());
            }
        }
        template<typename U, typename = std::enable_if_t<h5pp::type::sfinae::is_text_v<U>>>
        Names(const std::initializer_list<U> &il) {
            data = std::vector<std::string>(std::begin(il), std::end(il));
        }

        operator std::vector<std::string> &() { return data; }
        operator const std::vector<std::string> &() const { return data; }
        [[nodiscard]] auto empty() const { return data.empty(); }
    };

#if !defined(NDEBUG)
    static_assert(std::is_constructible_v<Names, std::initializer_list<std::string>>);
    static_assert(std::is_constructible_v<Names, std::initializer_list<std::string_view>>);
    static_assert(std::is_constructible_v<Names, std::initializer_list<char *>>);
    static_assert(std::is_constructible_v<Names, std::initializer_list<const char *>>);
    static_assert(std::is_constructible_v<Names, std::vector<char *>>);
    static_assert(std::is_constructible_v<Names, std::vector<const char *>>);
    static_assert(std::is_constructible_v<Names, std::string>);
    static_assert(std::is_constructible_v<Names, std::string_view>);
    static_assert(std::is_constructible_v<Names, const char *>);
    static_assert(std::is_constructible_v<Names, const char[]>);
    static_assert(std::is_constructible_v<Names, char>);
#endif

    struct NamesOrIndices {
        private:
        h5pp::Names   names;
        h5pp::Indices indices;

        public:
        template<typename T>
        NamesOrIndices(const T &data_) {
            if constexpr(std::is_constructible_v<Names, T>) names = Names(data_);
            else if constexpr(std::is_constructible_v<Indices, T>) indices = Indices(data_);
            else static_assert(h5pp::type::sfinae::invalid_type_v<T>, "Unrecognized type for indices or names");
        }
        template<typename T>
        NamesOrIndices(const std::initializer_list<T> &data_) {
            if constexpr(std::is_constructible_v<Names, std::initializer_list<T>>) names = Names(data_);
            else if constexpr(std::is_constructible_v<Indices, std::initializer_list<T>>) indices = Indices(data_);
            else static_assert(h5pp::type::sfinae::invalid_type_v<std::initializer_list<T>>, "Unrecognized type for indices or names");
        }

        [[nodiscard]] auto index() const {
            if(not names.empty()) return 0;
            if(not indices.empty()) return 1;
            return -1;
        }
        [[nodiscard]] bool has_names() const { return not names.empty(); }
        [[nodiscard]] bool has_indices() const { return not indices.empty(); }

        template<auto N>
        [[nodiscard]] const auto &get_value() const {
            if constexpr(N == 0) return names;
            else if constexpr(N == 1) return indices;
        }
        [[nodiscard]] const std::vector<size_t>      &get_indices() const { return indices; }
        [[nodiscard]] const std::vector<std::string> &get_names() const { return names; }
    };
#if !defined(NDEBUG)
    static_assert(std::is_constructible_v<NamesOrIndices, std::initializer_list<std::string>>);
    static_assert(std::is_constructible_v<NamesOrIndices, std::initializer_list<std::string_view>>);
    static_assert(std::is_constructible_v<NamesOrIndices, std::initializer_list<char *>>);
    static_assert(std::is_constructible_v<NamesOrIndices, std::initializer_list<const char *>>);
    static_assert(std::is_constructible_v<NamesOrIndices, std::vector<char *>>);
    static_assert(std::is_constructible_v<NamesOrIndices, std::vector<const char *>>);
    static_assert(std::is_constructible_v<NamesOrIndices, std::string>);
    static_assert(std::is_constructible_v<NamesOrIndices, std::string_view>);
    static_assert(std::is_constructible_v<NamesOrIndices, const char *>);
    static_assert(std::is_constructible_v<NamesOrIndices, const char[]>);
    static_assert(std::is_constructible_v<NamesOrIndices, char>);
#endif
}
