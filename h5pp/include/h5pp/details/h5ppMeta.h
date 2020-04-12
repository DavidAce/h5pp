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

    struct Options {
        std::optional<std::vector<hsize_t>>       dataDims          = std::nullopt;
        std::optional<std::vector<hsize_t>>       dataDimsMax       = std::nullopt;
        std::optional<std::vector<hsize_t>>       chunkDims         = std::nullopt;
        std::optional<hid::h5t>                   h5_type           = std::nullopt;
        std::optional<H5D_layout_t>               h5_layout         = std::nullopt;
        std::optional<unsigned int>               compression       = std::nullopt;
        std::optional<std::vector<HyperSlab>>     fileSlabs         = std::nullopt;
        std::optional<std::vector<HyperSlab>>     mmrySlabs         = std::nullopt;
        std::optional<std::vector<H5S_seloper_t>> fileSlabSelectOps = std::nullopt;
        std::optional<std::vector<H5S_seloper_t>> mmrySlabSelectOps = std::nullopt;
    };

    /*!
     * \struct MetaData
     * Struct with optional fields describing a C++ data type in memory
     */
    struct MetaData {
        std::optional<size_t>               dataByte = std::nullopt;
        std::optional<hsize_t>              dataSize = std::nullopt;
        std::optional<int>                  dataRank = std::nullopt;
        std::optional<std::vector<hsize_t>> dataDims = std::nullopt;
        std::optional<hid::h5s>             h5_space = std::nullopt;
        std::optional<std::string>          cpp_type = std::nullopt;
        MetaData()                                   = default;
        explicit MetaData(const hid::h5s &space) {
            h5_space = space;
            setFromSpace();
        }
        void setFromSpace() {
            if(not h5_space) return;
            dataRank = H5Sget_simple_extent_ndims(h5_space.value());
            dataDims = std::vector<hsize_t>(dataRank.value(), 0);
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
                throw std::runtime_error("Cannot write from memory. The following meta fields are undefined\n\t" + error_msg);
            if(not h5_space->valid() ) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write from memory. The following meta fields are not valid\n\t" + error_msg);

            /* clang-format on */
            hsize_t size_check = std::accumulate(dataDims->begin(), dataDims->end(), 1, std::multiplies<>());
            if(size_check != dataSize.value()) throw std::runtime_error(h5pp::format("Data size mismatch: dataSize [{}] | size check [{}]", dataSize.value(), size_check));
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
                throw std::runtime_error("Cannot read into memory. The following meta fields are undefined\n\t" + error_msg);
            if(not h5_space->valid() ) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot read into memory. The following meta fields are not valid\n\t" + error_msg);

            /* clang-format on */
            hsize_t size_check = std::accumulate(dataDims->begin(), dataDims->end(), 1, std::multiplies<>());
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
            if(cpp_type) msg.append(h5pp::format(" | type [{}]", cpp_type.value()));
            return msg;
            /* clang-format on */
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
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write into dataset. The following meta fields are undefined\n\t" + error_msg);
            if(not h5_dset->valid() ) error_msg.append("\n\t h5_dset");
            if(not h5_type->valid() ) error_msg.append("\n\t h5_type");
            if(not h5_space->valid() ) error_msg.append("\n\t h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write into dataset. The following meta fields are not valid\n\t" + error_msg);
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
            if(not error_msg.empty())
                throw std::runtime_error("Cannot read from dataset. The following meta fields are undefined\n\t" + error_msg);
            if(not h5_file->valid() ) error_msg.append("\n\t h5_file");
            if(not h5_type->valid() ) error_msg.append("\n\t h5_type");
            if(not h5_space->valid() ) error_msg.append("\n\t h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot read from dataset. The following meta fields are not valid\n\t" + error_msg);
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
            if(dsetDimsMax){
                std::vector<std::string> maxDimStr;
                for(auto &dim : dsetDimsMax.value()) {
                    if(dim == H5S_UNLIMITED)
                        maxDimStr.emplace_back("H5S_UNLIMITED");
                    else
                        maxDimStr.emplace_back(std::to_string(dim));
                }
                msg.append(h5pp::format(" | max dims {}", maxDimStr));
            }
            if(chunkDims)   msg.append(h5pp::format(" | chunk dims {}", chunkDims.value()));
            if(dsetPath)    msg.append(h5pp::format(" | path [{}]",dsetPath.value()));
            return msg;
            /* clang-format on */
        }
    };

    /*!
     * \struct MetaAttr
     * Struct with optional fields describing data on file, i.e. a dataset
     */
    struct MetaAttr {
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

        void assertCreateReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_link             ) error_msg.append("\n\t h5_link");
            if(not h5_type             ) error_msg.append("\n\t h5_type");
            if(not h5_space            ) error_msg.append("\n\t h5_space");
            if(not h5_plist_attr_create) error_msg.append("\n\t h5_plist_attr_create");
            if(not h5_plist_attr_access) error_msg.append("\n\t h5_plist_attr_access");
            if(not attrName            ) error_msg.append("\n\t attrName");
            if(not linkPath            ) error_msg.append("\n\t linkPath");
            if(not attrExists          ) error_msg.append("\n\t attrExists");
            if(not linkExists          ) error_msg.append("\n\t linkExists");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot create attribute. The following meta fields are undefined" + error_msg);
            if(not linkExists.value())
                throw std::runtime_error(h5pp::format("Cannot create attribute [{}] for link [{}]. The link does not exist",attrName.value(),linkPath.value()));
            if(not h5_link->valid()             ) error_msg.append("\n\t h5_link");
            if(not h5_type->valid()             ) error_msg.append("\n\t h5_type");
            if(not h5_space->valid()            ) error_msg.append("\n\t h5_space");
            if(not h5_plist_attr_create->valid()) error_msg.append("\n\t h5_plist_attr_create");
            if(not h5_plist_attr_access->valid()) error_msg.append("\n\t h5_plist_attr_access");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create attribute [{}] for link [{}]. The following fields are not valid: {}",attrName.value(),linkPath.value(),error_msg));
            /* clang-format on */
        }

        void assertWriteReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_attr             ) error_msg.append("\n\t h5_attr");
            if(not h5_type             ) error_msg.append("\n\t h5_type");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot create attribute. The following meta fields are undefined\n\t" + error_msg);
            if(not h5_attr->valid()             ) error_msg.append("\n\t h5_attr");
            if(not h5_type->valid()             ) error_msg.append("\n\t h5_type");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create attribute [{}] for link [{}]. The following fields are not valid: {}",attrName.value(),linkPath.value(),error_msg));
            /* clang-format on */
        }

        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_attr             ) error_msg.append("\n\t h5_attr");
            if(not h5_type             ) error_msg.append("\n\t h5_type");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot create attribute. The following meta fields are undefined\n\t" + error_msg);
            if(not h5_attr->valid()             ) error_msg.append("\n\t h5_attr");
            if(not h5_type->valid()             ) error_msg.append("\n\t h5_type");
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
            if(attrDims) msg.append(h5pp::format(" | dims {}", attrDims.value()));
            if(attrName) msg.append(h5pp::format(" | name [{}]",attrName.value()));
            if(linkPath) msg.append(h5pp::format(" | link [{}]",linkPath.value()));
            return msg;
            /* clang-format on */
        }
    };

}
