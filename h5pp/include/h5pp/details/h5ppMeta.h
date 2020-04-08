#pragma once
#include "h5ppEnums.h"
#include "h5ppHid.h"
#include "h5ppLogger.h"
#include "h5ppOptional.h"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <string>
#include <vector>

namespace h5pp {

    struct DsetOptions {
        using DimsType                          = std::vector<hsize_t>;
        std::optional<DimsType>     dataDims    = std::nullopt;
        std::optional<DimsType>     chunkDims   = std::nullopt;
        std::optional<hid::h5t>     h5_type     = std::nullopt;
        std::optional<H5D_layout_t> h5_layout   = std::nullopt;
        std::optional<unsigned int> compression = std::nullopt;

        DsetOptions() = default;
        template<typename DType = std::initializer_list<hsize_t>>
        DsetOptions(DType                       dataDims_,
                    DType                       chunkDims_,
                    std::optional<hid::h5t>     h5_type_   = std::nullopt,
                    std::optional<H5D_layout_t> h5_layout_ = std::nullopt,
                    //                    std::optional<HyperSlab>    memSlab_     = std::nullopt,
                    //                    std::optional<HyperSlab>    fileSlab_    = std::nullopt,
                    std::optional<unsigned int> compression_ = std::nullopt)
            : h5_type(std::move(h5_type_)), h5_layout(h5_layout_), compression(compression_) {
            if(dataDims_.size() > 0) dataDims = dataDims_;
            if(chunkDims_.size() > 0) chunkDims = chunkDims_;
        }
    };

    /*!
     * \struct MetaData
     * Struct with optional fields describing a C++ data type
     */
    struct MetaData {
        std::optional<size_t>               dataByte = std::nullopt;
        std::optional<hsize_t>              dataSize = std::nullopt;
        std::optional<int>                  dataRank = std::nullopt;
        std::optional<std::vector<hsize_t>> dataDims = std::nullopt;
        std::optional<hid::h5s>             h5_space = std::nullopt;

        //        std::optional<hid::h5t>             h5_type     = std::nullopt;
        //        std::optional<hid::h5s>             h5_memSpace = std::nullopt;

        void assertWriteReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dataByte) error_msg.append(" | dataBytes");
            if(not dataSize) error_msg.append(" | dataSize");
            if(not dataRank) error_msg.append(" | dataRank");
            if(not dataDims) error_msg.append(" | dataDims");
            if(not h5_space) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write to dataset. The following meta fields are undefined\n\t" + error_msg);
            if(not h5_space->valid() ) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write to dataset. The following meta fields are not valid\n\t" + error_msg);

            /* clang-format on */
            hsize_t size_check = std::accumulate(dataDims->begin(), dataDims->end(), 1, std::multiplies<>());
//            if(size_check == (hsize_t) 1 and dataSize.value() == dataByte.value()) return; // This is typical for strings. Just return.
            if(size_check != dataSize.value()) throw std::runtime_error(h5pp::format("Data size mismatch: dataSize [{}] | size check [{}]", dataSize.value(), size_check));
        }

        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dataByte) error_msg.append(" | dataBytes");
            if(not dataSize) error_msg.append(" | dataSize");
            if(not dataRank) error_msg.append(" | dataRank");
            if(not dataDims) error_msg.append(" | dataDims");
            if(not h5_space) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write to dataset. The following meta fields are undefined\n\t" + error_msg);
            if(not h5_space->valid() ) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write to dataset. The following meta fields are not valid\n\t" + error_msg);

            /* clang-format on */
            hsize_t size_check = std::accumulate(dataDims->begin(), dataDims->end(), 1, std::multiplies<>());
