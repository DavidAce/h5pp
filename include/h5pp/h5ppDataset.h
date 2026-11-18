#pragma once

#include "h5ppAttribute.h"
#include "h5ppConvert.h"
#include <string>
#include <type_traits>
#include <utility>

namespace h5pp::inline v2 {
    class File;

    template<typename FileType>
    requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    class BasicDataset {
        private:
        hid::h5f                            fileHandle_;
        PropertyLists                       plists_;
        FileAccess                          fileAccess_;
        int                                 compression_;
        std::string                         dsetPath_;
        std::optional<Hyperslab>            dsetSlab_      = std::nullopt;
        std::optional<DatasetCreateOptions> createOptions_ = std::nullopt;

        void requireWritable_(std::string_view operation) const {
            if(fileAccess_ == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to {} on read-only dataset [{}]", operation, dsetPath_);
        }

        public:
        BasicDataset(FileType                           &file,
                     std::string_view                    dsetPath,
                     std::optional<Hyperslab>            dsetSlab      = std::nullopt,
                     std::optional<DatasetCreateOptions> createOptions = std::nullopt)
            : BasicDataset(file.openFileHandle(),
                           file.plists,
                           file.getFileAccess(),
                           file.getCompressionLevel(),
                           dsetPath,
                           std::move(dsetSlab),
                           std::move(createOptions)) {}

        private:
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicGroup;
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicLink;

        BasicDataset(const hid::h5f                     &fileHandle,
                     const PropertyLists                &plists,
                     FileAccess                          fileAccess,
                     int                                 compression,
                     std::string_view                    dsetPath,
                     std::optional<Hyperslab>            dsetSlab      = std::nullopt,
                     std::optional<DatasetCreateOptions> createOptions = std::nullopt)
            : fileHandle_(fileHandle), plists_(plists), fileAccess_(fileAccess), compression_(compression), dsetPath_(dsetPath),
              dsetSlab_(std::move(dsetSlab)), createOptions_(std::move(createOptions)) {}

        public:
        [[nodiscard]] bool     exists() const { return h5pp::hdf5::checkIfLinkExists(fileHandle_, dsetPath_, plists_.linkAccess); }
        [[nodiscard]] DsetInfo getInfo() const {
            Options options;
            options.linkPath = h5pp::util::safe_str(dsetPath_);
            return h5pp::scan::readDsetInfo(fileHandle_, options, plists_);
        }
        [[nodiscard]] std::optional<std::vector<hsize_t>> getChunkDimensions() const {
            auto fileHandle = fileHandle_;
            auto dataset    = h5pp::hdf5::openLink<hid::h5d>(fileHandle, dsetPath_, std::nullopt, plists_.dsetAccess);
            return h5pp::hdf5::getChunkDimensions(dataset);
        }
        [[nodiscard]] TypeInfo getTypeInfo() const {
            return h5pp::hdf5::getTypeInfo(fileHandle_, dsetPath_, std::nullopt, plists_.dsetAccess);
        }
        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes() const {
            return h5pp::hdf5::getTypeInfo_allAttributes(fileHandle_, dsetPath_, std::nullopt, plists_.linkAccess);
        }
        [[nodiscard]] std::vector<std::string> getAttributeNames() const {
            return h5pp::hdf5::getAttributeNames(fileHandle_, dsetPath_, std::nullopt, plists_.linkAccess);
        }
        [[nodiscard]] std::vector<std::string> findAttributes(std::string_view searchKey = "") const {
            std::vector<std::string> matches;
            for(const auto &attrName : getAttributeNames()) {
                if(searchKey.empty() or attrName.find(searchKey) != std::string::npos) matches.emplace_back(attrName);
            }
            return matches;
        }

        [[nodiscard]] BasicDataset ensure(const DatasetCreateOptions &createOptions) requires detail::MutableFile<FileType>
        {
            auto copy           = *this;
            copy.createOptions_ = createOptions;
            return copy;
        }

        [[nodiscard]] DsetInfo create(const DatasetCreateOptions &createOptions) requires detail::MutableFile<FileType>
        {
            requireWritable_("create dataset");
            auto options = detail::makeLegacyOptions(compression_, dsetPath_, createOptions);
            options.assertWellDefined();
            if(not options.linkPath.has_value()) throw h5pp::runtime_error("Error creating dataset: No dataset path specified");
            if(not options.dataDims.has_value())
                throw h5pp::runtime_error("Error creating dataset [{}]: Dimensions or size not specified", options.linkPath.value());
            if(not options.h5Type.has_value())
                throw h5pp::runtime_error("Error creating dataset [{}]: HDF5 type not specified", options.linkPath.value());
            auto fileHandle = fileHandle_;
            auto dsetInfo   = h5pp::scan::makeDsetInfo(fileHandle, options, plists_);
            h5pp::hdf5::createDataset(dsetInfo, plists_);
            return dsetInfo;
        }

        [[nodiscard]] BasicDataset select(const Hyperslab &dsetSlab) const {
            auto copy      = *this;
            copy.dsetSlab_ = dsetSlab;
            return copy;
        }

        [[nodiscard]] BasicDataset select(const DimsType &offset, const DimsType &extent) const {
            return select(Hyperslab(offset, extent));
        }

        [[nodiscard]] BasicAttribute<FileType> attribute(std::string_view attrName) const {
            return BasicAttribute<FileType>(fileHandle_, plists_, fileAccess_, dsetPath_, attrName);
        }

        void resize(const DimsType &dims, std::optional<h5pp::ResizePolicy> resizePolicy = std::nullopt)
            requires detail::MutableFile<FileType>
        {
            requireWritable_("resize dataset");
            Options options;
            options.linkPath     = dsetPath_;
            options.resizePolicy = resizePolicy;
            auto info            = h5pp::scan::inferDsetInfo(fileHandle_, dsetPath_, options, plists_);
            if(not info.dsetExists.value()) throw h5pp::runtime_error("Failed to resize dataset [{}]: dataset does not exist", dsetPath_);
            h5pp::hdf5::resizeDataset(info, dims, resizePolicy);
        }

        template<typename DataType>
        [[nodiscard]] DsetInfo append(const DataType &data, size_t axis, const DatasetAppendOptions &options = DatasetAppendOptions())
            requires detail::MutableFile<FileType>
        {
            if(dsetSlab_.has_value()) throw h5pp::runtime_error("Cannot append through a selected dataset view [{}]", dsetPath_);
            requireWritable_("write");
            auto legacyOptions = detail::makeLegacyOptions(dsetPath_, options);
            auto fileHandle    = fileHandle_;
            auto dsetInfo      = h5pp::scan::readDsetInfo(fileHandle, legacyOptions, plists_);
            auto dataInfo      = h5pp::scan::scanDataInfo(data, legacyOptions);
            h5pp::hdf5::extendDataset(dsetInfo, dataInfo, axis);
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists_);
            return dsetInfo;
        }

        template<typename DataType>
        requires detail::MutableFile<FileType>
        void write(const DataType &data, const DatasetWriteOptions &options = DatasetWriteOptions()) {
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            requireWritable_("write");
            auto legacyOptions = detail::makeLegacyOptions(compression_, dsetPath_, dsetSlab_, createOptions_, options);
            legacyOptions.assertWellDefined();
            auto dataInfo   = h5pp::scan::scanDataInfo(data, legacyOptions);
            auto fileHandle = fileHandle_;
            auto dsetInfo   = h5pp::scan::inferDsetInfo(fileHandle, data, legacyOptions, plists_);
            if(dsetInfo.hasLocId()) h5pp::scan::readDsetInfo(dsetInfo, dsetInfo.getLocId(), legacyOptions, plists_);
            else h5pp::scan::readDsetInfo(dsetInfo, fileHandle, legacyOptions, plists_);
            if(not dsetInfo.dsetExists.has_value() or not dsetInfo.dsetExists.value()) h5pp::hdf5::createDataset(dsetInfo, plists_);
            h5pp::hdf5::resizeDataset(dsetInfo, dataInfo);
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists_);
        }

