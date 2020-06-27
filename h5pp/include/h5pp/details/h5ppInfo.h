#pragma once
#include "h5ppEnums.h"
#include "h5ppHid.h"
#include "h5ppLogger.h"
#include "h5ppOptional.h"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <string>
#include <typeindex>
#include <variant>
#include <vector>

namespace h5pp {
    namespace debug {
        enum class DimSizeComparison { ENFORCE, PERMISSIVE };
        inline auto reportCompatibility(std::optional<std::vector<hsize_t>> smallDims,
                                 std::optional<std::vector<hsize_t>> largeDims,
                                 DimSizeComparison                   dimComp = DimSizeComparison::ENFORCE) {
            std::string msg;
            if(not smallDims) return msg;
            if(not largeDims) return msg;
            if(smallDims->size() != largeDims->size()) msg.append("rank mismatch | ");
            bool ok = false;
            switch(dimComp) {
                case DimSizeComparison::ENFORCE:
                    ok = std::equal(std::begin(smallDims.value()),
                                    std::end(smallDims.value()),
                                    std::begin(largeDims.value()),
                                    std::end(largeDims.value()),
                                    [](const hsize_t &s, const hsize_t &l) -> bool { return s <= l; });
                    break;
                case DimSizeComparison::PERMISSIVE: ok = true; break;
                default: break;
            }

            if(not ok) msg.append("dimensions incompatible | ");
            return msg;
        }

        inline auto reportCompatibility(std::optional<H5D_layout_t>         h5_layout,
                                 std::optional<std::vector<hsize_t>> dims,
                                 std::optional<std::vector<hsize_t>> dimsChunk,
                                 std::optional<std::vector<hsize_t>> dimsMax) {
            std::string error_msg;
            if(h5_layout) {
                if(h5_layout.value() == H5D_CHUNKED) {}
                if(h5_layout.value() == H5D_COMPACT) {
                    if(dimsChunk)
                        error_msg.append(h5pp::format("Chunk dims {} | Layout is H5D_COMPACT | chunk dimensions are only meant for H5D_CHUNKED layouts\n", dimsChunk.value()));
                    if(dimsMax and dims and dimsMax.value() != dims.value())
                        error_msg.append(h5pp::format(
                            "dims {} | max dims {} | layout is H5D_COMPACT | dims and max dims must be equal unless the layout is H5D_CHUNKED\n", dims.value(), dimsMax.value()));
                }
                if(h5_layout.value() == H5D_CONTIGUOUS) {
                    if(dimsChunk)
                        error_msg.append(
                            h5pp::format("Chunk dims {} | Layout is H5D_CONTIGUOUS | chunk dimensions are only meant for datasets with H5D_CHUNKED layout \n", dimsChunk.value()));
                    if(dimsMax)
                        error_msg.append(
                            h5pp::format("Max dims {} | Layout is H5D_CONTIGUOUS | max dimensions are only meant for datasets with H5D_CHUNKED layout \n", dimsMax.value()));
                }
            }
            std::string res1 = reportCompatibility(dims, dimsMax);
            std::string res2 = reportCompatibility(dims, dimsChunk, DimSizeComparison::PERMISSIVE);
            std::string res3 = reportCompatibility(dimsChunk, dimsMax);
            if(not res1.empty()) error_msg.append(h5pp::format("\t{}: dims {} | max dims {}\n", res1, dims.value(), dimsMax.value()));
            if(not res2.empty()) error_msg.append(h5pp::format("\t{}: dims {} | chunk dims {}\n", res2, dims.value(), dimsChunk.value()));
            if(not res3.empty()) error_msg.append(h5pp::format("\t{}: chunk dims {} | max dims {}\n", res3, dimsChunk.value(), dimsMax.value()));
            return error_msg;
        }

    }

