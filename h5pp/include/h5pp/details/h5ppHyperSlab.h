#pragma once
#include "h5ppTypeSfinae.h"
#include "h5ppOptional.h"
#include <hdf5.h>
#include <type_traits>

#include <vector>
namespace h5pp {
    class HyperSlab {
        public:
        // Hyperslab properties. Read here https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-SelectHyperslab
        std::optional<std::vector<hsize_t>> offset = std::nullopt; /*!< The start position of a hyperslab */
        std::optional<std::vector<hsize_t>> extent = std::nullopt; /*!< The extent (or "count") of a hyperslab */
        std::optional<std::vector<hsize_t>> stride = std::nullopt; /*!< The stride of a hyperslab. Empty means contiguous */
        std::optional<std::vector<hsize_t>> block  = std::nullopt; /*!< The block size of each element in  the hyperslab. Empty means 1x1 */

        HyperSlab() = default;
        template<typename DimType = std::initializer_list<hsize_t>>
        HyperSlab(DimType slabOffset_, DimType slabExtent_, DimType slabStride_ = {}, DimType slabBlock_ = {}) {
            offset = getOptionalIterable(slabOffset_);
            extent = getOptionalIterable(slabExtent_);
            stride = getOptionalIterable(slabStride_);
            block  = getOptionalIterable(slabBlock_);
        }

        explicit HyperSlab(const hid::h5s &space) {
            if(H5Sget_select_type(space) != H5S_SEL_HYPERSLABS) return;
            int rank = H5Sget_simple_extent_ndims(space);
            if(rank < 0) throw std::runtime_error("Could not read ndims on given space");
            if(rank == 0) return;
            offset = std::vector<hsize_t>(rank, 0);
            extent = std::vector<hsize_t>(rank, 0);
            stride = std::vector<hsize_t>(rank, 0);
            block  = std::vector<hsize_t>(rank, 0);
            H5Sget_regular_hyperslab(space, offset->data(), stride->data(), extent->data(), block->data());
        }
        [[nodiscard]] bool empty() const { return not offset and not extent and not stride and not block; }

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
                optiter = {(hsize_t) iterable};
            }
            return optiter;
        }
    };

}
