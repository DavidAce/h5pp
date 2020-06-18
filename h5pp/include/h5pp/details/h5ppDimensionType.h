#pragma once
#include "h5ppFormat.h"
#include "h5ppTypeSfinae.h"
#include <optional>

namespace h5pp {
    struct Options;
    struct DsetInfo;
    struct DataInfo;
    struct TableInfo;
    struct DimsType {
        std::vector<hsize_t> dims;
        DimsType()=default;
        DimsType(H5D_layout_t) = delete;
        DimsType(hid::h5t) = delete;
        DimsType(hid_t) = delete;
        DimsType(std::string) = delete;
        DimsType(std::string_view) = delete;
        DimsType(const char *) = delete;
        DimsType(h5pp::Options) = delete;
        DimsType(h5pp::DsetInfo) = delete;
        DimsType(h5pp::DataInfo) = delete;
        DimsType(h5pp::TableInfo) = delete;
        template<typename h5x, std::enable_if_t<std::is_base_of_v<hid::hid_base,h5x>>>
        DimsType(h5x) = delete;
        DimsType(const std::nullopt_t &) { throw std::runtime_error("nullopt is not a valid dimension for this argument");}
        DimsType(std::initializer_list<hsize_t> &&list) {
            std::copy(list.begin(), list.end(), std::back_inserter(dims));
        }
        template<typename UnknownType>
        DimsType (const UnknownType &dims_) {
            if constexpr (std::is_integral_v<UnknownType>){
                dims = std::vector<hsize_t>{static_cast<size_t>(dims_)};
            }else if constexpr (h5pp::type::sfinae::is_iterable_v<UnknownType>){
                std::copy(dims_.begin(), dims_.end(), std::back_inserter(dims));
            }
            else
                throw std::runtime_error(h5pp::format("Could not identify dimension type: {}", h5pp::type::sfinae::type_name<UnknownType>()));
        }
    };

    struct OptDimsType {
        std::optional<std::vector<hsize_t>> dims = std::vector<hsize_t>();
        OptDimsType()=default;
        OptDimsType(H5D_layout_t) = delete;
        OptDimsType(hid::h5t) = delete;
        OptDimsType(hid_t) = delete;
        OptDimsType(std::string) = delete;
        OptDimsType(std::string_view) = delete;
        OptDimsType(const char *) = delete;
        OptDimsType(h5pp::Options) = delete;
        OptDimsType(h5pp::DsetInfo) = delete;
        OptDimsType(h5pp::DataInfo) = delete;
        OptDimsType(h5pp::TableInfo) = delete;

        OptDimsType(const std::nullopt_t &nullopt) { dims = nullopt;}
        OptDimsType(std::initializer_list<hsize_t> &&list) {
            std::copy(list.begin(), list.end(), std::back_inserter(dims.value()));
        }
        template<typename UnknownType>
        OptDimsType (const UnknownType &dims_) {
            if constexpr (std::is_integral_v<UnknownType>){
                dims = std::vector<hsize_t>{static_cast<size_t>(dims_)};
            }else if constexpr (h5pp::type::sfinae::is_iterable_v<UnknownType>){
                std::copy(dims_.begin(), dims_.end(), std::back_inserter(dims.value()));
            }
            else
                throw std::runtime_error(h5pp::format("Could not identify dimension type: {}", h5pp::type::sfinae::type_name<UnknownType>()));
        }
    };

}