        template<typename DataType>
        [[nodiscard]] DataType read(const DatasetReadOptions &options = DatasetReadOptions()) const {
            if constexpr(h5pp::type::sfinae::is_specialization_v<DataType, std::optional>) {
                if(not exists()) return std::nullopt;
                return read<typename DataType::value_type>(options);
            }
            DataType data;
            readInto(data, options);
            return data;
        }

        template<typename DataType>
        void readInto(DataType &data, const DatasetReadOptions &options = DatasetReadOptions()) const {
            static_assert(not std::is_const_v<DataType>);
            auto legacyOptions = detail::makeLegacyOptions(dsetPath_, dsetSlab_, options);
            if(not dsetSlab_.has_value()) {
                legacyOptions.assertWellDefined();
                auto fileHandle = fileHandle_;
                auto dsetInfo   = h5pp::scan::readDsetInfo(fileHandle, legacyOptions, plists_);
                auto dataInfo   = h5pp::scan::scanDataInfo(data, legacyOptions);
                h5pp::hdf5::resizeData(data, dataInfo, dsetInfo);
                h5pp::hdf5::readDataset(data, dataInfo, dsetInfo, plists_);
                return;
            }

            auto dsetInfo = h5pp::scan::readDsetInfo(fileHandle_, legacyOptions, plists_);
            if(dsetInfo.dsetExists.has_value() and not dsetInfo.dsetExists.value())
                throw h5pp::runtime_error("Cannot read dataset [{}]: It does not exist", dsetPath_);

            h5pp::DataInfo dataInfo;
            if(legacyOptions.dataDims.has_value()) dataInfo.dataDims = legacyOptions.dataDims;
            else if(dsetInfo.dsetSlab.has_value() and dsetInfo.dsetSlab->extent.has_value()) dataInfo.dataDims = dsetInfo.dsetSlab->extent;
            if(legacyOptions.dataSlab.has_value()) dataInfo.dataSlab = legacyOptions.dataSlab;
            if(legacyOptions.h5Type.has_value()) dataInfo.h5Type = legacyOptions.h5Type;
            else if(dsetInfo.h5Type.has_value()) dataInfo.h5Type = dsetInfo.h5Type;

            if(dataInfo.dataDims.has_value()) {
                h5pp::util::resizeData(data, dataInfo.dataDims.value());
                dataInfo.dataSize = h5pp::util::getSizeFromDimensions(dataInfo.dataDims.value());
                dataInfo.dataRank = h5pp::util::getRankFromDimensions(dataInfo.dataDims.value());
                dataInfo.dataByte = dataInfo.dataSize.value() * h5pp::util::getBytesPerElem<DataType>();
                auto dims         = dataInfo.dataDims.value();
                auto size         = dataInfo.dataSize.value();
                auto bytes        = dataInfo.dataByte.value();
                h5pp::util::setStringSize<DataType>(data, size, bytes, dims);
                dataInfo.dataDims = dims;
                dataInfo.dataSize = size;
                dataInfo.dataByte = bytes;
                dataInfo.h5Space  = h5pp::util::getMemSpace(dataInfo.dataSize.value(), dataInfo.dataDims.value());
                if(dataInfo.dataSlab.has_value()) h5pp::hdf5::selectHyperslab(dataInfo.h5Space.value(), dataInfo.dataSlab.value());
            } else {
                h5pp::scan::scanDataInfo(dataInfo, data, legacyOptions);
            }

            h5pp::hdf5::readDataset(data, dataInfo, dsetInfo, plists_);
        }
    };

    using Dataset      = BasicDataset<h5pp::v2::File>;
    using ConstDataset = BasicDataset<const h5pp::v2::File>;
}
