#pragma once
#if !defined(_MSC_VER) && __has_include(<experimental/type_traits>)
#include <experimental/type_traits>
#else
    /* https : // stackoverflow.com/questions/35661129/strange-msvc-behaviour-with-stdexperimentalis-detected */
    #include <type_traits>
namespace std {
    namespace experimental {
        template<typename...>
        using void_t = void;

        namespace internal {
            template<typename V, typename D>
            struct detect_impl {
                using value_t = V;
                using type    = D;
            };

            template<typename D, template<typename...> class Scan, typename... Args>
            auto detect_check(char) -> detect_impl<std::false_type, D>;

            template<typename D, template<typename...> class Scan, typename... Args>
            auto detect_check(int) -> decltype(void_t<Scan<Args...> >(), detect_impl<std::true_type, Scan<Args...> >{});

            template<typename D, typename Void, template<typename...> class Scan, typename... Args>
            struct detect : decltype(detect_check<D, Scan, Args...>(0)) {};
        }

        struct nonesuch {
            nonesuch()                 = delete;
            ~nonesuch()                = delete;
            nonesuch(nonesuch const &) = delete;
            void operator=(nonesuch const &) = delete;
        };

        template<template<typename...> class Scan, typename... Args>
        using is_detected = typename internal::detect<nonesuch, void, Scan, Args...>::value_t;

    }
}
#endif
