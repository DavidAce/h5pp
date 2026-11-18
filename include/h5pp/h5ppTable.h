#pragma once

#include "h5ppConvert.h"
#include <memory>
#include <string>
#include <utility>

namespace h5pp::inline v2 {
    class File;

    template<typename FileType>
    requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    class BasicTable {
        private:
        hid::h5f                                           fileHandle_;
        PropertyLists                                      plists_;
        std::shared_ptr<std::vector<ReclaimInfo::Reclaim>> reclaimStack_;
        std::string                                        tablePath_;
        std::optional<TableCreateOptions>                  createOptions_ = std::nullopt;

        [[nodiscard]] Options tableCreateOptions_() const {
            Options options;
            if(not createOptions_.has_value()) return options;
            if(createOptions_->h5Type.has_value()) options.h5Type = createOptions_->h5Type;
            if(createOptions_->dimsChunk.has_value()) options.dsetChunkDims = detail::toOptDimsType(createOptions_->dimsChunk);
            if(createOptions_->compression.has_value())
                options.compression = h5pp::hdf5::getValidCompressionLevel(createOptions_->compression);
            return options;
        }

        void createIfMissing_() requires detail::MutableFile<FileType>
        {
            if(h5pp::hdf5::checkIfLinkExists(fileHandle_, tablePath_, plists_.linkAccess)) return;
            if(not createOptions_.has_value())
                throw h5pp::runtime_error("Cannot create missing table [{}]: no creation options were provided", tablePath_);
            if(not createOptions_->h5Type.has_value())
                throw h5pp::runtime_error("Cannot create missing table [{}]: h5Type is not set", tablePath_);
            if(not createOptions_->title.has_value())
                throw h5pp::runtime_error("Cannot create missing table [{}]: title is not set", tablePath_);

            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            options.h5Type   = createOptions_->h5Type;
            if(createOptions_->dimsChunk.has_value()) options.dsetChunkDims = detail::toOptDimsType(createOptions_->dimsChunk);
            if(createOptions_->compression.has_value())
                options.compression = h5pp::hdf5::getValidCompressionLevel(createOptions_->compression);
            auto info = h5pp::scan::makeTableInfo(fileHandle_, options, createOptions_->title.value(), plists_);
            h5pp::hdf5::createTable(info, plists_);
        }

        public:
        BasicTable(FileType &file, std::string_view tablePath, std::optional<TableCreateOptions> createOptions = std::nullopt)
            : BasicTable(file.openFileHandle(), file.plists, file.reclaimStack, tablePath, std::move(createOptions)) {}

        private:
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicGroup;
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicLink;

        BasicTable(const hid::h5f                                           &fileHandle,
                   const PropertyLists                                      &plists,
                   const std::shared_ptr<std::vector<ReclaimInfo::Reclaim>> &reclaimStack,
                   std::string_view                                          tablePath,
                   std::optional<TableCreateOptions>                         createOptions = std::nullopt)
            : fileHandle_(fileHandle), plists_(plists), reclaimStack_(reclaimStack), tablePath_(tablePath),
              createOptions_(std::move(createOptions)) {}

        public:
        [[nodiscard]] bool      exists() const { return h5pp::hdf5::checkIfLinkExists(fileHandle_, tablePath_, plists_.linkAccess); }
        [[nodiscard]] TableInfo getInfo() const {
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            return h5pp::scan::readTableInfo(fileHandle_, options, plists_);
        }
        [[nodiscard]] TableFieldInfo getFieldInfo() const {
            return h5pp::scan::getTableFieldInfo(fileHandle_, tablePath_, std::nullopt, std::nullopt, plists_.dsetAccess);
        }
        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes() const {
            return h5pp::hdf5::getTypeInfo_allAttributes(fileHandle_, tablePath_, std::nullopt, plists_.linkAccess);
        }
        [[nodiscard]] std::vector<std::string> getAttributeNames() const {
            return h5pp::hdf5::getAttributeNames(fileHandle_, tablePath_, std::nullopt, plists_.linkAccess);
        }
        [[nodiscard]] std::vector<std::string> findAttributes(std::string_view searchKey = "") const {
            std::vector<std::string> matches;
            for(const auto &attrName : getAttributeNames())
                if(searchKey.empty() or attrName.find(searchKey) != std::string::npos) matches.emplace_back(attrName);
            return matches;
        }

        template<typename Fields>
        [[nodiscard]] bool fieldExists(const Fields &fields) const {
            h5pp::NamesOrIndices fieldSelection(fields);
            if(fieldSelection.has_indices())
                return hdf5::checkIfTableFieldsExists(fileHandle_, tablePath_, fieldSelection.get_indices(), plists_);
            if(fieldSelection.has_names())
                return hdf5::checkIfTableFieldsExists(fileHandle_, tablePath_, fieldSelection.get_names(), plists_);
            return false;
        }

        [[nodiscard]] BasicTable ensure(const TableCreateOptions &createOptions) requires detail::MutableFile<FileType>
        {
            auto copy           = *this;
            copy.createOptions_ = createOptions;
            return copy;
        }

        [[nodiscard]] BasicTable ensure(const hid::h5t &h5Type, std::string_view title, const TableCreateOptions &createOptions = {})
            requires detail::MutableFile<FileType>
        {
            auto copy                   = ensure(createOptions);
            copy.createOptions_->h5Type = h5Type;
            copy.createOptions_->title  = h5pp::util::safe_str(title);
            return copy;
        }

        [[nodiscard]] BasicTable create(const TableCreateOptions &createOptions) requires detail::MutableFile<FileType>
        {
            auto copy = ensure(createOptions);
            copy.createIfMissing_();
            return copy;
        }

        [[nodiscard]] BasicTable create(const hid::h5t &h5Type, std::string_view title, const TableCreateOptions &createOptions = {})
            requires detail::MutableFile<FileType>
        {
            auto copy = ensure(h5Type, title, createOptions);
            copy.createIfMissing_();
            return copy;
        }

        template<typename DataType>
        requires detail::MutableFile<FileType>
        void appendRecords(const DataType &data, std::optional<hsize_t> extent = std::nullopt) {
            createIfMissing_();
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            auto info        = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            info.assertWriteReady();
            h5pp::hdf5::writeTableRecords(data, info, info.numRecords.value(), extent);
        }

        template<typename DataType>
        requires detail::MutableFile<FileType>
        void writeRecords(const DataType &data, hsize_t offset = 0, std::optional<hsize_t> extent = std::nullopt) {
            createIfMissing_();
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            auto info        = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            h5pp::hdf5::writeTableRecords(data, info, offset, extent);
        }

        [[nodiscard]] TableInfo appendRecordsFrom(const h5pp::TableInfo &srcInfo, TableSelection selection = TableSelection::ALL)
            requires detail::MutableFile<FileType>
        {
            Options options  = tableCreateOptions_();
            options.linkPath = h5pp::util::safe_str(tablePath_);
            auto tgtInfo     = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            if(not tgtInfo.tableExists.has_value() or not tgtInfo.tableExists.value()) {
                srcInfo.assertCreateReady();
                if(not options.h5Type.has_value()) options.h5Type = srcInfo.h5Type;
                h5pp::scan::makeTableInfo(tgtInfo, fileHandle_, options, srcInfo.tableTitle.value(), plists_);
                h5pp::hdf5::createTable(tgtInfo, plists_);
            }
            tgtInfo.assertWriteReady();
            auto [offset, extent] = util::parseTableSelection(selection, srcInfo.numRecords.value());
            h5pp::hdf5::copyTableRecords(srcInfo, offset, extent, tgtInfo, tgtInfo.numRecords.value(), plists_);
            return tgtInfo;
        }

        template<typename h5x_src>
        [[nodiscard]] TableInfo appendRecordsFrom(const h5x_src   &srcLocation,
                                                  std::string_view srcTablePath,
                                                  TableSelection   selection = TableSelection::ALL) requires detail::MutableFile<FileType>
        {
            Options options  = tableCreateOptions_();
            options.linkPath = h5pp::util::safe_str(srcTablePath);
            auto srcInfo     = h5pp::scan::readTableInfo(srcLocation, options, plists_);
            options.linkPath = h5pp::util::safe_str(tablePath_);
            auto tgtInfo     = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            if(not tgtInfo.tableExists.has_value() or not tgtInfo.tableExists.value()) {
                srcInfo.assertCreateReady();
                if(not options.h5Type.has_value()) options.h5Type = srcInfo.h5Type;
                h5pp::scan::makeTableInfo(tgtInfo, fileHandle_, options, srcInfo.tableTitle.value(), plists_);
                h5pp::hdf5::createTable(tgtInfo, plists_);
            }
            tgtInfo.assertWriteReady();
            auto [offset, extent] = util::parseTableSelection(selection, srcInfo.numRecords.value());
            h5pp::hdf5::copyTableRecords(srcInfo, offset, extent, tgtInfo, tgtInfo.numRecords.value(), plists_);
            return tgtInfo;
        }

        template<typename DataType>
        [[nodiscard]] DataType readRecords(std::optional<hsize_t> offset = std::nullopt,
                                           std::optional<hsize_t> extent = std::nullopt) const {
            if constexpr(h5pp::type::sfinae::is_specialization_v<DataType, std::optional>) {
                if(not exists()) return std::nullopt;
                return readRecords<typename DataType::value_type>(offset, extent);
            }
            DataType data;
            Options  options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            auto info        = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            h5pp::hdf5::readTableRecords(data, info, offset, extent, plists_);
            if(info.reclaimInfo.has_value()) reclaimStack_->emplace_back(info.reclaimInfo.value());
            return data;
        }

        template<typename DataType>
        [[nodiscard]] DataType readRecords(TableSelection selection) const {
            if constexpr(h5pp::type::sfinae::is_specialization_v<DataType, std::optional>) {
                if(not exists()) return std::nullopt;
                return readRecords<typename DataType::value_type>(selection);
            }
            DataType data;
            Options  options;
            options.linkPath      = h5pp::util::safe_str(tablePath_);
            auto info             = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            auto [offset, extent] = util::parseTableSelection(data, selection, info.numRecords, info.recordBytes);
            h5pp::hdf5::readTableRecords(data, info, offset, extent, plists_);
            if(info.reclaimInfo.has_value()) reclaimStack_->emplace_back(info.reclaimInfo.value());
            return data;
        }

        template<typename DataType>
        void readRecordsInto(DataType              &data,
                             std::optional<hsize_t> offset = std::nullopt,
                             std::optional<hsize_t> extent = std::nullopt) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            auto info        = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            h5pp::hdf5::readTableRecords(data, info, offset, extent, plists_);
        }

