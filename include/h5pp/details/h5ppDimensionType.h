#pragma once
#include "h5ppError.h"
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

    struct DimsType {
        std::vector<hsize_t> dims;
        DimsType()                          = default;
        explicit DimsType(H5D_layout_t)     = delete;
        explicit DimsType(hid::h5t)         = delete;
        explicit DimsType(hid_t)            = delete;
        explicit DimsType(std::string)      = delete;
        explicit DimsType(std::string_view) = delete;
        explicit DimsType(const char *)     = delete;
        DimsType(h5pp::Options)             = delete;
        DimsType(h5pp::DsetInfo)            = delete;
        DimsType(h5pp::DataInfo)            = delete;
        DimsType(h5pp::TableInfo)           = delete;
        DimsType(h5pp::Hyperslab)           = delete;
        DimsType(const std::nullopt_t &) { throw h5pp::runtime_error("nullopt is not a valid dimension for this argument"); }
        DimsType(std::initializer_list<hsize_t> &&list) { dims = std::vector<hsize_t>(std::begin(list), std::end(list)); }
        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        DimsType(std::initializer_list<T> &&list) {
            dims = std::vector<hsize_t>(std::begin(list), std::end(list));
        }
        DimsType(std::optional<std::vector<hsize_t>> otherDims) {
            if(not otherDims) throw h5pp::runtime_error("Cannot initialize DimsType with nullopt");
            dims = otherDims.value();
        }
        template<typename UnknownType>
        DimsType(const UnknownType &dims_) {
            if constexpr(std::is_integral_v<UnknownType>) {
                dims = std::vector<hsize_t>{type::safe_cast<size_t>(dims_)};
            } else if constexpr(h5pp::type::sfinae::is_iterable_v<UnknownType>) {
                dims = std::vector<hsize_t>(std::begin(dims_), std::end(dims_));
            } else if constexpr(std::is_same_v<UnknownType, OptDimsType>) {
                if(not dims_) throw h5pp::runtime_error("Cannot initialize DimsType with nullopt");
                else dims = dims_.value();
            } else if constexpr(std::is_assignable_v<UnknownType, DimsType>) {
                dims = dims_;
            } else if constexpr(std::is_array_v<UnknownType> and std::is_integral_v<std::remove_all_extents_t<UnknownType>>) {
                dims = std::vector<hsize_t>(std::begin(dims_), std::end(dims_));
            } else {
                static_assert(h5pp::type::sfinae::invalid_type_v<UnknownType>, "Could not identify dimension type");
                throw h5pp::runtime_error("Could not identify dimension type: {}", h5pp::type::sfinae::type_name<UnknownType>());
            }
        }
        [[nodiscard]] operator const std::vector<hsize_t> &() const { return dims; }
        [[nodiscard]] operator std::vector<hsize_t> &() { return dims; }
    };

    struct OptDimsType {
        std::optional<std::vector<hsize_t>> dims = std::vector<hsize_t>();
        OptDimsType()                            = default;
        explicit OptDimsType(H5D_layout_t)       = delete;
        explicit OptDimsType(hid::h5t)           = delete;
        explicit OptDimsType(hid_t)              = delete;
        explicit OptDimsType(std::string)        = delete;
        explicit OptDimsType(std::string_view)   = delete;
        explicit OptDimsType(const char *)       = delete;
        OptDimsType(h5pp::Options)               = delete;
        OptDimsType(h5pp::DsetInfo)              = delete;
        OptDimsType(h5pp::DataInfo)              = delete;
        OptDimsType(h5pp::TableInfo)             = delete;
        OptDimsType(h5pp::Hyperslab)             = delete;

        OptDimsType(const std::nullopt_t &nullopt) { dims = nullopt; }
        OptDimsType(std::initializer_list<hsize_t> &&list) { dims = std::vector<hsize_t>(std::begin(list), std::end(list)); }
        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        OptDimsType(std::initializer_list<T> &&list) {
            dims = std::vector<hsize_t>(std::begin(list), std::end(list));
        }
        OptDimsType(std::optional<std::vector<hsize_t>> otherDims) : dims(std::move(otherDims)) {}
        template<typename UnknownType>
        OptDimsType(const UnknownType &dims_) {
            if constexpr(std::is_integral_v<UnknownType>) {
                dims = std::vector<hsize_t>{type::safe_cast<size_t>(dims_)};
            } else if constexpr(h5pp::type::sfinae::is_iterable_v<UnknownType>) {
                dims = std::vector<hsize_t>(std::begin(dims_), std::end(dims_));
            } else if constexpr(std::is_assignable_v<UnknownType, OptDimsType> or std::is_assignable_v<UnknownType, DimsType>) {
                dims = dims_;
            } else if constexpr(std::is_array_v<UnknownType> and std::is_integral_v<std::remove_all_extents_t<UnknownType>>) {
                dims = std::vector<hsize_t>(std::begin(dims_), std::end(dims_));
            } else {
                static_assert(h5pp::type::sfinae::invalid_type_v<UnknownType>, "Could not identify dimension type");
                throw h5pp::runtime_error("Could not identify dimension type: {}", h5pp::type::sfinae::type_name<UnknownType>());
            }
        }
        [[nodiscard]] bool                        has_value() const { return dims.has_value(); }
                                                  operator bool() const { return dims.has_value(); }
        [[nodiscard]] const std::vector<hsize_t> &value() const { return dims.value(); }
        [[nodiscard]] std::vector<hsize_t>       &value() { return dims.value(); }
        [[nodiscard]]                             operator const std::optional<std::vector<hsize_t>> &() const { return dims; }
        [[nodiscard]]                             operator std::optional<std::vector<hsize_t>> &() { return dims; }
        auto                                      operator->() { return dims.operator->(); }
        auto                                      operator->() const { return dims.operator->(); }
    };

}
