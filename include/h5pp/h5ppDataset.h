#pragma once

#include "h5ppAttribute.h"
#include "h5ppConvert.h"
#include <string>
#include <type_traits>
#include <utility>

namespace h5pp::inline v2 {
    class File;

    /**
     * @brief Dataset handle.
     *
     * Obtain a Dataset with write methods from non-const File::dataset() or Group::dataset().
     * ConstDataset is obtained from a const File and provides read-only methods.
     * A dataset on file is created when calling create() or write() if it does not exist.
     *
     * Each dataset object keeps the file open and remains usable after the File
     * object is destroyed. Release these objects before moving the file with
     * File::moveFileTo().
     *
     * @code{.cpp}
     * auto dataset = file.dataset("measurements");
     * dataset.write(std::vector<double>{1.0, 2.0, 3.0});
     * auto values = dataset.read<std::vector<double>>();
     * auto sample = dataset.select({1}, {1}).read<std::vector<double>>();
     * @endcode
     *
     * @tparam FileType File for Dataset, or const File for ConstDataset.
     */
    template<typename FileType>
    requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    class BasicDataset {
        private:
        hid::h5f                 fileHandle_;
        PropertyLists            plists_;
        FileAccess               fileAccess_;
        int                      compression_;
        std::string              dsetPath_;
        std::optional<Hyperslab> dsetSlab_ = std::nullopt;

        void requireWritable_(std::string_view operation) const {
            if(fileAccess_ == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to {} on read-only dataset [{}]", operation, dsetPath_);
        }

        public:
        /**
         * @brief Refer to a dataset path without creating the dataset.
         * @param file File containing the dataset. Its file handle is retained by this object.
         * @param dsetPath Dataset path relative to the file root.
         */
        BasicDataset(FileType &file, std::string_view dsetPath)
            : BasicDataset(file.openFileHandle(), file.plists, file.getFileAccess(), file.getCompressionLevel(), dsetPath) {}

        private:
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicGroup;
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicLink;

        BasicDataset(const hid::h5f      &fileHandle,
                     const PropertyLists &plists,
                     FileAccess           fileAccess,
                     int                  compression,
                     std::string_view     dsetPath)
            : fileHandle_(fileHandle), plists_(plists), fileAccess_(fileAccess), compression_(compression), dsetPath_(dsetPath) {}

        public:
        //! Object path relative to the file root.
        [[nodiscard]] const std::string &getPath() const { return dsetPath_; }
        //! Object name without its parent path.
        [[nodiscard]] std::string getName() const { return dsetPath_ == "/" ? "/" : dsetPath_.substr(dsetPath_.find_last_of('/') + 1); }
        //! Parent group path; "/" for objects in the file root.
        [[nodiscard]] std::string getParentPath() const {
            auto parent = util::getParentPath(dsetPath_);
            return parent.empty() ? "/" : std::string(parent);
        }
        //! Owning file path.
        [[nodiscard]] std::string getFilePath() const { return hdf5::getFilePath(fileHandle_); }
        //! Owning filename.
        [[nodiscard]] std::string getFileName() const { return fs::path(getFilePath()).filename().string(); }
        //! File access policy.
        [[nodiscard]] FileAccess  getFileAccess() const { return fileAccess_; }

        //! Check whether a link exists at the dataset path.
        [[nodiscard]] bool     exists() const { return h5pp::hdf5::checkIfLinkExists(fileHandle_, dsetPath_, plists_.linkAccess); }
        //! Read metadata for the whole dataset, regardless of the current selection.
        [[nodiscard]] DsetInfo getInfo() const {
            Options options;
            options.linkPath = h5pp::util::safe_str(dsetPath_);
            return h5pp::scan::readDsetInfo(fileHandle_, options, plists_);
        }
        //! Read chunk dimensions, or return std::nullopt if the dataset is not chunked.
        [[nodiscard]] std::optional<std::vector<hsize_t>> getChunkDimensions() const {
            auto fileHandle = fileHandle_;
            auto dataset    = h5pp::hdf5::openLink<hid::h5d>(fileHandle, dsetPath_, std::nullopt, plists_.dsetAccess);
            return h5pp::hdf5::getChunkDimensions(dataset);
        }
        //! Read datatype information for the dataset.
        [[nodiscard]] TypeInfo getTypeInfo() const {
            return h5pp::hdf5::getTypeInfo(fileHandle_, dsetPath_, std::nullopt, plists_.dsetAccess);
        }
        //! Read datatype information for all attributes on the dataset.
        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes() const {
            return h5pp::hdf5::getTypeInfo_allAttributes(fileHandle_, dsetPath_, std::nullopt, plists_.linkAccess);
        }
        //! List attribute names on the dataset.
        [[nodiscard]] std::vector<std::string> getAttributeNames() const {
            return h5pp::hdf5::getAttributeNames(fileHandle_, dsetPath_, std::nullopt, plists_.linkAccess);
        }
        /**
         * @brief Find attribute names containing a substring.
         * @param searchKey Substring to match. An empty string returns all names.
         */
        [[nodiscard]] std::vector<std::string> findAttributes(std::string_view searchKey = "") const {
            std::vector<std::string> matches;
            for(const auto &attrName : getAttributeNames())
                if(searchKey.empty() or attrName.find(searchKey) != std::string::npos) matches.emplace_back(attrName);
            return matches;
        }

        /**
         * @brief Create the dataset with explicit dimensions and datatype, without writing data.
         *
         * An existing dataset is left unchanged. Creation options do not change its
         * datatype, layout, chunk dimensions or compression.
         *
         * @param createOptions Requires dims and h5Type; other creation properties are optional.
         * @return A copy of this object, for chained calls.
         * @see create(const DataType &, const DatasetCreateOptions &)
         */
        BasicDataset create(const DatasetCreateOptions &createOptions) requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("create dataset");
            auto options = detail::makeOptions(compression_, dsetPath_, createOptions);
            options.assertWellDefined();
            if(not options.linkPath.has_value()) throw h5pp::runtime_error("Error creating dataset: No dataset path specified");
            if(not options.dataDims.has_value())
                throw h5pp::runtime_error("Error creating dataset [{}]: Dimensions or size not specified", options.linkPath.value());
            if(not options.h5Type.has_value())
                throw h5pp::runtime_error("Error creating dataset [{}]: HDF5 type not specified", options.linkPath.value());
            auto fileHandle = fileHandle_;
            auto dsetInfo   = h5pp::scan::makeDsetInfo(fileHandle, options, plists_);
            h5pp::hdf5::createDataset(dsetInfo, plists_);
            return *this;
        }

        /**
         * @brief Create the dataset, inferring missing dimensions and datatype from data.
         *
         * The values in data are not written. An existing dataset is left unchanged.
         * Call write() afterward to store values.
         *
         * @param data Data used to infer the shape and datatype.
         * @param createOptions Override inferred properties or specify layout, chunking and compression.
         *                     Pointer data requires explicit dims.
         * @return A copy of this object, for chained calls.
         */
        template<typename DataType>
        BasicDataset create(const DataType &data, const DatasetCreateOptions &createOptions = {})
            requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("create dataset");
            auto options = detail::makeOptions(compression_, dsetPath_, createOptions);
            options.assertWellDefined();
            auto dsetInfo = h5pp::scan::inferDsetInfo(fileHandle_, data, options, plists_);
            h5pp::hdf5::createDataset(dsetInfo, plists_);
            return *this;
        }