        template<typename DataType>
        void readRecordsInto(DataType &data, TableSelection selection) const {
            Options options;
            options.linkPath      = h5pp::util::safe_str(tablePath_);
            auto info             = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            auto [offset, extent] = util::parseTableSelection(data, selection, info.numRecords, info.recordBytes);
            h5pp::hdf5::readTableRecords(data, info, offset, extent, plists_);
        }

        template<typename DataType, typename Fields>
        [[nodiscard]] DataType readField(const Fields          &fields,
                                         std::optional<hsize_t> offset = std::nullopt,
                                         std::optional<hsize_t> extent = std::nullopt) const {
            h5pp::NamesOrIndices fieldSelection(fields);
            if constexpr(h5pp::type::sfinae::is_specialization_v<DataType, std::optional>) {
                if(not fieldExists(fieldSelection)) return std::nullopt;
                return readField<typename DataType::value_type>(fieldSelection, offset, extent);
            }
            DataType data;
            Options  options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            auto info        = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            if(fieldSelection.has_indices()) h5pp::hdf5::readTableField(data, info, fieldSelection.get_indices(), offset, extent, plists_);
            else if(fieldSelection.has_names()) h5pp::hdf5::readTableField(data, info, fieldSelection.get_names(), offset, extent, plists_);
            else throw h5pp::runtime_error("No field names or indices have been specified");
            if(info.reclaimInfo.has_value()) reclaimStack_->emplace_back(info.reclaimInfo.value());
            return data;
        }

