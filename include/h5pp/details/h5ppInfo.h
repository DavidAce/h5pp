#pragma once
#include "h5ppDebug.h"
#include "h5ppDimensionType.h"
#include "h5ppEnums.h"
#include "h5ppHid.h"
#include "h5ppHyperslab.h"
#include "h5ppLogger.h"
#include "h5ppOptional.h"
#include "h5ppType.h"
#include <hdf5.h>
#include <numeric>
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

        inline auto reportCompatibility(std::optional<H5D_layout_t>         h5Layout,
                                        std::optional<std::vector<hsize_t>> dims,
                                        std::optional<std::vector<hsize_t>> dimsChunk,
                                        std::optional<std::vector<hsize_t>> dimsMax) {
            std::string error_msg;
            if(h5Layout) {
                if(h5Layout.value() == H5D_CHUNKED) {}
                if(h5Layout.value() == H5D_COMPACT) {
                    if(dimsChunk) {
                        error_msg.append(h5pp::format(
                            "Chunk dims {} | Layout is H5D_COMPACT | chunk dimensions are only meant for H5D_CHUNKED layouts\n",
                            dimsChunk.value()));
                    }
                    if(dimsMax and dims and dimsMax.value() != dims.value()) {
                        error_msg.append(h5pp::format("dims {} | max dims {} | layout is H5D_COMPACT | dims and max dims must be equal "
                                                      "unless the layout is H5D_CHUNKED\n",
                                                      dims.value(),
                                                      dimsMax.value()));
                    }
                }
                if(h5Layout.value() == H5D_CONTIGUOUS) {
                    if(dimsChunk) {
                        error_msg.append(h5pp::format("Chunk dims {} | Layout is H5D_CONTIGUOUS | chunk dimensions are only meant for "
                                                      "datasets with H5D_CHUNKED layout \n",
                                                      dimsChunk.value()));
                    }
                    if(dimsMax) {
                        error_msg.append(h5pp::format("Max dims {} | Layout is H5D_CONTIGUOUS | max dimensions are only meant for datasets "
                                                      "with H5D_CHUNKED layout \n",
                                                      dimsMax.value()));
                    }
                }
            }
            std::string res1 = reportCompatibility(dims, dimsMax);
            std::string res2 = reportCompatibility(dims, dimsChunk, DimSizeComparison::PERMISSIVE);
            std::string res3 = reportCompatibility(dimsChunk, dimsMax);
            if(not res1.empty()) error_msg.append(h5pp::format("\t{}: dims {} | max dims {}\n", res1, dims.value(), dimsMax.value()));
            if(not res2.empty()) error_msg.append(h5pp::format("\t{}: dims {} | chunk dims {}\n", res2, dims.value(), dimsChunk.value()));
            if(not res3.empty())
                error_msg.append(h5pp::format("\t{}: chunk dims {} | max dims {}\n", res3, dimsChunk.value(), dimsMax.value()));
            return error_msg;
        }

    }

    struct Options {
        /* clang-format off */
        std::optional<std::string>      linkPath      = std::nullopt; /*!< Path to HDF5 dataset relative to the file root */
        std::optional<std::string>      attrName      = std::nullopt; /*!< Name of attribute on group or dataset */
        OptDimsType                     dataDims      = std::nullopt; /*!< Data dimensions hint. Required for pointer data */
        OptDimsType                     dsetChunkDims = std::nullopt; /*!< (On create) Chunking dimensions. Only valid for H5D_CHUNKED datasets */
        OptDimsType                     dsetMaxDims   = std::nullopt; /*!< (On create) Maximum dimensions. Only valid for H5D_CHUNKED datasets */
        std::optional<Hyperslab>        dsetSlab      = std::nullopt; /*!< Select hyperslab, a subset of the data to participate in transfers to/from the dataset  */
        std::optional<Hyperslab>        attrSlab      = std::nullopt; /*!< Select hyperslab, a subset of the data to participate in transfers to/from the attribute  */
        std::optional<Hyperslab>        dataSlab      = std::nullopt; /*!< Select hyperslab, a subset of the data to participate in transfers to/from memory  */
        std::optional<hid::h5t>         h5Type        = std::nullopt; /*!< (On create) Type of dataset. Override automatic type detection. */
        std::optional<H5D_layout_t>     h5Layout      = std::nullopt; /*!< (On create) Layout of dataset. Choose between H5D_CHUNKED,H5D_COMPACT and H5D_CONTIGUOUS */
        std::optional<int>              compression   = std::nullopt; /*!< (On create) Compression level 0-9, 0 = off, 9 is gives best compression and is slowest */
        std::optional<h5pp::ResizePolicy> resizePolicy    = std::nullopt; /*!< Type of resizing if needed. Choose GROW, TO_FIT,OFF */
        /* clang-format on */
        [[nodiscard]] std::string string(bool enable = true) const {
            if(not enable) return {};
            std::string msg;
            /* clang-format off */
            if(dataDims) msg.append(h5pp::format(" | data dims {}", dataDims.value()));
            if(dsetMaxDims) msg.append(h5pp::format(" | max dims {}", dsetMaxDims.value()));
            if(h5Layout){
                switch(h5Layout.value()){
                    case H5D_CHUNKED: msg.append(h5pp::format(" | H5D_CHUNKED")); break;
                    case H5D_CONTIGUOUS: msg.append(h5pp::format(" | H5D_CONTIGUOUS")); break;
                    case H5D_COMPACT: msg.append(h5pp::format(" | H5D_COMPACT")); break;
                    default: break;
                }
            }
            if(dsetChunkDims) msg.append(h5pp::format(" | chunk dims {}", dsetChunkDims.value()));
            if (dataSlab) msg.append(h5pp::format(" | memory hyperslab {}", dataSlab->string()));
            if (dsetSlab) msg.append(h5pp::format(" | file hyperslab {}", dsetSlab->string()));
            return msg;
            /* clang-format on */
        }

        void assertWellDefined() const {
            std::string error_msg;
            if(not linkPath) error_msg.append("\tMissing field: linkPath\n");
            error_msg.append(debug::reportCompatibility(h5Layout, dataDims, dsetChunkDims, dsetMaxDims));
            if(not error_msg.empty()) throw h5pp::runtime_error("Options are not well defined: \n{}", error_msg);
        }
    };
    struct ReclaimInfo {
        public:
        struct Reclaim {
            /*! This stores metadata required to reclaim (free) any memory that is allocated when HDF5 reads variable-length arrays */

            private:
            std::optional<hid::h5t> type  = std::nullopt; /*!< Identifier of the datatype */
            std::optional<hid::h5s> space = std::nullopt; /*!< Identifier of the dataspace */
            std::optional<hid::h5p> plist = std::nullopt; /*!< Identifier of the property list used to create the buffer */
            void *buf = nullptr; /*!<  Pointer to the buffer to be reclaimed (which was allocated during a variable-length array read) */
            std::string  tag;    /*!< The name of the object that was read */
            mutable long counter = 0; // Counts how many instances of this object there are
            public:
            void drop() {
                type    = std::nullopt;
                space   = std::nullopt;
                plist   = std::nullopt;
                buf     = nullptr;
                tag     = "";
                counter = 0;
            }
            void reclaim() {
                if(buf != nullptr and type and space and plist) {
                    h5pp::logger::log->trace("Reclaiming vlen buffer from reading [{}]", tag);
#if H5_VERSION_GE(1, 12, 0)
                    H5Treclaim(type.value(), space.value(), plist.value(), buf);
#else
                    H5Dvlen_reclaim(type.value(), space.value(), plist.value(), buf);
#endif
                }
                drop();
            }
            Reclaim() = default;
            Reclaim(const hid::h5t &type, const hid::h5s &space, const hid::h5p &plist, void *buf, const std::string &tag)
                : type(type), space(space), plist(plist), buf(buf), tag(tag), counter(1){};
            Reclaim(const Reclaim &r) : type(r.type), space(r.space), plist(r.plist), buf(r.buf), tag(r.tag), counter(r.counter++) {}
            Reclaim(Reclaim &&r) noexcept
                : type(std::move(r.type)), space(std::move(r.space)), plist(std::move(r.plist)), buf(std::move(r.buf)),
                  tag(std::move(r.tag)), counter(std::move(r.counter++)) {}

            Reclaim &operator=(Reclaim &&r) noexcept {
                type    = std::move(r.type);
                space   = std::move(r.space);
                plist   = std::move(r.plist);
                buf     = std::move(r.buf);
                tag     = std::move(r.tag);
                counter = std::move(r.counter++); // ++ makes sure "this" is now in charge of destruction
                return *this;
            }
            Reclaim &operator=(const Reclaim &r) noexcept {
                type    = r.type;
                space   = r.space;
                plist   = r.plist;
                buf     = r.buf;
                tag     = r.tag;
                counter = r.counter++; // ++ makes sure "this" is now in charge of destruction
                return *this;
            }
            ~Reclaim() noexcept {
                counter--;
                if(buf != nullptr and counter == 0) {
                    h5pp::logger::log->warn(
                        "~Reclaim: a pointer for a variable-length array likely remains without free() after reading [{}]. "
                        "Tip: use h5pp::vlen_t or h5pp::vstr_t or call h5pp::File::vlenReclaim() to avoid a memory leak, "
                        "or call h5pp::File::vlenDisableReclaimsTracking() to handle memory manually",
                        tag);
                }
            }
        };

        public:
        mutable std::optional<Reclaim> reclaimInfo = /*!< Used to vlenReclaim any memory allocated for variable-length arrays */
            std::nullopt;

        /*! Calls H5Treclaim(...) on memory that HDF5 allocates when reading variable-length arrays */
        void reclaim() const {
            if(reclaimInfo) reclaimInfo->reclaim();
        }
    };

    /*!
     * \struct DataInfo
     * Struct with optional fields describing a C++ data type in memory
     */
    struct DataInfo {
        std::optional<hsize_t>         dataSize     = std::nullopt;
        std::optional<size_t>          dataByte     = std::nullopt;
        OptDimsType                    dataDims     = std::nullopt;
        std::optional<int>             dataRank     = std::nullopt;
        std::optional<Hyperslab>       dataSlab     = std::nullopt;
        std::optional<hid::h5s>        h5Space      = std::nullopt;
        std::optional<std::string>     cppTypeName  = std::nullopt;
        std::optional<size_t>          cppTypeSize  = std::nullopt;
        std::optional<std::type_index> cppTypeIndex = std::nullopt;

        void setFromSpace() {
            if(not h5Space) return;
            dataRank = H5Sget_simple_extent_ndims(h5Space.value());
            dataDims = std::vector<hsize_t>(type::safe_cast<size_t>(dataRank.value()), 0);
            H5Sget_simple_extent_dims(h5Space.value(), dataDims->data(), nullptr);
        }

        void assertWriteReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dataSize) error_msg.append(" | dataSize");
            if(not dataByte) error_msg.append(" | dataByte");
            if(not dataDims) error_msg.append(" | dataDims");
            if(not dataRank) error_msg.append(" | dataRank");
            if(not h5Space)  error_msg.append(" | h5Space");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot write from memory. The following fields are undefined:\n{}", error_msg);
            if(not h5Space->valid() ) error_msg.append(" | h5Space");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot write from memory. The following fields are not valid:\n{}", error_msg);

            /* clang-format on */
            hsize_t size_check = std::accumulate(dataDims->begin(), dataDims->end(), static_cast<hsize_t>(1), std::multiplies<>());
            if(size_check != dataSize.value()) {
                throw h5pp::runtime_error("Data size mismatch: dataSize [{}] | dataDims {} = size [{}]",
                                          dataSize.value(),
                                          dataDims.value(),
                                          size_check);
            }
        }

        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dataSize) error_msg.append(" | dataSize");
            if(not dataByte) error_msg.append(" | dataByte");
            if(not dataRank) error_msg.append(" | dataRank");
            if(not dataDims) error_msg.append(" | dataDims");
            if(not h5Space)  error_msg.append(" | h5Space");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot read into memory. The following fields are undefined:\n{}", error_msg);
            if(not h5Space->valid() ) error_msg.append(" | h5Space");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot read into memory. The following fields are not valid:\n{}", error_msg);

            /* clang-format on */
            hsize_t size_check = std::accumulate(dataDims->begin(), dataDims->end(), static_cast<hsize_t>(1), std::multiplies<>());
            if(size_check != dataSize.value())
                throw h5pp::runtime_error("Data size mismatch: dataSize [{}] | size check [{}]", dataSize.value(), size_check);
        }
        [[nodiscard]] std::string string(bool enable = true) const {
            if(not enable) return {};
            std::string msg;
            /* clang-format off */
            if(dataSize) msg.append(h5pp::format(" | size {}", dataSize.value()));
            if(dataByte) msg.append(h5pp::format(" | bytes {}", dataByte.value()));
            if(dataRank) msg.append(h5pp::format(" | rank {}", dataRank.value()));
            if(dataDims) msg.append(h5pp::format(" | dims {}", dataDims.value()));
            if(h5Space and H5Sget_select_type(h5Space.value()) == H5S_sel_type::H5S_SEL_HYPERSLABS){
                Hyperslab slab(h5Space.value());
                msg.append(h5pp::format(" | [ Hyperslab {} ]", slab.string()));
            }else if(dataSlab){
                msg.append(h5pp::format(" | [ Hyperslab {} ]", dataSlab->string()));
            }
            if(cppTypeName) msg.append(h5pp::format(" | type [{}]", cppTypeName.value()));
            return msg;
            /* clang-format on */
        }
    };

    /*!
     * \struct DsetInfo
     * Struct with optional fields describing data on file, i.e. a dataset
     */
    struct DsetInfo : public ReclaimInfo {
        std::optional<hid::h5f>           h5File       = std::nullopt;
        std::optional<hid::h5d>           h5Dset       = std::nullopt;
        std::optional<hid::h5t>           h5Type       = std::nullopt;
        std::optional<H5D_layout_t>       h5Layout     = std::nullopt;
        std::optional<hid::h5s>           h5Space      = std::nullopt;
        std::optional<hid::h5p>           h5DsetCreate = std::nullopt; // dataset creation property list
        std::optional<hid::h5p>           h5DsetAccess = std::nullopt; // dataset access property list
        std::optional<H5Z_filter_t>       h5Filters    = std::nullopt;
        std::optional<std::string>        dsetPath     = std::nullopt;
        std::optional<bool>               dsetExists   = std::nullopt;
        std::optional<hsize_t>            dsetSize     = std::nullopt;
        std::optional<size_t>             dsetByte     = std::nullopt;
        std::optional<int>                dsetRank     = std::nullopt;
        OptDimsType                       dsetDims     = std::nullopt;
        OptDimsType                       dsetDimsMax  = std::nullopt;
        OptDimsType                       dsetChunk    = std::nullopt;
        std::optional<Hyperslab>          dsetSlab     = std::nullopt;
        std::optional<h5pp::ResizePolicy> resizePolicy = std::nullopt;
        std::optional<int>                compression  = std::nullopt;
        std::optional<std::string>        cppTypeName  = std::nullopt;
        std::optional<size_t>             cppTypeSize  = std::nullopt;
        std::optional<std::type_index>    cppTypeIndex = std::nullopt;

        [[nodiscard]] hid::h5f getLocId() const {
            if(h5File) return h5File.value();
            if(h5Dset) return H5Iget_file_id(h5Dset.value());
            h5pp::logger::log->debug("Dataset location id is not defined");
            return static_cast<hid_t>(0);
        }
        [[nodiscard]] bool hasLocId() const { return h5File.has_value() or h5Dset.has_value(); }
        void               assertCreateReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dsetPath               ) error_msg.append("\t dsetPath\n");
            if(not dsetExists             ) error_msg.append("\t dsetExists\n");
            if(not h5Type                 ) error_msg.append("\t h5Type\n");
            if(not h5Space                ) error_msg.append("\t h5Space\n");
            if(not h5DsetCreate           ) error_msg.append("\t h5PlistDsetCreate\n");
            if(not h5DsetAccess           ) error_msg.append("\t h5PlistDsetAccess\n");
            if(not error_msg.empty())     throw h5pp::runtime_error("Cannot create dataset. The following fields are undefined:\n{}",error_msg);
            if(not h5Type->valid()        ) error_msg.append("\t h5Type\n");
            if(not h5Space->valid()       ) error_msg.append("\t h5Space\n");
            if(not h5DsetCreate->valid()  ) error_msg.append("\t h5PlistDsetCreate\n");
            if(not h5DsetAccess->valid()  ) error_msg.append("\t h5PlistDsetAccess\n");
            if(not error_msg.empty())     throw h5pp::runtime_error("Cannot create dataset. The following fields are not valid\n\t{}", error_msg);
            if(not hasLocId())            throw h5pp::runtime_error("Cannot create dataset [{}]: The location ID is not set", dsetPath.value());
            error_msg.append(debug::reportCompatibility(h5Layout,dsetDims,dsetChunk,dsetDimsMax));
            if(not error_msg.empty()) throw h5pp::runtime_error("Dataset dimensions are not well defined:\n{}", error_msg);
            /* clang-format on */
        }
        void assertResizeReady() const {
            std::string error_msg;
            /* clang-format off */
            if(dsetExists and dsetPath and not dsetExists.value()) error_msg.append(h5pp::format("\t Dataset does not exist [{}]", dsetPath.value()));
            else if(dsetExists and not dsetExists.value()) error_msg.append("\t Dataset does not exist");
            if(resizePolicy and resizePolicy == h5pp::ResizePolicy::OFF) error_msg.append("\t Resize policy is [OFF]");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot resize dataset.\n{}", error_msg);
            if(not dsetPath           ) error_msg.append("\t dsetPath\n");
            if(not dsetExists         ) error_msg.append("\t dsetExists\n");
            if(not dsetDimsMax        ) error_msg.append("\t dsetDimsMax\n");
            if(not h5Dset             ) error_msg.append("\t h5Dset\n");
            if(not h5Type             ) error_msg.append("\t h5Type\n");
            if(not h5Space            ) error_msg.append("\t h5Space\n");
            if(not h5Layout           ) error_msg.append("\t h5Layout\n");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot resize dataset. The following fields are undefined:\n{}", error_msg);
            if(not dsetExists.value() ) error_msg.append("\t dsetExists == false\n");
            if(not h5Dset->valid() )   error_msg.append("\t h5Dset\n");
            if(not h5Type->valid() )   error_msg.append("\t h5Type\n");
            if(not h5Space->valid() )  error_msg.append("\t h5Space\n");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot resize dataset [{}]. The following fields are not valid:\n{}",dsetPath.value(), error_msg);
            /* clang-format on */
        }

        void assertWriteReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dsetPath           ) error_msg.append("\t dsetPath\n");
            if(not dsetExists         ) error_msg.append("\t dsetExists\n");
            if(not h5Dset             ) error_msg.append("\t h5Dset\n");
            if(not h5Type             ) error_msg.append("\t h5Type\n");
            if(not h5Space            ) error_msg.append("\t h5Space\n");
            if(not h5DsetCreate       ) error_msg.append("\t h5DsetCreate\n");
            if(not h5DsetAccess       ) error_msg.append("\t h5DsetAccess\n");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot write into dataset. The following fields are undefined:\n{}", error_msg);
            if(not dsetExists.value() ) error_msg.append("\t dsetExists == false\n");
            if constexpr(not h5pp::ndebug){
                if(not h5Dset->valid()  ) error_msg.append("\t h5Dset\n");
                if(not h5Type->valid()  ) error_msg.append("\t h5Type\n");
                if(not h5Space->valid() ) error_msg.append("\t h5Space\n");
            }
            /* clang-format on */
            if(not error_msg.empty()) {
                throw h5pp::runtime_error("Cannot write into dataset [{}]. The following fields are not valid:\n",
                                          dsetPath.value(),
                                          error_msg);
            }
        }
        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dsetPath           ) error_msg.append("\t dsetPath\n");
            if(not dsetExists         ) error_msg.append("\t dsetExists\n");
            if(not h5Dset             ) error_msg.append("\t h5Dset\n");
            if(not h5Type             ) error_msg.append("\t h5Type\n");
            if(not h5Space            ) error_msg.append("\t h5Space\n");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot read from dataset. The following fields are undefined:\n{}",error_msg);
            if(not h5Type->valid() ) error_msg.append("\t h5Type\n");
            if(not h5Space->valid() ) error_msg.append("\t h5Space\n");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot read from dataset [{}]. The following fields are not valid:\n{}",dsetPath.value(), error_msg);
            if(not dsetExists.value())
                throw h5pp::runtime_error("Cannot read from dataset [{}]: It may not exist", dsetPath.value());

            /* clang-format on */
        }
        [[nodiscard]] std::string string(bool enable = true) const {
            if(not enable) return {};
            std::string msg;
            /* clang-format off */
            if(dsetSize)    msg.append(h5pp::format(" | size {}", dsetSize.value()));
            if(dsetByte)    msg.append(h5pp::format(" | bytes {}", dsetByte.value()));
            if(dsetRank)    msg.append(h5pp::format(" | rank {}", dsetRank.value()));
            if(dsetDims)    msg.append(h5pp::format(" | dims {}", dsetDims.value()));
            if(h5Layout){
                msg.append(" | layout ");
                switch(h5Layout.value()){
                    case H5D_CHUNKED: msg.append(h5pp::format("H5D_CHUNKED")); break;
                    case H5D_CONTIGUOUS: msg.append(h5pp::format("H5D_CONTIGUOUS")); break;
                    case H5D_COMPACT: msg.append(h5pp::format("H5D_COMPACT")); break;
                    default: break;
                }
            }
            if(dsetChunk)   msg.append(h5pp::format(" | chunk dims {}", dsetChunk.value()));
            if(dsetDimsMax){
                std::vector<long> dsetDimsMaxPretty;
                for(auto &dim : dsetDimsMax.value()) {
                    if(dim == H5S_UNLIMITED)
                        dsetDimsMaxPretty.emplace_back(-1);
                    else
                        dsetDimsMaxPretty.emplace_back(type::safe_cast<long>(dim));
                }
                msg.append(h5pp::format(" | max dims {}", dsetDimsMaxPretty));
            }
            if(h5Space and H5Sget_select_type(h5Space.value()) == H5S_sel_type::H5S_SEL_HYPERSLABS){
                Hyperslab slab(h5Space.value());
                msg.append(h5pp::format(" | [ Hyperslab {} ]", slab.string()));
            }else if(dsetSlab){
                msg.append(h5pp::format(" | [ Hyperslab {} ]", dsetSlab->string()));
            }
            if(resizePolicy){
                msg.append(" | resize mode ");
                switch(resizePolicy.value()){
                    case ResizePolicy::FIT: msg.append(h5pp::format("FIT")); break;
                    case ResizePolicy::GROW: msg.append(h5pp::format("GROW")); break;
                    case ResizePolicy::OFF: msg.append(h5pp::format("OFF")); break;
                    default: break;
                }
            }
            if(compression) msg.append(h5pp::format(" | compression {}", compression.value()));
            if(dsetPath)    msg.append(h5pp::format(" | dset path [{}]",dsetPath.value()));
            if(cppTypeName) msg.append(h5pp::format(" | c++ type [{}]",cppTypeName.value()));
            if(cppTypeSize) msg.append(h5pp::format(" | c++ size [{}] bytes",cppTypeSize.value()));
            return msg;
            /* clang-format on */
        }
    };

    /*!
     * \struct AttrInfo
     * Struct with optional fields describing an attribute on file
     */
    struct AttrInfo : public ReclaimInfo {
        std::optional<hid::h5f>             h5File            = std::nullopt;
        std::optional<hid::h5o>             h5Link            = std::nullopt;
        std::optional<hid::h5a>             h5Attr            = std::nullopt;
        std::optional<hid::h5t>             h5Type            = std::nullopt;
        std::optional<hid::h5s>             h5Space           = std::nullopt;
        std::optional<hid::h5p>             h5PlistAttrCreate = std::nullopt;
        std::optional<hid::h5p>             h5PlistAttrAccess = std::nullopt;
        std::optional<std::string>          attrName          = std::nullopt;
        std::optional<std::string>          linkPath          = std::nullopt;
        std::optional<bool>                 attrExists        = std::nullopt;
        std::optional<bool>                 linkExists        = std::nullopt;
        std::optional<hsize_t>              attrSize          = std::nullopt;
        std::optional<size_t>               attrByte          = std::nullopt;
        std::optional<int>                  attrRank          = std::nullopt;
        std::optional<std::vector<hsize_t>> attrDims          = std::nullopt;
        std::optional<Hyperslab>            attrSlab          = std::nullopt;
        std::optional<std::string>          cppTypeName       = std::nullopt;
        std::optional<size_t>               cppTypeSize       = std::nullopt;
        std::optional<std::type_index>      cppTypeIndex      = std::nullopt;

        [[nodiscard]] hid::h5f getLocId() const {
            if(h5File) return h5File.value();
            if(h5Link) return H5Iget_file_id(h5Link.value());
            if(h5Attr) return H5Iget_file_id(h5Attr.value());
            h5pp::logger::log->debug("Attribute location id is not defined");
            return static_cast<hid_t>(0);
        }
        [[nodiscard]] bool hasLocId() const { return h5File.has_value() or h5Link.has_value() or h5Attr.has_value(); }

        void assertCreateReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not attrName           ) error_msg.append("\t attrName\n");
            if(not linkPath           ) error_msg.append("\t linkPath\n");
            if(not attrExists         ) error_msg.append("\t attrExists\n");
            if(not linkExists         ) error_msg.append("\t linkExists\n");
            if(not h5Link             ) error_msg.append("\t h5Link\n");
            if(not h5Type             ) error_msg.append("\t h5Type\n");
            if(not h5Space            ) error_msg.append("\t h5Space\n");
            if(not h5PlistAttrCreate  ) error_msg.append("\t h5PlistAttrCreate\n");
            if(not h5PlistAttrAccess  ) error_msg.append("\t h5PlistAttrAccess\n");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot create attribute. The following fields are undefined:\n{}", error_msg);
            if(not linkExists.value())
                throw h5pp::runtime_error("Cannot create attribute [{}] for link [{}]. The link does not exist",attrName.value(),linkPath.value());
            if(not h5Link->valid()           ) error_msg.append("\t h5Link\n");
            if(not h5Type->valid()           ) error_msg.append("\t h5Type\n");
            if(not h5Space->valid()          ) error_msg.append("\t h5Space\n");
            if(not h5PlistAttrCreate->valid()) error_msg.append("\t h5PlistAttrCreate\n");
            if(not h5PlistAttrAccess->valid()) error_msg.append("\t h5PlistAttrAccess\n");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot create attribute [{}] for link [{}]. The following fields are not valid: {}",
                                            attrName.value(),linkPath.value(),error_msg);
            /* clang-format on */
        }

        void assertWriteReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5Attr             ) error_msg.append("\t h5Attr\n");
            if(not h5Type             ) error_msg.append("\t h5Type\n");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot create attribute. The following fields are undefined:\n{}", error_msg);
            if(not h5Attr->valid()    ) error_msg.append("\t h5Attr\n");
            if(not h5Type->valid()    ) error_msg.append("\t h5Type\n");
            if(not error_msg.empty())
                throw h5pp::runtime_error(
                        "Cannot create attribute [{}] for link [{}]. The following fields are not valid: {}",
                        attrName.value(),linkPath.value(),error_msg);
            /* clang-format on */
        }

        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5Attr             ) error_msg.append("\t h5Attr\n");
            if(not h5Type             ) error_msg.append("\t h5Type\n");
            if(not h5Space            ) error_msg.append("\t h5Space\n");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot create attribute. The following fields are undefined:\n{}",error_msg);
            if(not h5Attr->valid()             ) error_msg.append("\t h5Attr\n");
            if(not h5Type->valid()             ) error_msg.append("\t h5Type\n");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot create attribute [{}] for link [{}]. The following fields are not valid: {}",attrName.value(),linkPath.value(),error_msg);
            /* clang-format on */
        }

        [[nodiscard]] std::string string(bool enable = true) const {
            if(not enable) return {};
            std::string msg;
            /* clang-format off */
            if(attrSize) msg.append(h5pp::format(" | size {}", attrSize.value()));
            if(attrByte) msg.append(h5pp::format(" | bytes {}", attrByte.value()));
            if(attrRank) msg.append(h5pp::format(" | rank {}", attrRank.value()));
            if(attrDims) msg.append(h5pp::format(" | dims {}", attrDims.value()));
            if(attrName) msg.append(h5pp::format(" | name [{}]",attrName.value()));
            if(linkPath) msg.append(h5pp::format(" | link [{}]",linkPath.value()));
            if(h5Space and H5Sget_select_type(h5Space.value()) == H5S_sel_type::H5S_SEL_HYPERSLABS){
                Hyperslab slab(h5Space.value());
                msg.append(h5pp::format(" | [ Hyperslab {} ]", slab.string()));
            }else if(attrSlab){
                msg.append(h5pp::format(" | [ Hyperslab {} ]", attrSlab->string()));
            }
            return msg;
            /* clang-format on */
        }
    };

    /*!
     * \brief Information about table fields
     */
    struct TableFieldInfo : public ReclaimInfo {
        std::optional<size_t>                       recordBytes  = std::nullopt;
        std::optional<hsize_t>                      numFields    = std::nullopt;
        std::optional<std::vector<std::string>>     fieldNames   = std::nullopt;
        std::optional<std::vector<size_t>>          fieldSizes   = std::nullopt;
        std::optional<std::vector<size_t>>          fieldOffsets = std::nullopt;
        std::optional<std::vector<hid::h5t>>        fieldTypes   = std::nullopt;
        std::optional<std::vector<H5T_class_t>>     fieldClasses = std::nullopt;
        std::optional<std::vector<std::string>>     cppTypeName  = std::nullopt;
        std::optional<std::vector<size_t>>          cppTypeSize  = std::nullopt;
        std::optional<std::vector<std::type_index>> cppTypeIndex = std::nullopt;
        /* clang-format off */
        virtual void assertCreateReady() const{
            std::string error_msg;
            if(not numFields)    error_msg.append("\t numFields\n");
            if(not recordBytes)  error_msg.append("\t recordBytes\n");
            if(not fieldNames)   error_msg.append("\t fieldNames\n");
            if(not fieldSizes)   error_msg.append("\t fieldSizes\n");
            if(not fieldOffsets) error_msg.append("\t fieldOffsets\n");
            if(not fieldTypes)   error_msg.append("\t fieldTypes\n");
            if(not fieldClasses) error_msg.append("\t fieldClasses\n");
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot create new table: The following fields are not set:\n{}", error_msg);
        }
        virtual void assertReadReady() const {
            std::string error_msg;
            if(not numFields)    error_msg.append("\t numFields\n");
            if(not recordBytes)  error_msg.append("\t recordBytes\n");
            if(not fieldNames)   error_msg.append("\t fieldNames\n");
            if(not fieldSizes)   error_msg.append("\t fieldSizes\n");
            if(not fieldTypes)   error_msg.append("\t fieldTypes\n");
            if(not fieldClasses) error_msg.append("\t fieldClasses\n");
            if(not fieldOffsets) error_msg.append("\t fieldOffsets\n");
            if(not error_msg.empty()) throw h5pp::runtime_error("Cannot read from table: The following fields are not set:\n{}", error_msg);
        }
        virtual void assertWriteReady() const {
            std::string error_msg;
            if(not numFields)    error_msg.append("\t numFields\n");
            if(not recordBytes)  error_msg.append("\t recordBytes\n");
            if(not fieldSizes)   error_msg.append("\t fieldSizes\n");
            if(not fieldOffsets) error_msg.append("\t fieldOffsets\n");
            if(not error_msg.empty()) throw h5pp::runtime_error("Cannot write to table: The following fields are not set:\n{}", error_msg);
        }

        [[nodiscard]] virtual std::string string(bool enable = true) const {
            if(not enable) return {};
            std::string msg;
            if(numFields)    msg.append(h5pp::format(" | fields [{}]", numFields.value()));
            if(fieldSizes)   msg.append(h5pp::format(" | sizes [{}]", fieldSizes.value()));
            if(fieldOffsets) msg.append(h5pp::format(" | offsets [{}]", fieldOffsets.value()));
            return msg;
        }
        [[nodiscard]] std::string string_fields(bool enable = true) const {
            if(not enable) return {};
            std::string msg;
            /* clang-format off */
            if(numFields){
                msg.append(h5pp::format("Fields ({}):\n", numFields.value()));
                for(size_t m = 0; m < type::safe_cast<size_t>(numFields.value()); m++){
                    if(fieldNames and fieldNames->size() > m) msg.append(h5pp::format("{:<32} ", fieldNames->operator[](m)));
                    if(fieldTypes and fieldTypes->size() > m) msg.append(h5pp::format("| {:<16} ", type::getH5TypeName(fieldTypes->operator[](m))));
                    if(fieldSizes and fieldSizes->size() > m) msg.append(h5pp::format("| extent {:<4} bytes ", fieldSizes->operator[](m)));
                    if(fieldOffsets and fieldOffsets->size() > m) msg.append(h5pp::format("| offset {:<4} bytes", fieldOffsets->operator[](m)));
                    msg.append(".\n");
                }
            }
            return msg;
            /* clang-format on */
        }
        /* clang-format on */
    };

    /*!
     * \brief Information about tables
     */
    struct TableInfo : public TableFieldInfo {
        std::optional<hid::h5f>     h5File         = std::nullopt;
        std::optional<hid::h5d>     h5Dset         = std::nullopt;
        std::optional<hid::h5t>     h5Type         = std::nullopt;
        std::optional<hid::h5p>     h5DsetCreate   = std::nullopt; // dataset creation property list
        std::optional<hid::h5p>     h5DsetAccess   = std::nullopt; // dataset access property list
        std::optional<H5Z_filter_t> h5Filters      = std::nullopt;
        std::optional<std::string>  tableTitle     = std::nullopt;
        std::optional<std::string>  tablePath      = std::nullopt;
        std::optional<std::string>  tableGroupName = std::nullopt;
        std::optional<hsize_t>      numRecords     = std::nullopt;
        OptDimsType                 chunkDims      = std::nullopt;
        std::optional<bool>         tableExists    = std::nullopt;
        std::optional<int>          compression    = std::nullopt;

        [[nodiscard]] hid::h5f getLocId() const {
            if(h5File) return h5File.value();
            if(h5Dset) return H5Iget_file_id(h5Dset.value());
            h5pp::logger::log->debug("Table location is not defined");
            return static_cast<hid_t>(0);
        }

        [[nodiscard]] bool hasLocId() const { return h5File.has_value() or h5Dset.has_value(); }

        /* clang-format off */
        void assertCreateReady() const override {
            std::string error_msg;
            if(not tableTitle)          error_msg.append("\t tableTitle\n");
            if(not tablePath)           error_msg.append("\t tablePath\n");
            if(not tableGroupName)      error_msg.append("\t tableGroupName\n");
            if(not numRecords)          error_msg.append("\t numRecords\n");
            if(not recordBytes)         error_msg.append("\t recordBytes\n");
            if(not compression)         error_msg.append("\t compression\n");
            if(not chunkDims)           error_msg.append("\t chunkDims\n");
            if(not error_msg.empty()) throw h5pp::runtime_error("Cannot create new table: The following fields are not set:\n{}", error_msg);
            if(not hasLocId())        throw h5pp::runtime_error("Cannot create new table [{}]: The location ID is not set", tablePath.value());
            TableFieldInfo::assertCreateReady();
        }
        void assertReadReady() const override {
            std::string error_msg;
            if(not h5Dset)              error_msg.append("\t h5Dset\n");
            if(not h5Type)              error_msg.append("\t h5Type\n");
            if(not tablePath)           error_msg.append("\t tablePath\n");
            if(not tableExists)         error_msg.append("\t tableExists\n");
            if(not numRecords)          error_msg.append("\t numRecords\n");
            if(not recordBytes)         error_msg.append("\t recordBytes\n");
            if(not error_msg.empty()) throw h5pp::runtime_error("Cannot read from table: The following fields are not set:\n{}", error_msg);
            if(not tableExists.value()) throw h5pp::runtime_error("Cannot read from table [{}]: it may not exist", tablePath.value());
            TableFieldInfo::assertReadReady();
        }
        void assertWriteReady() const override {
            std::string error_msg;
            if(not tablePath)           error_msg.append("\t tablePath\n");
            if(not h5Dset)              error_msg.append("\t h5Dset\n");
            if(not h5Type)              error_msg.append("\t h5Type\n");
            if(not h5DsetCreate)        error_msg.append("\t h5DsetCreate\n");
            if(not h5DsetAccess)        error_msg.append("\t h5DsetAccess\n");
            if(not tableExists)         error_msg.append("\t tableExists\n");
            if(not numRecords)          error_msg.append("\t numRecords\n");
            if(not recordBytes)         error_msg.append("\t recordBytes\n");
            if(not error_msg.empty()) throw h5pp::runtime_error("Cannot write to table: The following fields are not set:\n{}", error_msg);
            if(not tableExists.value()) throw h5pp::runtime_error("Cannot write to table [{}]: it may not exist", tablePath.value());
            TableFieldInfo::assertWriteReady();
        }
        [[nodiscard]] std::string string(bool enable = true) const override {
            if(not enable) return {};
            std::string msg;
            if(tableTitle) msg.append(h5pp::format(" | title [{}]", tableTitle.value()));
            if(numRecords) msg.append(h5pp::format(" | num records [{}]", numRecords.value()));
            if(chunkDims)  msg.append(h5pp::format(" | chunk dims [{}]", chunkDims.value()));
            if(tablePath)  msg.append(h5pp::format(" | path [{}]",tablePath.value()));
            return msg;
        }
        /* clang-format on */
    };

    /*!
     * \brief Collects type information about existing datasets
     */
    struct TypeInfo {
        std::optional<std::string>          cppTypeName;
        std::optional<size_t>               cppTypeBytes;
        std::optional<std::type_index>      cppTypeIndex;
        std::optional<std::string>          h5Path;
        std::optional<std::string>          h5Name;
        std::optional<hsize_t>              h5Size;
        std::optional<int>                  h5Rank;
        std::optional<std::vector<hsize_t>> h5Dims;
        std::optional<hid::h5t>             h5Type;
        std::optional<hid::h5o>             h5Link;

        [[nodiscard]] std::string string(bool enable = true) const {
            if(not enable) return {};
            std::string msg;
            if(cppTypeName) msg.append(h5pp::format("C++: type [{}]", cppTypeName.value()));
            if(cppTypeBytes) msg.append(h5pp::format(" bytes [{}]", cppTypeBytes.value()));
            if(not msg.empty()) msg.append(" | HDF5:");
            if(h5Path) msg.append(h5pp::format(" path [{}]", h5Path.value()));
            if(h5Name) msg.append(h5pp::format(" name [{}]", h5Name.value()));
            if(h5Size) msg.append(h5pp::format(" size [{}]", h5Size.value()));
            if(h5Rank) msg.append(h5pp::format(" rank [{}]", h5Rank.value()));
            if(h5Dims) msg.append(h5pp::format(" dims {}", h5Dims.value()));
            return msg;
        }
    };

    struct LinkInfo {
        std::optional<hid::h5f>       h5File       = std::nullopt;
        std::optional<hid::h5o>       h5Link       = std::nullopt;
        std::optional<std::string>    linkPath     = std::nullopt;
        std::optional<bool>           linkExists   = std::nullopt;
        std::optional<H5O_hdr_info_t> h5HdrInfo    = std::nullopt; /*!< Information struct for object header metadata */
        std::optional<hsize_t>        h5HdrByte    = std::nullopt; /*!< Total space for storing object header in file */
        std::optional<H5O_type_t>     h5ObjType    = std::nullopt; /*!< Object type (dataset, group etc)              */
        std::optional<H5L_type_t>     h5LinkType   = std::nullopt; /*!< Link type (hard, soft, external)              */
        std::optional<unsigned>       refCount     = std::nullopt; /*!< Reference count of object                     */
        std::optional<time_t>         atime        = std::nullopt; /*!< Access time                                   */
        std::optional<time_t>         mtime        = std::nullopt; /*!< Modification time                             */
        std::optional<time_t>         ctime        = std::nullopt; /*!< Change time                                   */
        std::optional<time_t>         btime        = std::nullopt; /*!< Birth time                                    */
        std::optional<hsize_t>        num_attrs    = std::nullopt; /*!< Number of attributes attached to object       */
        std::optional<int64_t>        corder       = std::nullopt; /*!< Link creation order position                  */
        std::optional<hbool_t>        corder_valid = std::nullopt; /*!< Whether corder can assumed to be valid        */
        std::optional<H5T_cset_t>     cset         = std::nullopt; /*!< Character set encoding for the linkPath       */

        [[nodiscard]] std::string string(bool enable = true) const {
            if(not enable) return {};
            std::string msg;
            /* clang-format off */
            if(refCount)  msg.append(h5pp::format(" | refCount {}", refCount.value()));
            if(h5HdrByte) msg.append(h5pp::format(" | header bytes {}", h5HdrByte.value()));
            if(linkPath)  msg.append(h5pp::format(" | link [{}]", linkPath.value()));
            return msg;
            /* clang-format on */
        }

        [[nodiscard]] hid::h5f getLocId() const {
            if(h5File) return h5File.value();
            if(h5Link) return H5Iget_file_id(h5Link.value());
            h5pp::logger::log->debug("Header location id is not defined");
            return static_cast<hid_t>(0);
        }
        [[nodiscard]] bool hasLocId() const { return h5File.has_value() or h5Link.has_value(); }

        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5File)     error_msg.append("\t h5File\n");
            if(not h5Link)     error_msg.append("\t h5Link\n");
            if(not linkPath)   error_msg.append("\t linkPath\n");
            if(not linkExists) error_msg.append("\t linkExists\n");
            if(not h5HdrInfo)  error_msg.append("\t h5HdrInfo\n");
            if(not h5HdrByte)  error_msg.append("\t h5HdrByte\n");
            if(not h5ObjType)  error_msg.append("\t h5ObjType\n");
            if(not refCount)   error_msg.append("\t refCount\n");
            if(not atime)      error_msg.append("\t atime\n");
            if(not mtime)      error_msg.append("\t mtime\n");
            if(not ctime)      error_msg.append("\t ctime\n");
            if(not btime)      error_msg.append("\t btime\n");
            if(not num_attrs)  error_msg.append("\t num_attrs\n");
            /* clang-format on */
            if(not error_msg.empty())
                throw h5pp::runtime_error("Cannot read from LinkInfo: The following fields are not set:\n{}", error_msg);
        }
    };

    struct H5TInfo {
        std::optional<hid::h5t>                 h5Type       = std::nullopt;
        std::optional<H5T_class_t>              h5Class      = std::nullopt;
        std::optional<size_t>                   typeSize     = std::nullopt;
        std::optional<int>                      numMembers   = std::nullopt;
        std::optional<std::vector<std::string>> memberNames  = std::nullopt;
        std::optional<std::vector<hid::h5t>>    memberTypes  = std::nullopt;
        std::optional<std::vector<size_t>>      memberSizes  = std::nullopt;
        std::optional<std::vector<size_t>>      memberOffset = std::nullopt;
        std::optional<std::vector<int>>         memberIndex  = std::nullopt;
        std::optional<std::vector<H5T_class_t>> memberClass  = std::nullopt;
        [[nodiscard]] std::string               string(bool enable = true) const {
            if(not enable) return {};
            std::string msg;
            /* clang-format off */
            if(h5Type)   msg.append(h5pp::format(" | h5Type {}", type::getH5TypeName(h5Type.value())));
            if(h5Class)  msg.append(h5pp::format(" | h5Class {}", h5pp::enum2str(h5Class.value())));
            if(typeSize) msg.append(h5pp::format(" | typeSize {} bytes", typeSize.value()));
            if(numMembers) msg.append(h5pp::format(" | numMembers {}", numMembers.value()));
            return msg;
            /* clang-format on */
        }
        [[nodiscard]] std::string string_members(bool enable = true) const {
            if(not enable) return {};
            std::string msg;
            /* clang-format off */
            std::string msg = fmt::format("{1:4}: {2:{0}} | {3:6} | {4:6} | {5:16} | {6:24}\n", nwidth, "idx", "name", "size", "offset", "class", "type");
            for(size_t m = 0; m < type::safe_cast<size_t>(numMembers.value()); ++m) {
                msg += fmt::format("{1:4}: {2:{0}} | {3:6} | {4:6} | {5:16} | {6:24}\n", nwidth,
                                         m,
                                         memberNames and memberNames->size() > m ? memberNames->at(m) : "",
                                         memberSizes and memberSizes->size() > m ? memberSizes->at(m) : -1ul,
                                         memberOffset and memberOffset->size() > m ? memberOffset->at(m) : -1ul,
                                         memberClass and memberClass->size() > m ? h5pp::type::getH5ClassName(memberClass->at(m)) : "",
                                         memberTypes and memberTypes->size() > m ? h5pp::type::getH5TypeName(memberTypes->at(m)) : ""
                                         );
            }
            /* clang-format on */
            return msg;
            /* clang-format on */
        }
    };
}
