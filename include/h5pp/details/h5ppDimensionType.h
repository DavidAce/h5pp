#pragma once
#include "h5ppExcept.h"
#include "h5ppFormat.h"
#include "h5ppTypeCast.h"
#include "h5ppTypeSfinae.h"
#include <H5Dpublic.h>
#include <optional>
#include <utility>

namespace h5pp {
    struct Options;
    struct DsetInfo;
    struct DataInfo;
    struct TableInfo;
    struct OptDimsType;
    class Hyperslab;

    struct DimsType : std::vector<hsize_t> {
        using std::vector<hsize_t>::vector;
        explicit DimsType(H5D_layout_t) = delete;
        explicit DimsType(hid::h5t)     = delete;
        explicit DimsType(hid_t)        = delete;

        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        DimsType(std::initializer_list<T> &&list) : std::vector<hsize_t>(std::begin(list), std::end(list)) {}

        template<typename... ARGS, typename = std::enable_if_t<(... && std::is_integral_v<ARGS>)>>
        DimsType(const ARGS &...args) : std::vector<hsize_t>(std::initializer_list<hsize_t>{h5pp::type::safe_cast<hsize_t>(args)...}) {}

        template<typename T, typename = std::enable_if_t<h5pp::type::sfinae::is_iterable_v<T>>>
        DimsType(T &&dims_) : std::vector<hsize_t>(std::begin(dims_), std::end(dims_)) {}
    };

    struct OptDimsType : std::optional<std::vector<hsize_t>> {
        using std::optional<std::vector<hsize_t>>::optional;
        explicit OptDimsType(H5D_layout_t) = delete;
        explicit OptDimsType(hid::h5t)     = delete;
        explicit OptDimsType(hid_t)        = delete;

        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        OptDimsType(std::initializer_list<T> &&list)
            : std::optional<std::vector<hsize_t>>(std::vector<hsize_t>(std::begin(list), std::end(list))) {}

        template<typename... ARGS, typename = std::enable_if_t<(... && std::is_integral_v<ARGS>)>>
        OptDimsType(const ARGS &...args)
            : std::optional<std::vector<hsize_t>>(std::initializer_list<hsize_t>{h5pp::type::safe_cast<hsize_t>(args)...}) {}

        template<typename T, typename = std::enable_if_t<h5pp::type::sfinae::is_iterable_v<T>>>
        OptDimsType(T &&dims_) : std::optional<std::vector<hsize_t>>(std::vector<hsize_t>(std::begin(dims_), std::end(dims_))) {}

        template<typename T, typename = std::enable_if_t<h5pp::type::sfinae::is_iterable_v<T>>>
        OptDimsType(std::optional<T> &&dims_)
            : std::optional<std::vector<hsize_t>>(dims_.has_value() ? OptDimsType(dims_.value()) : std::nullopt) {}
    };

    //
    //
    // struct OptDimsType : std::optional<std::vector<hsize_t>> {
    //     using std::optional<std::vector<hsize_t>>::optional;
    //     explicit OptDimsType(H5D_layout_t) = delete;
    //     explicit OptDimsType(hid::h5t)     = delete;
    //     explicit OptDimsType(hid_t)        = delete;
    //
    //     template<typename T>
    //     OptDimsType(std::initializer_list<T> &&list)
    //         : std::optional<std::vector<hsize_t>>(std::vector<hsize_t>(std::begin(list), std::end(list))) {
    //         static_assert(std::is_integral_v<T>);
    //     }
    //     template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    //     OptDimsType(T v) : std::optional<std::vector<hsize_t>>(std::vector<hsize_t>(1ul, type::safe_cast<hsize_t>(v))) {}
    //
    //     template<typename T, typename = std::enable_if_t<h5pp::type::sfinae::is_iterable_v<T>>>
    //     OptDimsType(T &&dims_) : std::optional<std::vector<hsize_t>>(std::vector<hsize_t>(std::begin(dims_), std::end(dims_))) {}
    //
    //     template<typename T>
    //     OptDimsType(std::optional<T> &&dims_)
    //         : std::optional<std::vector<hsize_t>>(dims_.has_value() ? OptDimsType(dims_.value()) : std::nullopt) {
    //         static_assert(h5pp::type::sfinae::is_iterable_v<T>);
    //     }
    // };
}