        template<typename DataType, typename Fields>
        [[nodiscard]] DataType readField(const Fields &fields, TableSelection selection) const {
            h5pp::NamesOrIndices fieldSelection(fields);
            if constexpr(h5pp::type::sfinae::is_specialization_v<DataType, std::optional>) {
                if(not fieldExists(fieldSelection)) return std::nullopt;
                return readField<typename DataType::value_type>(fieldSelection, selection);
            }
            DataType data;
            readFieldInto(data, fieldSelection, selection);
            return data;
        }

        template<typename DataType, typename Fields>
        void readFieldInto(DataType              &data,
                           const Fields          &fields,
                           std::optional<hsize_t> offset = std::nullopt,
                           std::optional<hsize_t> extent = std::nullopt) const {
            h5pp::NamesOrIndices fieldSelection(fields);
            Options              options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            auto info        = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            if(fieldSelection.has_indices()) h5pp::hdf5::readTableField(data, info, fieldSelection.get_indices(), offset, extent, plists_);
            else if(fieldSelection.has_names()) h5pp::hdf5::readTableField(data, info, fieldSelection.get_names(), offset, extent, plists_);
            else throw h5pp::runtime_error("No field names or indices have been specified");
        }

        template<typename DataType, typename Fields>
        void readFieldInto(DataType &data, const Fields &fields, TableSelection selection) const {
            static_assert(not std::is_const_v<DataType>);
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            h5pp::NamesOrIndices fieldSelection(fields);
            auto                 info = getInfo();
            info.assertReadReady();
            hsize_t offset, extent;
            if(fieldSelection.has_indices()) {
                std::tie(offset, extent) = util::parseTableSelection(data, selection, fieldSelection.get_indices(), info);
                h5pp::hdf5::readTableField(data, info, fieldSelection.get_indices(), offset, extent, plists_);
            } else if(fieldSelection.has_names()) {
                std::tie(offset, extent) = util::parseTableSelection(data, selection, fieldSelection.get_names(), info);
                h5pp::hdf5::readTableField(data, info, fieldSelection.get_names(), offset, extent, plists_);
            } else {
                throw h5pp::runtime_error("No field names or indices have been specified");
            }
        }
    };

    using Table      = BasicTable<h5pp::v2::File>;
    using ConstTable = BasicTable<const h5pp::v2::File>;
}