    struct Options {
        std::optional<std::string>      linkPath      = std::nullopt; /*!< Path to HDF5 dataset relative to the file root */
        std::optional<std::string>      attrName      = std::nullopt; /*!< Name of attribute on group or dataset */
        OptDimsType                     dataDims      = std::nullopt; /*!< Data dimensions hint. Required for pointer data */
        OptDimsType                     dsetDimsChunk = std::nullopt; /*!< (On create) Chunking dimensions. Only valid for H5D_CHUNKED datasets */
        OptDimsType                     dsetDimsMax   = std::nullopt; /*!< (On create) Maximum dimensions. Only valid for H5D_CHUNKED datasets */
        std::optional<Hyperslab>        dsetSlab      = std::nullopt; /*!< Select hyperslab, a subset of the data to participate in transfers to/from the dataset  */
        std::optional<Hyperslab>        attrSlab      = std::nullopt; /*!< Select hyperslab, a subset of the data to participate in transfers to/from the attribute  */
        std::optional<Hyperslab>        dataSlab      = std::nullopt; /*!< Select hyperslab, a subset of the data to participate in transfers to/from memory  */
        std::optional<hid::h5t>         h5_type       = std::nullopt; /*!< (On create) Type of dataset. Override automatic type detection. */
        std::optional<H5D_layout_t>     h5_layout     = std::nullopt; /*!< (On create) Layout of dataset. Choose between H5D_CHUNKED,H5D_COMPACT and H5D_CONTIGUOUS */
        std::optional<unsigned int>     compression   = std::nullopt; /*!< (On create) Compression level 0-9, 0 = off, 9 is gives best compression and is slowest */
        std::optional<h5pp::ResizeMode> resizeMode    = std::nullopt; /*!< Type of resizing if needed. Choose INCREASE_ONLY, RESIZE_TO_FIT,DO_NOT_RESIZE */
        [[nodiscard]] std::string       string() const {
            std::string msg;
            /* clang-format off */
            if(dataDims) msg.append(h5pp::format(" | data dims {}", dataDims.value()));
            if(dsetDimsMax) msg.append(h5pp::format(" | max dims {}", dsetDimsMax.value()));
            if(h5_layout){
                switch(h5_layout.value()){
                    case H5D_CHUNKED: msg.append(h5pp::format(" | H5D_CHUNKED")); break;
                    case H5D_CONTIGUOUS: msg.append(h5pp::format(" | H5D_CONTIGUOUS")); break;
                    case H5D_COMPACT: msg.append(h5pp::format(" | H5D_COMPACT")); break;
                    default: break;
                }
            }
            if(dsetDimsChunk) msg.append(h5pp::format(" | chunk dims {}", dsetDimsChunk.value()));
            if (dataSlab) msg.append(h5pp::format(" | memory hyperslab {}", dataSlab->string()));
            if (dsetSlab) msg.append(h5pp::format(" | file hyperslab {}", dsetSlab->string()));
            return msg;
            /* clang-format on */
        }

        void assertWellDefined() const {
            std::string error_msg;
            if(not linkPath) error_msg.append("\tMissing field: linkPath\n");
            error_msg.append(debug::reportCompatibility(h5_layout, dataDims, dsetDimsChunk, dsetDimsMax));
            if(not error_msg.empty()) throw std::runtime_error(h5pp::format("Options are not well defined: \n{}", error_msg));
        }
    };

    /*!
     * \struct DataInfo
     * Struct with optional fields describing a C++ data type in memory
     */
    struct DataInfo {
        std::optional<hid::h5s>    h5_space = std::nullopt;
        std::optional<size_t>      dataByte = std::nullopt;
        std::optional<hsize_t>     dataSize = std::nullopt;
        std::optional<int>         dataRank = std::nullopt;
        OptDimsType                dataDims = std::nullopt;
        std::optional<std::string> cpp_type = std::nullopt;
        std::optional<Hyperslab>   dataSlab = std::nullopt;
        DataInfo()                          = default;
        explicit DataInfo(const hid::h5s &space) {
            h5_space = space;
            setFromSpace();
        }
        void setFromSpace() {
            if(not h5_space) return;
            dataRank = H5Sget_simple_extent_ndims(h5_space.value());
            dataDims = std::vector<hsize_t>(static_cast<size_t>(dataRank.value()), 0);
            H5Sget_simple_extent_dims(h5_space.value(), dataDims->data(), nullptr);
        }

