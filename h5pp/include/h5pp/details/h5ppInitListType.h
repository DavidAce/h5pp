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
        Indices()                              = default;
        Indices(Indices &&indices)             = delete;
        explicit Indices(const char *str)      = delete;
        explicit Indices(std::string_view str) = delete;
        explicit Indices(std::string &&str)    = delete;
        template<typename U, typename = std::enable_if_t<h5pp::type::sfinae::is_integral_iterable_or_num_v<U>>>
        Indices(const U &num) {
            if constexpr(h5pp::type::sfinae::is_integral_iterable_v<U>) {
                if constexpr(std::is_assignable_v<std::vector<size_t>, U>)
                    data = num;
                else if constexpr(std::is_constructible_v<std::vector<size_t>, U>)
                    data = std::vector<size_t>(num);
                else
                    data = std::vector<size_t>(std::begin(num), std::end(num));
            } else if constexpr(std::is_integral_v<U>)
                data = {static_cast<size_t>(num)};
            else {
                static_assert(h5pp::type::sfinae::invalid_type_v<U>, "Unrecognized index type");
                throw std::runtime_error(h5pp::format("Could not identify index type: {}", h5pp::type::sfinae::type_name<U>()));
            }
        }
        template<typename U, typename = std::enable_if_t<std::is_integral_v<U>>>
        Indices(std::initializer_list<U> &&il) : data(il) {}
        operator std::vector<size_t> &() { return data; }
    };

    struct Names {
        private:
        std::vector<std::string> data;

        public:
        Names()              = default;
        Names(Names &&names) = delete;
        template<typename U, typename = std::enable_if_t<h5pp::type::sfinae::is_text_v<U> or h5pp::type::sfinae::has_text_v<U>>>
        Names(U &&str) {
            if constexpr(h5pp::type::sfinae::is_text_v<U>)
                data = {std::string(str)};
            else if constexpr(std::is_assignable_v<std::vector<std::string>, U>)
                data = str;
            else if constexpr(std::is_constructible_v<std::vector<std::string>, U>)
                data = std::vector<std::string>(str);
            else if constexpr(h5pp::type::sfinae::has_text_v<U> and h5pp::type::sfinae::is_iterable_v<U>)
                data = std::vector<std::string>(std::begin(str), std::end(str));
            else {
                static_assert(h5pp::type::sfinae::invalid_type_v<U>, "Unrecognized text list type");
                throw std::runtime_error(h5pp::format("Could not identify text list type: {}", h5pp::type::sfinae::type_name<U>()));
            }
        };
        template<typename U, typename = std::enable_if_t<h5pp::type::sfinae::is_text_v<U>>>
        Names(std::initializer_list<U> &&il) : data(il) {}
        operator std::vector<std::string> &() { return data; }
    };

    struct NamesOrIndices {
        private:
        std::variant<h5pp::Names, h5pp::Indices> data;

        public:
        template<typename T>
        NamesOrIndices(T &&data_) {
            if constexpr(std::is_constructible_v<Names, T>)
                data.emplace<0>(data_);
            else if constexpr(std::is_constructible_v<Indices, T>)
                data.emplace<1>(data_);
            else
                static_assert(h5pp::type::sfinae::invalid_type_v<T>, "Unrecognized type for indices or names");
        }

        template<typename T>
        NamesOrIndices(std::initializer_list<T> &&data_) : data(data_) {}

        operator std::variant<Names, Indices> &() { return data; }

        auto &get_variant() { return data; }
    };
}
