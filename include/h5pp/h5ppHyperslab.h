#pragma once
#include "h5ppDimensionType.h"
#include "h5ppFormat.h"
#include "h5ppHid.h"
#include "h5ppOptional.h"
#include "h5ppTypeCast.h"
#include "h5ppTypeSfinae.h"
#include <H5Spublic.h>
#include <type_traits>
#include <utility>
#include <vector>
namespace h5pp {
    class Hyperslab {
        public:
        // Hyperslab properties. Read here https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-SelectHyperslab

        OptDimsType                 offset      = std::nullopt;   /*!< The start position of a hyperslab */
        OptDimsType                 extent      = std::nullopt;   /*!< The extent (or "count") of a hyperslab */
        OptDimsType                 stride      = std::nullopt;   /*!< The stride of a hyperslab. Empty means contiguous */
        OptDimsType                 blocks      = std::nullopt;   /*!< The blocks size of each element in  the hyperslab. Empty means 1x1 */
        std::optional<H5S_sel_type> select_type = std::nullopt;   /*!< Use to select hyperslab (i.e. subset), none, all, hyperslabs */
        H5S_seloper_t               select_oper = H5S_SELECT_SET; /*!< Default clears previous selection.  */

        Hyperslab() = default;
        Hyperslab(const DimsType &offset, const DimsType &extent, OptDimsType stride = std::nullopt, OptDimsType blocks = std::nullopt)
            : offset(offset), extent(extent), stride(std::move(stride)), blocks(std::move(blocks)) {}

        // Delete some constructors to avoid ambiguity. Both offset and extent are required if any of them is to be given
        Hyperslab(DimsType)                       = delete;
        Hyperslab(OptDimsType)                    = delete;
        Hyperslab(std::initializer_list<hsize_t>) = delete;
        Hyperslab(std::vector<hsize_t>)           = delete;

        explicit Hyperslab(const hid::h5s &space) {
            int rank = H5Sget_simple_extent_ndims(space);
            if(rank < 0) throw h5pp::runtime_error("Could not read ndims on given space");
            if(rank == 0) return;
            select_type = H5Sget_select_type(space);
            if(select_type.value() == H5S_SEL_HYPERSLABS) {
#if H5_VERSION_GE(1, 10, 0)
                htri_t is_regular = H5Sis_regular_hyperslab(space);
                if(is_regular < 0) throw h5pp::runtime_error("Failed to query hyperslab type in space");
                if(not is_regular) {
                    throw h5pp::runtime_error("The space has irregular (non-rectangular) hyperslab selection.\n"
                                              "This is not yet supported by h5pp");
                }
#endif
                offset = std::vector<hsize_t>(type::safe_cast<size_t>(rank), 0);
                extent = std::vector<hsize_t>(type::safe_cast<size_t>(rank), 0);
                stride = std::vector<hsize_t>(type::safe_cast<size_t>(rank), 0);
                blocks = std::vector<hsize_t>(type::safe_cast<size_t>(rank), 0);
#if H5_VERSION_GE(1, 10, 0)
                H5Sget_regular_hyperslab(space, offset->data(), stride->data(), extent->data(), blocks->data());
#else
                H5Sget_simple_extent_dims(space, extent->data(), nullptr);
#endif
            } else if(select_type.value() == H5S_SEL_ALL) {
                offset = std::vector<hsize_t>(type::safe_cast<size_t>(rank), 0);
                extent = std::vector<hsize_t>(type::safe_cast<size_t>(rank), 0);
                H5Sget_simple_extent_dims(space, extent->data(), nullptr);
            } else if(select_type.value() == H5S_SEL_NONE) {
                return;
            } else if(select_type.value() == H5S_SEL_ERROR) {
                throw h5pp::runtime_error("Invalid hyperslab selection");
            } else {
                throw h5pp::runtime_error("Unsupported selection type. Choose space selection type NONE, ALL or HYPERSLABS");
            }
        }
        [[nodiscard]] bool empty() const { return not offset and not extent and not stride and not blocks; }

        [[nodiscard]] std::string string(bool enable = true) const {
            std::string msg;
            if(not enable) return msg;
            if(offset) msg.append(h5pp::format(" | offset {}", offset.value()));
            if(extent) msg.append(h5pp::format(" | extent {}", extent.value()));
            if(stride) msg.append(h5pp::format(" | stride {}", stride.value()));
            if(blocks) msg.append(h5pp::format(" | blocks {}", blocks.value()));
            return msg;
        }

        template<typename h5x>
        void applySelection(const h5x &space) const {
            static_assert(type::sfinae::is_hdf5_space_id<h5x>);
            H5S_seloper_t sel = select_oper;
            if(H5Sget_select_type(space) != H5S_SEL_HYPERSLABS and sel != H5S_SELECT_SET)
                sel = H5S_SELECT_SET; // First hyperslab selection must be H5S_SELECT_SET

            const hsize_t *offptr = offset ? offset->data() : nullptr;
            const hsize_t *extptr = extent ? extent->data() : nullptr;
            const hsize_t *strptr = stride ? stride->data() : nullptr;
            const hsize_t *blkptr = blocks ? blocks->data() : nullptr;
            herr_t         retval = H5Sselect_hyperslab(space, sel, offptr, strptr, extptr, blkptr);
            if(retval < 0) throw h5pp::runtime_error("Failed to select hyperslab");
        }
    };

}