        void assertWriteReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dataSize) error_msg.append(" | dataSize");
            if(not dataByte) error_msg.append(" | dataByte");
            if(not dataRank) error_msg.append(" | dataRank");
            if(not dataDims) error_msg.append(" | dataDims");
            if(not h5_space) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot write from memory. The following fields are undefined:\n{}", error_msg));
            if(not h5_space->valid() ) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot write from memory. The following fields are not valid:\n{}", error_msg));

            /* clang-format on */
            hsize_t size_check = std::accumulate(dataDims->begin(), dataDims->end(), static_cast<hsize_t>(1), std::multiplies<>());
            if(size_check != dataSize.value())
                throw std::runtime_error(h5pp::format("Data size mismatch: dataSize [{}] | dataDims {} = size [{}]", dataSize.value(), dataDims.value(), size_check));
        }

        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dataSize) error_msg.append(" | dataSize");
            if(not dataByte) error_msg.append(" | dataByte");
            if(not dataRank) error_msg.append(" | dataRank");
            if(not dataDims) error_msg.append(" | dataDims");
            if(not h5_space) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot read into memory. The following fields are undefined:\n{}", error_msg));
            if(not h5_space->valid() ) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot read into memory. The following fields are not valid:\n{}", error_msg));

            /* clang-format on */
            hsize_t size_check = std::accumulate(dataDims->begin(), dataDims->end(), static_cast<hsize_t>(1), std::multiplies<>());
            if(size_check != dataSize.value()) throw std::runtime_error(h5pp::format("Data size mismatch: dataSize [{}] | size check [{}]", dataSize.value(), size_check));
        }
        [[nodiscard]] std::string string() const {
            //            std::string msg;
            std::string msg;
            /* clang-format off */
            if(dataSize) msg.append(h5pp::format(" | size {}", dataSize.value()));
            if(dataByte) msg.append(h5pp::format(" | bytes {}", dataByte.value()));
            if(dataRank) msg.append(h5pp::format(" | rank {}", dataRank.value()));
            if(dataDims) msg.append(h5pp::format(" | dims {}", dataDims.value()));
            if (h5_space and H5Sget_select_type(h5_space.value()) == H5S_sel_type::H5S_SEL_HYPERSLABS){
                Hyperslab slab(h5_space.value());
                msg.append(h5pp::format(" | [ Hyperslab {} ]", slab.string()));
            }
            if(cpp_type) msg.append(h5pp::format(" | type [{}]", cpp_type.value()));
            return msg;
            /* clang-format on */
        }
    };

    /*!
     * \struct DsetInfo
     * Struct with optional fields describing data on file, i.e. a dataset
     */
    struct DsetInfo {
        std::optional<hid::h5f>         h5_file              = std::nullopt;
        std::optional<hid::h5f>         h5_group             = std::nullopt;
        std::optional<hid::h5f>         h5_objLoc            = std::nullopt;
        std::optional<hid::h5d>         h5_dset              = std::nullopt;
        std::optional<hid::h5t>         h5_type              = std::nullopt;
        std::optional<H5D_layout_t>     h5_layout            = std::nullopt;
        std::optional<hid::h5s>         h5_space             = std::nullopt;
        std::optional<hid::h5p>         h5_plist_dset_create = std::nullopt;
        std::optional<hid::h5p>         h5_plist_dset_access = std::nullopt;
        std::optional<std::string>      dsetPath             = std::nullopt;
        std::optional<bool>             dsetExists           = std::nullopt;
        std::optional<hsize_t>          dsetSize             = std::nullopt;
        std::optional<size_t>           dsetByte             = std::nullopt;
        std::optional<int>              dsetRank             = std::nullopt;
        OptDimsType                     dsetDims             = std::nullopt;
        OptDimsType                     dsetDimsMax          = std::nullopt;
        OptDimsType                     dsetChunk            = std::nullopt;
        std::optional<h5pp::ResizeMode> resizeMode           = std::nullopt;
        std::optional<unsigned int>     compression          = std::nullopt;
        std::optional<Hyperslab>        dsetSlab             = std::nullopt;
        hid_t                           getLocId() const {
            if(h5_file) return h5_file.value();
            if(h5_group) return h5_group.value();
            if(h5_objLoc) return h5_objLoc.value();
            h5pp::logger::log->debug("Dataset location id is not defined");
            return -1;
        }
        void assertCreateReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dsetPath            ) error_msg.append("\t dsetPath\n");
            if(not dsetExists          ) error_msg.append("\t dsetExists\n");
            if(not h5_type             ) error_msg.append("\t h5_type\n");
            if(not h5_space            ) error_msg.append("\t h5_space\n");
            if(not h5_plist_dset_create) error_msg.append("\t h5_plist_dset_create\n");
            if(not h5_plist_dset_access) error_msg.append("\t h5_plist_dset_access\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create dataset. The following fields are undefined:\n{}",error_msg));
            if(not h5_type->valid()             ) error_msg.append("\t h5_type\n");
            if(not h5_space->valid()            ) error_msg.append("\t h5_space\n");
            if(not h5_plist_dset_create->valid()) error_msg.append("\t h5_plist_dset_create\n");
            if(not h5_plist_dset_access->valid()) error_msg.append("\t h5_plist_dset_access\n");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot create dataset. The following fields are not valid\n\t" + error_msg);
            if(getLocId() < 0) throw std::runtime_error(h5pp::format("Cannot create dataset [{}]: The location ID is not set", dsetPath.value()));
            error_msg.append(debug::reportCompatibility(h5_layout,dsetDims,dsetChunk,dsetDimsMax));
            if(not error_msg.empty()) throw std::runtime_error(h5pp::format("Dataset dimensions are not well defined:\n{}", error_msg));
            /* clang-format on */
        }
        void assertResizeReady() const {
            std::string error_msg;
            /* clang-format off */
            if(dsetExists and dsetPath and not dsetExists.value()) error_msg.append(h5pp::format("\t Dataset does not exist [{}]", dsetPath.value()));
            else if(dsetExists and not dsetExists.value()) error_msg.append("\t Dataset does not exist");
            if(resizeMode and resizeMode == h5pp::ResizeMode::DO_NOT_RESIZE) error_msg.append("\t Resize mode is set to DO_NOT_RESIZE");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot resize dataset.\n{}", error_msg));
            if(not dsetPath            ) error_msg.append("\t dsetPath\n");
            if(not dsetExists          ) error_msg.append("\t dsetExists\n");
            if(not dsetDimsMax         ) error_msg.append("\t dsetDimsMax\n");
            if(not h5_dset             ) error_msg.append("\t h5_dset\n");
            if(not h5_type             ) error_msg.append("\t h5_type\n");
            if(not h5_space            ) error_msg.append("\t h5_space\n");
            if(not h5_layout           ) error_msg.append("\t h5_layout\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot resize dataset. The following fields are undefined:\n{}", error_msg));
            if(not dsetExists.value() ) error_msg.append("\t dsetExists == false\n");
            if(not h5_dset->valid() )   error_msg.append("\t h5_dset\n");
            if(not h5_type->valid() )   error_msg.append("\t h5_type\n");
            if(not h5_space->valid() )  error_msg.append("\t h5_space\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot resize dataset [{}]. The following fields are not valid:\n{}",dsetPath.value(), error_msg));
            /* clang-format on */
        }

        void assertWriteReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dsetPath            ) error_msg.append("\t linkPath\n");
            if(not dsetExists          ) error_msg.append("\t dsetExists\n");
            if(not h5_dset             ) error_msg.append("\t h5_dset\n");
            if(not h5_type             ) error_msg.append("\t h5_type\n");
            if(not h5_space            ) error_msg.append("\t h5_space\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot write into dataset. The following fields are undefined:\n{}", error_msg));
            if(not h5_dset->valid() ) error_msg.append("\t h5_dset\n");
            if(not h5_type->valid() ) error_msg.append("\t h5_type\n");
            if(not h5_space->valid() ) error_msg.append("\t h5_space\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot write into dataset [{}]. The following fields are not valid:\n",dsetPath.value(), error_msg));
            /* clang-format on */
        }
        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dsetPath            ) error_msg.append("\t linkPath\n");
            if(not dsetExists          ) error_msg.append("\t dsetExists\n");
            if(not h5_dset             ) error_msg.append("\t h5_dset\n");
            if(not h5_type             ) error_msg.append("\t h5_type\n");
            if(not h5_space            ) error_msg.append("\t h5_space\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot read from dataset. The following fields are undefined:\n{}",error_msg));
            if(not h5_type->valid() ) error_msg.append("\t h5_type\n");
            if(not h5_space->valid() ) error_msg.append("\t h5_space\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot read from dataset [{}]. The following fields are not valid:\n{}",dsetPath.value(), error_msg));
            if(not dsetExists.value())
                throw std::runtime_error(h5pp::format("Cannot read from dataset [{}]: It does not exist", dsetPath.value()));


            /* clang-format on */
        }
        [[nodiscard]] std::string string() const {
            //            std::string msg;
            std::string msg;
            /* clang-format off */
            if(dsetSize)    msg.append(h5pp::format(" | size {}", dsetSize.value()));
            if(dsetByte)    msg.append(h5pp::format(" | bytes {}", dsetByte.value()));
            if(dsetRank)    msg.append(h5pp::format(" | rank {}", dsetRank.value()));
            if(dsetDims)    msg.append(h5pp::format(" | dims {}", dsetDims.value()));
            if(h5_layout){
                msg.append(" | layout ");
                switch(h5_layout.value()){
                    case H5D_CHUNKED: msg.append(h5pp::format("H5D_CHUNKED")); break;
                    case H5D_CONTIGUOUS: msg.append(h5pp::format("H5D_CONTIGUOUS")); break;
                    case H5D_COMPACT: msg.append(h5pp::format("H5D_COMPACT")); break;
                    default: break;
                }
            }
            if(dsetChunk)   msg.append(h5pp::format(" | chunk dims {}", dsetChunk.value()));
            if(dsetDimsMax){
                std::vector<long> maxDimsLong;
                for(auto &dim : dsetDimsMax.value()) {
                    if(dim == H5S_UNLIMITED)
                        maxDimsLong.emplace_back(-1);
                    else
                        maxDimsLong.emplace_back(static_cast<long>(dim));
                }
                msg.append(h5pp::format(" | max dims {}", maxDimsLong));
            }
            if (h5_space and H5Sget_select_type(h5_space.value()) == H5S_sel_type::H5S_SEL_HYPERSLABS){
                Hyperslab slab(h5_space.value());
                msg.append(h5pp::format(" | [ Hyperslab {} ]", slab.string()));
            }
            if(resizeMode){
                msg.append(" | resize mode ");
                switch(resizeMode.value()){
                    case ResizeMode::RESIZE_TO_FIT: msg.append(h5pp::format("RESIZE_TO_FIT")); break;
                    case ResizeMode::INCREASE_ONLY: msg.append(h5pp::format("INCREASE_ONLY")); break;
                    case ResizeMode::DO_NOT_RESIZE: msg.append(h5pp::format("DO_NOT_RESIZE")); break;
                    default: break;
                }
            }
            if(dsetPath)    msg.append(h5pp::format(" | dset path [{}]",dsetPath.value()));
            return msg;
            /* clang-format on */
        }
    };

    /*!
     * \struct AttrInfo
     * Struct with optional fields describing data on file, i.e. a dataset
     */
    struct AttrInfo {
        std::optional<hid::h5a>             h5_attr              = std::nullopt;
        std::optional<hid::h5o>             h5_link              = std::nullopt;
        std::optional<hid::h5t>             h5_type              = std::nullopt;
        std::optional<hid::h5s>             h5_space             = std::nullopt;
        std::optional<hid::h5p>             h5_plist_attr_create = std::nullopt;
        std::optional<hid::h5p>             h5_plist_attr_access = std::nullopt;
        std::optional<std::string>          attrName             = std::nullopt;
        std::optional<std::string>          linkPath             = std::nullopt;
        std::optional<bool>                 attrExists           = std::nullopt;
        std::optional<bool>                 linkExists           = std::nullopt;
        std::optional<hsize_t>              attrSize             = std::nullopt;
        std::optional<size_t>               attrByte             = std::nullopt;
        std::optional<int>                  attrRank             = std::nullopt;
        std::optional<std::vector<hsize_t>> attrDims             = std::nullopt;
        std::optional<Hyperslab>            attrSlab             = std::nullopt;

        void assertCreateReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not attrName            ) error_msg.append("\t attrName\n");
            if(not linkPath            ) error_msg.append("\t linkPath\n");
            if(not attrExists          ) error_msg.append("\t attrExists\n");
            if(not linkExists          ) error_msg.append("\t linkExists\n");
            if(not h5_link             ) error_msg.append("\t h5_link\n");
            if(not h5_type             ) error_msg.append("\t h5_type\n");
            if(not h5_space            ) error_msg.append("\t h5_space\n");
            if(not h5_plist_attr_create) error_msg.append("\t h5_plist_attr_create\n");
            if(not h5_plist_attr_access) error_msg.append("\t h5_plist_attr_access\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create attribute. The following fields are undefined:\n{}", error_msg));
            if(not linkExists.value())
                throw std::runtime_error(h5pp::format("Cannot create attribute [{}] for link [{}]. The link does not exist",attrName.value(),linkPath.value()));
            if(not h5_link->valid()             ) error_msg.append("\t h5_link\n");
            if(not h5_type->valid()             ) error_msg.append("\t h5_type\n");
            if(not h5_space->valid()            ) error_msg.append("\t h5_space\n");
            if(not h5_plist_attr_create->valid()) error_msg.append("\t h5_plist_attr_create\n");
            if(not h5_plist_attr_access->valid()) error_msg.append("\t h5_plist_attr_access\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create attribute [{}] for link [{}]. The following fields are not valid: {}",attrName.value(),linkPath.value(),error_msg));
            /* clang-format on */
        }

        void assertWriteReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_attr             ) error_msg.append("\t h5_attr\n");
            if(not h5_type             ) error_msg.append("\t h5_type\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create attribute. The following fields are undefined:\n{}", error_msg));
            if(not h5_attr->valid()             ) error_msg.append("\t h5_attr\n");
            if(not h5_type->valid()             ) error_msg.append("\t h5_type\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create attribute [{}] for link [{}]. The following fields are not valid: {}",attrName.value(),linkPath.value(),error_msg));
            /* clang-format on */
        }

        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_attr             ) error_msg.append("\t h5_attr\n");
            if(not h5_type             ) error_msg.append("\t h5_type\n");
            if(not h5_space            ) error_msg.append("\t h5_space\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create attribute. The following fields are undefined:\n{}",error_msg));
            if(not h5_attr->valid()             ) error_msg.append("\t h5_attr\n");
            if(not h5_type->valid()             ) error_msg.append("\t h5_type\n");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create attribute [{}] for link [{}]. The following fields are not valid: {}",attrName.value(),linkPath.value(),error_msg));
            /* clang-format on */
        }

        [[nodiscard]] std::string string() const {
            std::string msg;
            /* clang-format off */
            if(attrSize) msg.append(h5pp::format(" | size {}", attrSize.value()));
            if(attrByte) msg.append(h5pp::format(" | bytes {}", attrByte.value()));
            if(attrRank) msg.append(h5pp::format(" | rank {}", attrRank.value()));
            if(attrDims and not attrDims->empty())
                         msg.append(h5pp::format(" | dims {}", attrDims.value()));
            if(attrName) msg.append(h5pp::format(" | name [{}]",attrName.value()));
            if(linkPath) msg.append(h5pp::format(" | link [{}]",linkPath.value()));
            return msg;
            /* clang-format on */
        }
    };

    /*!
     * \brief Information about tables
     */
    struct TableInfo {
        std::optional<size_t>                   numFields;
        std::optional<size_t>                   numRecords;
        std::optional<size_t>                   recordBytes;
        std::optional<std::vector<std::string>> fieldNames;
        std::optional<std::vector<size_t>>      fieldSizes;
        std::optional<std::vector<size_t>>      fieldOffsets;
        std::optional<std::vector<hid::h5t>>    fieldTypes;
        std::optional<bool>                     tableExists;
        std::optional<std::string>              tableTitle;
        std::optional<std::string>              tablePath;
        std::optional<std::string>              tableGroupName;
        std::optional<hid::h5f>                 tableFile;
        std::optional<hid::h5g>                 tableGroup;
        std::optional<hid::h5o>                 tableObjLoc;
        std::optional<hid::h5d>                 tableDset;
        std::optional<hid::h5t>                 tableType;
        std::optional<size_t>                   compressionLevel;
        std::optional<hsize_t>                  chunkSize;
        hid_t                                   getTableLocId() const {
            if(tableFile) return tableFile.value();
            if(tableGroup) return tableGroup.value();
            if(tableObjLoc) return tableObjLoc.value();
            h5pp::logger::log->debug("Table location is not defined");
            return -1;
        }
        void assertCreateReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not numFields)           error_msg.append("\t numFields\n");
            if(not numRecords)          error_msg.append("\t numRecords\n");
            if(not recordBytes)         error_msg.append("\t recordBytes\n");
            if(not fieldNames)          error_msg.append("\t fieldNames\n");
            if(not fieldSizes)          error_msg.append("\t fieldSizes\n");
            if(not fieldOffsets)        error_msg.append("\t fieldOffsets\n");
            if(not fieldTypes)          error_msg.append("\t fieldTypes\n");
            if(not tablePath)           error_msg.append("\t tablePath\n");
            if(not tableGroupName)      error_msg.append("\t tableGroupName\n");
            if(not tableTitle)          error_msg.append("\t tableTitle\n");
            if(not compressionLevel)    error_msg.append("\t compressionLevel\n");
            if(not chunkSize)           error_msg.append("\t chunkSize\n");
            if(not error_msg.empty()) throw std::runtime_error(h5pp::format("Cannot create new table: The following fields are not set:\n{}", error_msg));
        }
        void assertReadReady() const {
            std::string error_msg;
            if(not recordBytes)         error_msg.append("\t recordBytes\n");
            if(not fieldSizes)          error_msg.append("\t fieldSizes\n");
            if(not fieldOffsets)        error_msg.append("\t fieldOffsets\n");
            if(not tablePath)           error_msg.append("\t tablePath\n");
            if(not error_msg.empty()) throw std::runtime_error(h5pp::format("Cannot read from table: The following fields are not set:\n{}", error_msg));
            if(getTableLocId() < 0) throw std::runtime_error(h5pp::format("Cannot read from table [{}]: The location ID is not set", tablePath.value()));
        }
        void assertWriteReady() const {
            std::string error_msg;
            if(not recordBytes)         error_msg.append("\t recordBytes\n");
            if(not fieldSizes)          error_msg.append("\t fieldSizes\n");
            if(not fieldOffsets)        error_msg.append("\t fieldOffsets\n");
            if(not tablePath)           error_msg.append("\t tablePath\n");
            if(not error_msg.empty()) throw std::runtime_error(h5pp::format("Cannot write to table: The following fields are not set:\n{}", error_msg));
            if(getTableLocId() < 0) throw std::runtime_error(h5pp::format("Cannot write to table [{}]: The location ID is not set", tablePath.value()));
        }
        /* clang-format off */
    };

    /*!
     * \brief Collects type information about existing datasets
     */
    struct TypeInfo {
        std::optional<std::string>          cpp_type_name;
        std::optional<size_t>               cpp_type_bytes;
        std::optional<std::type_index>      cpp_type_index;
        std::optional<std::string>          h5_path;
        std::optional<std::string>          h5_name;
        std::optional<hsize_t>              h5_size;
        std::optional<int>                  h5_rank;
        std::optional<std::vector<hsize_t>> h5_dims;
        std::optional<hid::h5t>             h5_type;
        std::optional<hid::h5o>             h5_link;

        [[nodiscard]] std::string string() {
            std::string msg;
            if(cpp_type_name) msg.append(h5pp::format("C++: type [{}]", cpp_type_name.value()));
            if(cpp_type_bytes) msg.append(h5pp::format(" bytes [{}]", cpp_type_bytes.value()));
            if(not msg.empty()) msg.append(" | HDF5:");
            if(h5_path) msg.append(h5pp::format(" path [{}]", h5_path.value()));
            if(h5_name) msg.append(h5pp::format(" name [{}]", h5_name.value()));
            if(h5_size) msg.append(h5pp::format(" size [{}]", h5_size.value()));
            if(h5_rank) msg.append(h5pp::format(" rank [{}]", h5_rank.value()));
            if(h5_dims) msg.append(h5pp::format(" dims {}", h5_dims.value()));
            return msg;
        }
    };

}