//            if(size_check == (hsize_t) 1 and dataSize.value() == dataByte.value()) return; // This is typical for strings. Just return.
            if(size_check != dataSize.value()) throw std::runtime_error(h5pp::format("Data size mismatch: dataSize [{}] | size check [{}]", dataSize.value(), size_check));
        }

    };

    /*!
     * \struct MetaDset
     * Struct with optional fields describing data on file, i.e. a dataset
     */
    struct MetaDset {
        std::optional<hid::h5f>             h5_file              = std::nullopt;
        std::optional<hid::h5d>             h5_dset              = std::nullopt;
        std::optional<hid::h5t>             h5_type              = std::nullopt;
        std::optional<H5D_layout_t>         h5_layout            = std::nullopt;
        std::optional<hid::h5s>             h5_space             = std::nullopt;
        std::optional<hid::h5p>             h5_plist_dset_create = std::nullopt;
        std::optional<hid::h5p>             h5_plist_dset_access = std::nullopt;
        std::optional<std::string>          dsetPath             = std::nullopt;
        std::optional<bool>                 dsetExists           = std::nullopt;
        std::optional<hsize_t>              dsetSize             = std::nullopt;
        std::optional<size_t>               dsetByte             = std::nullopt;
        std::optional<int>                  dsetRank             = std::nullopt;
        std::optional<std::vector<hsize_t>> dsetDims             = std::nullopt;
        std::optional<std::vector<hsize_t>> dsetDimsMax          = std::nullopt;
        std::optional<std::vector<hsize_t>> chunkDims            = std::nullopt;
        std::optional<unsigned int>         compression          = std::nullopt;

        void assertCreateReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_file             ) error_msg.append("\n\t h5_file");
            if(not h5_type             ) error_msg.append("\n\t h5_type");
            if(not h5_space            ) error_msg.append("\n\t h5_space");
            if(not h5_plist_dset_create) error_msg.append("\n\t h5_plist_dset_create");
            if(not h5_plist_dset_access) error_msg.append("\n\t h5_plist_dset_access");
            if(not dsetPath            ) error_msg.append("\n\t dsetPath");
            if(not dsetExists          ) error_msg.append("\n\t dsetExists");
//            if(not dsetSize            ) error_msg.append("\n\t dsetSize");
//            if(not dsetBytes           ) error_msg.append("\n\t dsetBytes");
//            if(not dsetRank            ) error_msg.append("\n\t dsetRank");
//            if(not dsetDims            ) error_msg.append("\n\t dsetDims");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot create dataset. The following meta fields are undefined\n\t" + error_msg);
            if(not h5_file->valid()             ) error_msg.append("\n\t h5_file");
            if(not h5_type->valid()             ) error_msg.append("\n\t h5_type");
            if(not h5_space->valid()            ) error_msg.append("\n\t h5_space");
            if(not h5_plist_dset_create->valid()) error_msg.append("\n\t h5_plist_dset_create");
            if(not h5_plist_dset_access->valid()) error_msg.append("\n\t h5_plist_dset_access");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot create dataset. The following meta fields are not valid\n\t" + error_msg);
            /* clang-format on */
        }
        void assertWriteReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_dset             ) error_msg.append("\n\t h5_dset");
            if(not h5_type             ) error_msg.append("\n\t h5_type");
            if(not h5_space            ) error_msg.append("\n\t h5_space");
            if(not dsetPath            ) error_msg.append("\n\t dsetPath");
            if(not dsetExists          ) error_msg.append("\n\t dsetExists");
            if(not dsetSize            ) error_msg.append("\n\t dsetSize");
            if(not dsetByte            ) error_msg.append("\n\t dsetBytes");
            if(not dsetRank            ) error_msg.append("\n\t dsetRank");
            if(not dsetDims            ) error_msg.append("\n\t dsetDims");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write to dataset. The following meta fields are undefined\n\t" + error_msg);
            if(not h5_file->valid() ) error_msg.append("\n\t h5_file");
            if(not h5_type->valid() ) error_msg.append("\n\t h5_type");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write to dataset. The following meta fields are not valid\n\t" + error_msg);
            /* clang-format on */
        }
        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_dset             ) error_msg.append("\n\t h5_dset");
            if(not h5_type             ) error_msg.append("\n\t h5_type");
            if(not h5_space            ) error_msg.append("\n\t h5_space");
            if(not dsetPath            ) error_msg.append("\n\t dsetPath");
            if(not dsetExists          ) error_msg.append("\n\t dsetExists");
            if(not dsetSize            ) error_msg.append("\n\t dsetSize");
            if(not dsetByte            ) error_msg.append("\n\t dsetBytes");
            if(not dsetRank            ) error_msg.append("\n\t dsetRank");
            if(not dsetDims            ) error_msg.append("\n\t dsetDims");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write to dataset. The following meta fields are undefined\n\t" + error_msg);
            if(not h5_file->valid() ) error_msg.append("\n\t h5_file");
            if(not h5_type->valid() ) error_msg.append("\n\t h5_type");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write to dataset. The following meta fields are not valid\n\t" + error_msg);
            /* clang-format on */
        }
    };