        /**
         * @brief Return a copy with a file selection for subsequent reads and writes.
         *
         * This object's selection is unchanged. The selection does not restrict
         * metadata queries, attributes or resize(). Appending through a selection is not allowed.
         *
         * @param dsetSlab Hyperslab in the dataset's coordinates. Replaces any previous selection on the copy.
         */
        BasicDataset select(const Hyperslab &dsetSlab) const {
            auto copy      = *this;
            copy.dsetSlab_ = dsetSlab;
            return copy;
        }

        /**
         * @brief Select a rectangular part of the dataset in a returned copy.
         * @param offset Zero-based starting coordinates.
         * @param extent Number of elements to select along each dimension.
         * @see select(const Hyperslab &)
         */
        BasicDataset select(const DimsType &offset, const DimsType &extent) const { return select(Hyperslab(offset, extent)); }

        /**
         * @brief Refer to an attribute on this dataset without creating it.
         * @param attrName Attribute name. The dataset selection does not affect attributes.
         */
        BasicAttribute<FileType> attribute(std::string_view attrName) const {
            return BasicAttribute<FileType>(fileHandle_, plists_, fileAccess_, dsetPath_, attrName);
        }

        /**
         * @brief Resize an existing chunked dataset within its maximum dimensions.
         *
         * Shrinking discards values outside the new dimensions. This acts on the whole
         * dataset, regardless of any selection. An OFF policy leaves its size unchanged.
         *
         * @param dims Requested dataset dimensions.
         * @param resizePolicy Defaults to FIT for chunked datasets and OFF for other layouts.
         *                     GROW allows increases only.
         * @return A copy of this object, for chained calls.
         */
        BasicDataset resize(const DimsType &dims, std::optional<h5pp::ResizePolicy> resizePolicy = std::nullopt)
            requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("resize dataset");
            Options options;
            options.linkPath     = dsetPath_;
            options.resizePolicy = resizePolicy;
            auto info            = h5pp::scan::inferDsetInfo(fileHandle_, dsetPath_, options, plists_);
            if(not info.dsetExists.value()) throw h5pp::runtime_error("Failed to resize dataset [{}]: dataset does not exist", dsetPath_);
            h5pp::hdf5::resizeDataset(info, dims, resizePolicy);
            return *this;
        }

        /**
         * @brief Extend an existing chunked dataset along one dimension and write data at the end.
         *
         * The extension must fit within the maximum dimensions set at creation.
         * Appending through a selected dataset object is not allowed.
         *
         * @param data Values to append.
         * @param axis Zero-based dimension to extend.
         * @param options Dimensions and datatype of the input buffer. Specify dims for pointer data.
         * @return A copy of this object, for chained calls.
         */
        template<typename DataType>
        BasicDataset append(const DataType &data, size_t axis, const DatasetAppendOptions &options = DatasetAppendOptions())
            requires traits::is_mutable_file_v<FileType>
        {
            if(dsetSlab_.has_value()) throw h5pp::runtime_error("Cannot append through a selected dataset view [{}]", dsetPath_);
            requireWritable_("write");
            auto ioOptions  = detail::makeOptions(dsetPath_, options);
            auto fileHandle = fileHandle_;
            auto dsetInfo   = h5pp::scan::readDsetInfo(fileHandle, ioOptions, plists_);
            auto dataInfo   = h5pp::scan::scanDataInfo(data, ioOptions);
            h5pp::hdf5::extendDataset(dsetInfo, dataInfo, axis);
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists_);
            return *this;
        }

        /**
         * @brief Write data to the dataset or its current selection, creating the dataset if absent.
         *
         * Dimensions and datatype are inferred from data unless overridden. To set layout,
         * chunking or compression, call create() before write(). By default, existing
         * chunked datasets resize to fit a full write, or grow to accommodate a selection.
         *
         * @param data Values to write.
         * @param options Override buffer dimensions, datatype, memory selection or resize policy.
         *                Specify dims for pointer data.
         * @return A copy of this object, for chained calls.
         */
        template<typename DataType>
        requires traits::is_mutable_file_v<FileType>
        BasicDataset write(const DataType &data, const DatasetWriteOptions &options = DatasetWriteOptions()) {
            static_assert(not traits::is_h5pp_id<DataType>);
            requireWritable_("write");
            auto ioOptions = detail::makeOptions(compression_, dsetPath_, dsetSlab_, options);
            ioOptions.assertWellDefined();
            auto dataInfo = h5pp::scan::scanDataInfo(data, ioOptions);
            if constexpr(not std::is_pointer_v<DataType> and not traits::is_text_v<DataType>) {
                if(dataInfo.dataSize.value() > h5pp::util::getSize(data))
                    throw h5pp::runtime_error("Buffer dimensions exceed the supplied data size");
            }
            auto fileHandle      = fileHandle_;
            auto creationOptions = ioOptions;
            if(ioOptions.dataSlab.has_value() and ioOptions.dataSlab->extent.has_value()) {
                creationOptions.dataDims = ioOptions.dataSlab->extent;
                if(ioOptions.dataSlab->blocks.has_value())
                    for(size_t axis = 0; axis < creationOptions.dataDims->size(); ++axis)
                        creationOptions.dataDims->at(axis) *= ioOptions.dataSlab->blocks->at(axis);
            }
            if(dsetSlab_.has_value() and dsetSlab_->offset.has_value() and dsetSlab_->extent.has_value()) {
                creationOptions.dataDims = dsetSlab_->offset;
                for(size_t axis = 0; axis < creationOptions.dataDims->size(); ++axis) {
                    auto count  = dsetSlab_->extent->at(axis);
                    auto stride = dsetSlab_->stride.has_value() ? dsetSlab_->stride->at(axis) : hsize_t{1};
                    auto block  = dsetSlab_->blocks.has_value() ? dsetSlab_->blocks->at(axis) : hsize_t{1};
                    creationOptions.dataDims->at(axis) += count == 0 ? 0 : (count - 1) * stride + block;
                }
            }
            auto dsetInfo = h5pp::scan::inferDsetInfo(fileHandle, data, creationOptions, plists_);
            if(dsetInfo.hasLocId()) h5pp::scan::readDsetInfo(dsetInfo, dsetInfo.getLocId(), ioOptions, plists_);
            else h5pp::scan::readDsetInfo(dsetInfo, fileHandle, ioOptions, plists_);
            if(not dsetInfo.dsetExists.has_value() or not dsetInfo.dsetExists.value()) h5pp::hdf5::createDataset(dsetInfo, plists_);
            h5pp::hdf5::resizeDataset(dsetInfo, dataInfo);
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists_);
            return *this;
        }

        /**
         * @brief Write a selection from the input buffer.
         *
         * The memory and dataset selections must contain the same number of elements.
         * Their shapes may differ. Omit select() to write the whole dataset.
         * A missing dataset is created with the packed memory-selection shape, or with
         * dimensions large enough to contain the file selection when select() is used.
         *
         * @code{.cpp}
         * std::vector<double> data(100, 1.0);
         * h5pp::Hyperslab dataSlab({1, 1}, {2, 2});
         * file.dataset("matrix").select({3, 4}, {2, 2}).write(data, dataSlab, {.dims = h5pp::Dims{10, 10}});
         * @endcode
         *
         * @param data Input buffer.
         * @param dataSlab Selection in the buffer's coordinates.
         * @param options Use dims for a flat buffer interpreted as multidimensional data, or for a pointer.
         *                dataSlab takes precedence over options.dataSlab.
         * @return A copy of this object, for chained calls.
         */
        template<typename DataType, typename SlabType>
        requires(traits::is_mutable_file_v<FileType> and traits::type_is<SlabType, Hyperslab>)
        BasicDataset write(const DataType &data, const SlabType &dataSlab, const DatasetWriteOptions &options = {}) {
            auto ioOptions     = options;
            ioOptions.dataSlab = dataSlab;
            return write(data, ioOptions);
        }

        /**
         * @brief Read the dataset or its current selection into a returned value.
         *
         * With DataType = std::optional<T>, a missing path returns std::nullopt.
         * Other read errors are still reported as exceptions.
         *
         * @tparam DataType Type of the returned data.
         * @param options Override buffer dimensions, datatype or memory selection.
         */
        template<typename DataType>
        [[nodiscard]] DataType read(const DatasetReadOptions &options = DatasetReadOptions()) const {
            if constexpr(h5pp::traits::is_specialization_v<DataType, std::optional>) {
                if(not exists()) return std::nullopt;
                return read<typename DataType::value_type>(options);
            }
            DataType data;
            readInto(data, options);
            return data;
        }

        /**
         * @brief Read the dataset or its current selection into a supplied buffer.
         *
         * Resizable containers are resized to fit the requested data. Arrays and pointer
         * buffers are not resized and must have enough space for the read.
         * With a memory selection, the buffer retains its shape unless options.dims is specified.
         *
         * @param data Destination buffer.
         * @param options Override buffer dimensions, datatype or memory selection.
         *                Specify dims for pointer data.
         */
        template<typename DataType>
        void readInto(DataType &data, const DatasetReadOptions &options = DatasetReadOptions()) const {
            static_assert(not std::is_const_v<DataType>);
            auto ioOptions = detail::makeOptions(dsetPath_, dsetSlab_, options);
            if(ioOptions.dataSlab.has_value()) {
                ioOptions.assertWellDefined();
                if(ioOptions.dataDims.has_value()) h5pp::util::resizeData(data, ioOptions.dataDims.value());
                auto dsetInfo = h5pp::scan::readDsetInfo(fileHandle_, ioOptions, plists_);
                auto dataInfo = h5pp::scan::scanDataInfo(data, ioOptions);
                if constexpr(not std::is_pointer_v<DataType> and not traits::is_text_v<DataType>) {
                    if(dataInfo.dataSize.value() > h5pp::util::getSize(data))
                        throw h5pp::runtime_error("Buffer dimensions exceed the supplied data size");
                }
                h5pp::hdf5::readDataset(data, dataInfo, dsetInfo, plists_);
                return;
            }
            if(not dsetSlab_.has_value()) {
                ioOptions.assertWellDefined();
                auto fileHandle = fileHandle_;
                auto dsetInfo   = h5pp::scan::readDsetInfo(fileHandle, ioOptions, plists_);
                auto dataInfo   = h5pp::scan::scanDataInfo(data, ioOptions);
                h5pp::hdf5::resizeData(data, dataInfo, dsetInfo);
                h5pp::hdf5::readDataset(data, dataInfo, dsetInfo, plists_);
                return;
            }

            auto dsetInfo = h5pp::scan::readDsetInfo(fileHandle_, ioOptions, plists_);
            if(dsetInfo.dsetExists.has_value() and not dsetInfo.dsetExists.value())
                throw h5pp::runtime_error("Cannot read dataset [{}]: It does not exist", dsetPath_);

            h5pp::DataInfo dataInfo;
            if(ioOptions.dataDims.has_value()) dataInfo.dataDims = ioOptions.dataDims;
            else if(dsetInfo.dsetSlab.has_value() and dsetInfo.dsetSlab->extent.has_value()) dataInfo.dataDims = dsetInfo.dsetSlab->extent;
            if(ioOptions.dataSlab.has_value()) dataInfo.dataSlab = ioOptions.dataSlab;
            if(ioOptions.h5Type.has_value()) dataInfo.h5Type = ioOptions.h5Type;
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
                h5pp::scan::scanDataInfo(dataInfo, data, ioOptions);
            }

            h5pp::hdf5::readDataset(data, dataInfo, dsetInfo, plists_);
        }
        /**
         * @brief Read into a selection of the destination buffer.
         *
         * Elements outside the memory selection are unchanged. Without options.dims,
         * the buffer keeps its current shape. Omit select() to read the whole dataset.
         *
         * @param data Destination buffer.
         * @param dataSlab Selection in the buffer's coordinates; must match the dataset selection's element count.
         * @param options Use dims for a flat buffer interpreted as multidimensional data, or for a pointer.
         *                dataSlab takes precedence over options.dataSlab.
         */
        template<typename DataType, typename SlabType>
        requires traits::type_is<SlabType, Hyperslab>
        void readInto(DataType &data, const SlabType &dataSlab, const DatasetReadOptions &options = {}) const {
            auto ioOptions     = options;
            ioOptions.dataSlab = dataSlab;
            readInto(data, ioOptions);
        }
    };

    using Dataset      = BasicDataset<h5pp::v2::File>;       /*!< Dataset operations obtained from a non-const File. */
    using ConstDataset = BasicDataset<const h5pp::v2::File>; /*!< Dataset reads and inspection obtained from a const File. */
}
