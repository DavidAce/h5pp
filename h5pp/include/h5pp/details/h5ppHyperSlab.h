#pragma once
#include "h5ppFormat.h"
#include "h5ppOptional.h"
#include "h5ppTypeSfinae.h"
#include <hdf5.h>
#include <type_traits>
#include <vector>
namespace h5pp {
    class HyperSlab {
        public:
        // Hyperslab properties. Read here https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-SelectHyperslab

        std::optional<std::vector<hsize_t>> offset      = std::nullopt; /*!< The start position of a hyperslab */
        std::optional<std::vector<hsize_t>> extent      = std::nullopt; /*!< The extent (or "count") of a hyperslab */
        std::optional<std::vector<hsize_t>> stride      = std::nullopt; /*!< The stride of a hyperslab. Empty means contiguous */
        std::optional<std::vector<hsize_t>> blocks      = std::nullopt; /*!< The blocks size of each element in  the hyperslab. Empty means 1x1 */
        std::optional<H5S_sel_type>         select_type = std::nullopt;
        HyperSlab()                                     = default;
        template<typename DimType = std::initializer_list<hsize_t>>
        HyperSlab(DimType slabOffset_, DimType slabExtent_, DimType slabStride_ = {}, DimType slabBlock_ = {}) {
            offset = getOptionalIterable(slabOffset_);
            extent = getOptionalIterable(slabExtent_);
            stride = getOptionalIterable(slabStride_);
            blocks = getOptionalIterable(slabBlock_);
        }

        explicit HyperSlab(const hid::h5s &space) {
            int rank = H5Sget_simple_extent_ndims(space);
            if(rank < 0) throw std::runtime_error("Could not read ndims on given space");
            if(rank == 0) return;
            select_type = H5Sget_select_type(space);
            if(select_type.value() == H5S_SEL_HYPERSLABS) {
                htri_t is_regular = H5Sis_regular_hyperslab(space);
                if(is_regular < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to query hyperslab type in space");
                }
                if(not is_regular)
                    throw std::runtime_error("The space has irregular (non-rectangular) hyperslab selection.\n"
                                             "This is not yet supported by h5pp");
                offset = std::vector<hsize_t>(static_cast<size_t>(rank), 0);
                extent = std::vector<hsize_t>(static_cast<size_t>(rank), 0);
                stride = std::vector<hsize_t>(static_cast<size_t>(rank), 0);
                blocks = std::vector<hsize_t>(static_cast<size_t>(rank), 0);
#if H5_VERSION_GE(1, 10, 0)
                H5Sget_regular_hyperslab(space, offset->data(), stride->data(), extent->data(), blocks->data());
#else
                H5Sget_simple_extent_dims(space, extent->data(), nullptr);
#endif
            } else if(select_type.value() == H5S_SEL_ALL) {
                offset = std::vector<hsize_t>(static_cast<size_t>(rank), 0);
                extent = std::vector<hsize_t>(static_cast<size_t>(rank), 0);
                H5Sget_simple_extent_dims(space, extent->data(), nullptr);
            } else if(select_type.value() == H5S_SEL_ERROR)
                throw std::runtime_error("Invalid hyperslab selection");
            else
                throw std::runtime_error("Unsupported selection type. Choose space selection type NONE, ALL or HYPERSLABS");
        }
        [[nodiscard]] bool empty() const { return not offset and not extent and not stride and not blocks; }

        [[nodiscard]] std::string string() const {
            std::string msg;
            if(offset) msg.append(h5pp::format(" | offset {}", offset.value()));
            if(extent) msg.append(h5pp::format(" | extent {}", extent.value()));
            if(stride) msg.append(h5pp::format(" | stride {}", stride.value()));
            if(blocks) msg.append(h5pp::format(" | blocks {}", blocks.value()));
            return msg;
        }

        private:
        template<typename T, typename = std::void_t<>>
        struct is_iterable : public std::false_type {};
        template<typename T>
        struct is_iterable<T, std::void_t<decltype(std::declval<T>().begin()), decltype(std::declval<T>().end()), typename T::value_type>> : public std::true_type {};
        template<typename T>
        static constexpr bool is_iterable_v = is_iterable<T>::value;

        template<typename IterableType = std::initializer_list<hsize_t>, typename = std::enable_if_t<is_iterable_v<IterableType> or std::is_integral_v<IterableType>>>
        [[nodiscard]] std::optional<std::vector<hsize_t>> getOptionalIterable(const IterableType &iterable) {
            //            if constexpr(std::is_same_v<IterableType, std::optional<std::vector<hsize_t>>>) return iterable;
            std::optional<std::vector<hsize_t>> optiter = std::nullopt;
            if constexpr(h5pp::type::sfinae::is_iterable_v<IterableType>) {
                if(iterable.size() > 0) {
                    optiter = std::vector<hsize_t>();
                    std::copy(iterable.begin(), iterable.end(), std::back_inserter(optiter.value()));
                }
            } else if constexpr(std::is_integral_v<IterableType>) {
                optiter = {static_cast<hsize_t>(iterable)};
            }
            return optiter;
        }
    };

}