//    struct MetaSpace {
//        std::optional<hid::h5s> h5_memSpace  = std::nullopt;
//        std::optional<hid::h5s> h5_fileSpace = std::nullopt;
//
//        void assertSpaceReady() const {
//            /* clang-format off */
//            std::string error_msg;
//            if(not h5_memSpace ) error_msg.append("\n\t h5_memSpace");
//            if(not h5_fileSpace) error_msg.append("\n\t h5_fileSpace");
//            if(not error_msg.empty())
//                throw std::runtime_error("The space metadata is not ready: The following fields are undefined\n\t" + error_msg);
//            if(not h5_memSpace->valid() ) error_msg.append("\n\t h5_memSpace");
//            if(not h5_fileSpace->valid()) error_msg.append("\n\t h5_fileSpace");
//            if(not error_msg.empty())
//                throw std::runtime_error("The space metadata is not ready: The following fields are not valid\n\t" + error_msg);
//            if(H5Sselect_valid(h5_memSpace.value()) < 0) {
//                H5Eprint(H5E_DEFAULT, stderr);
//                throw std::runtime_error("h5_memSpace is not valid");
//            }
//            if(H5Sselect_valid(h5_fileSpace.value()) < 0) {
//                H5Eprint(H5E_DEFAULT, stderr);
//                throw std::runtime_error("h5_fileSpace is not valid");
//            }
//            /* clang-format on */
//            htri_t equal = H5Sextent_equal(h5_memSpace.value(), h5_fileSpace.value());
//            if(equal == 0) {
//                // One of the maxDims may beH5S_UNLIMITED, in which case, we just check the
//                // dimensions
//                if(getDims(h5_memSpace.value()) == getDims(h5_fileSpace.value())) return;
//                auto msg_mem  = getSpaceString(h5_memSpace.value());
//                auto msg_file = getSpaceString(h5_fileSpace.value());
//                throw std::runtime_error(h5pp::format("Spaces are not equal \n\t mem  space \t {} \n\t file space \t {}", msg_mem, msg_file));
//            } else if(equal < 0) {
//                throw std::runtime_error("Failed to compare space estents");
//            }
//        }
//
//        private:
//        std::vector<hsize_t> getDims(const hid::h5s &space) const {
//            int                  rank = H5Sget_simple_extent_ndims(space);
//            std::vector<hsize_t> dims(rank);
//            H5Sget_simple_extent_dims(space, dims.data(), nullptr);
//            return dims;
//        }
//
//        std::string getSpaceString(const hid::h5s &space) const {
//            std::string msg;
//            int         rank = H5Sget_simple_extent_ndims(space);
//            msg.append(h5pp::format(" | size {}", H5Sget_simple_extent_npoints(space)));
//            msg.append(h5pp::format(" | rank {}", rank));
//            std::vector<hsize_t>     dims(rank);
//            std::vector<hsize_t>     maxDims(rank);
//            std::vector<std::string> maxDimsStr;
//            H5Sget_simple_extent_dims(space, dims.data(), maxDims.data());
//            for(auto &dim : maxDims)
//                if(dim == H5S_UNLIMITED)
//                    maxDimsStr.emplace_back("H5S_UNLIMITED");
//                else
//                    maxDimsStr.emplace_back(std::to_string(dim));
//            msg.append(h5pp::format(" | dims {}", dims));
//            msg.append(h5pp::format(" | max dims {}", maxDimsStr));
//            if(H5Sget_select_type(space) == H5S_SEL_HYPERSLABS) {
//                HyperSlab slab(space);
//                msg.append(" | \t -- hyperslab -- ");
//                if(slab.offset) msg.append(h5pp::format(" | offset {} ", slab.offset.value()));
//                if(slab.extent) msg.append(h5pp::format(" | extent {} ", slab.extent.value()));
//                if(slab.stride) msg.append(h5pp::format(" | stride {} ", slab.stride.value()));
//                if(slab.block) msg.append(h5pp::format(" | block {} ", slab.block.value()));
//            }
//            return msg;
//        }
//    };

}
