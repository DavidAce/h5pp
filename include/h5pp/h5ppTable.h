#pragma once

#include "h5ppAttribute.h"
#include <memory>
#include <string>
#include <utility>

namespace h5pp::inline v2 {
    class File;

    /**
     * @brief Table handle.
     *
     * A table stores records of an HDF5 compound datatype in a one-dimensional,
     * extendible dataset. Obtain a Table with write methods from a non-const File
     * or Group. ConstTable provides read-only methods.
     *
     * Call create() before writing records. Each table object keeps the file open
     * and remains usable after the File object is destroyed. Release these objects
     * before File::moveFileTo().
     *
     * @code{.cpp}
     * struct Particle { double mass; };
     * h5pp::hid::h5t recordType = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
     * H5Tinsert(recordType, "mass", HOFFSET(Particle, mass), H5T_NATIVE_DOUBLE);
     * std::vector<Particle> records{{1.0}, {2.0}};
     * auto table = file.table("particles").create(recordType, "Particle records");
     * table.appendRecords(records);
     * auto copy = table.readRecords<std::vector<Particle>>();
     * auto masses = table.readField<std::vector<double>>("mass");
     * @endcode
     *
     * @tparam FileType File for Table, or const File for ConstTable.
     */
    template<typename FileType>
    requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    class BasicTable {
        private:
        hid::h5f                                           fileHandle_;
        PropertyLists                                      plists_;
        FileAccess                                         fileAccess_;
        std::shared_ptr<std::vector<ReclaimInfo::Reclaim>> reclaimStack_;
        std::string                                        tablePath_;

        void requireWritable_(std::string_view operation) const {
            if(fileAccess_ == FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to {} on read-only table [{}]", operation, tablePath_);
        }

        public:
        /**
         * @brief Refer to a table without creating it.
         * @param file File containing the table.
         * @param tablePath Table path relative to the file root.
         */
        BasicTable(FileType &file, std::string_view tablePath)
            : BasicTable(file.openFileHandle(), file.plists, file.getFileAccess(), file.reclaimStack, tablePath) {}

        private:
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicGroup;
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicLink;

        BasicTable(const hid::h5f                                           &fileHandle,
                   const PropertyLists                                      &plists,
                   FileAccess                                                fileAccess,
                   const std::shared_ptr<std::vector<ReclaimInfo::Reclaim>> &reclaimStack,
                   std::string_view                                          tablePath)
            : fileHandle_(fileHandle), plists_(plists), fileAccess_(fileAccess), reclaimStack_(reclaimStack), tablePath_(tablePath) {}

        public:
        [[nodiscard]] const std::string &getPath() const { return tablePath_; } /*!< Object path relative to the file root. */
        [[nodiscard]] std::string        getName() const {
            return tablePath_ == "/" ? "/" : tablePath_.substr(tablePath_.find_last_of('/') + 1);
        } /*!< Object name without its parent path. */
        [[nodiscard]] std::string getParentPath() const {
            auto parent = util::getParentPath(tablePath_);
            return parent.empty() ? "/" : std::string(parent);
        } /*!< Parent group path; "/" for objects in the file root. */
        [[nodiscard]] std::string getFilePath() const { return hdf5::getFilePath(fileHandle_); }              /*!< Owning file path. */
        [[nodiscard]] std::string getFileName() const { return fs::path(getFilePath()).filename().string(); } /*!< Owning filename. */
        [[nodiscard]] FileAccess  getFileAccess() const { return fileAccess_; }                               /*!< File access policy. */

        [[nodiscard]] bool exists() const {
            return h5pp::hdf5::checkIfLinkExists(fileHandle_, tablePath_, plists_.linkAccess);
        } /*!< Check whether a link exists at the table path. */
        [[nodiscard]] TableInfo getInfo() const {
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            return h5pp::scan::readTableInfo(fileHandle_, options, plists_);
        } /*!< Read table metadata, including record count and compound datatype. */
        [[nodiscard]] TableFieldInfo getFieldInfo() const {
            return h5pp::scan::getTableFieldInfo(fileHandle_, tablePath_, std::nullopt, std::nullopt, plists_.dsetAccess);
        } /*!< Read field names, datatypes, byte offsets and sizes. */
        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes() const {
            return h5pp::hdf5::getTypeInfo_allAttributes(fileHandle_, tablePath_, std::nullopt, plists_.linkAccess);
        } /*!< Read datatype information for attributes on the table. */
        [[nodiscard]] std::vector<std::string> getAttributeNames() const {
            return h5pp::hdf5::getAttributeNames(fileHandle_, tablePath_, std::nullopt, plists_.linkAccess);
        } /*!< List attribute names on the table. */
        /**
         * @brief Find attribute names on the table containing a substring.
         * @param searchKey Substring to match. An empty string returns all names.
         */
        [[nodiscard]] std::vector<std::string> findAttributes(std::string_view searchKey = "") const {
            std::vector<std::string> matches;
            for(const auto &attrName : getAttributeNames())
                if(searchKey.empty() or attrName.find(searchKey) != std::string::npos) matches.emplace_back(attrName);
            return matches;
        }

        BasicAttribute<FileType> attribute(std::string_view attrName) const {
            return BasicAttribute<FileType>(fileHandle_, plists_, fileAccess_, tablePath_, attrName);
        } /*!< Refer to an attribute on the table without creating it. */

        [[nodiscard]] TypeInfo getTypeInfo() const {
            return h5pp::hdf5::getTypeInfo(fileHandle_, tablePath_, std::nullopt, plists_.dsetAccess);
        } /*!< Read datatype information for the compound record type. */

        template<typename DataType>
        requires traits::is_mutable_file_v<FileType>
        BasicTable write(const DataType &data, hsize_t offset = 0, std::optional<hsize_t> extent = std::nullopt) {
            return writeRecords(data, offset, extent);
        } /*!< Write records at offset, extending the table if necessary. See writeRecords(). */

        template<typename DataType>
        requires traits::is_mutable_file_v<FileType>
        BasicTable append(const DataType &data, std::optional<hsize_t> extent = std::nullopt) {
            return appendRecords(data, extent);
        } /*!< Append records to an existing table. See appendRecords(). */

        template<typename DataType>
        [[nodiscard]] DataType read(std::optional<hsize_t> offset = std::nullopt, std::optional<hsize_t> extent = std::nullopt) const {
            return readRecords<DataType>(offset, extent);
        } /*!< Read records into a returned value. See readRecords(). */

        template<typename DataType>
        [[nodiscard]] DataType read(TableSelection selection) const {
            return readRecords<DataType>(selection);
        } /*!< Read ALL, FIRST or LAST records into a returned value. */

        template<typename DataType>
        void readInto(DataType &data, std::optional<hsize_t> offset = std::nullopt, std::optional<hsize_t> extent = std::nullopt) const {
            readRecordsInto(data, offset, extent);
        } /*!< Read records into a supplied buffer. See readRecordsInto(). */

        template<typename DataType>
        void readInto(DataType &data, TableSelection selection) const {
            readRecordsInto(data, selection);
        } /*!< Read ALL, FIRST or LAST records into a supplied buffer. */

        /**
         * @brief Check whether all requested fields exist.
         * @param fields Field names or zero-based indices, accepted by NamesOrIndices.
         */
        template<typename Fields>
        [[nodiscard]] bool fieldExists(const Fields &fields) const {
            h5pp::NamesOrIndices fieldSelection(fields);
            if(fieldSelection.has_indices())
                return hdf5::checkIfTableFieldsExists(fileHandle_, tablePath_, fieldSelection.get_indices(), plists_);
            if(fieldSelection.has_names())
                return hdf5::checkIfTableFieldsExists(fileHandle_, tablePath_, fieldSelection.get_names(), plists_);
            return false;
        }

        /**
         * @brief Create an empty table with the specified compound datatype and title.
         *
         * An existing table is left unchanged, including its title and creation properties.
         *
         * @param createOptions Requires h5Type and title; chunk dimensions and compression are optional.
         * @return A copy of this object, for chained calls.
         */
        BasicTable create(const TableCreateOptions &createOptions) requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("create table");
            if(exists()) return *this;
            if(not createOptions.h5Type.has_value()) throw h5pp::runtime_error("Cannot create table [{}]: h5Type is not set", tablePath_);
            if(not createOptions.title.has_value()) throw h5pp::runtime_error("Cannot create table [{}]: title is not set", tablePath_);

            Options options;
            options.linkPath      = h5pp::util::safe_str(tablePath_);
            options.h5Type        = createOptions.h5Type;
            options.dsetChunkDims = detail::toOptDimsType(createOptions.dimsChunk);
            if(createOptions.compression.has_value()) options.compression = h5pp::hdf5::getValidCompressionLevel(createOptions.compression);
            auto info = h5pp::scan::makeTableInfo(fileHandle_, options, createOptions.title.value(), plists_);
            h5pp::hdf5::createTable(info, plists_);
            return *this;
        }

        /**
         * @brief Create an empty table from a compound datatype and title.
         * @param h5Type HDF5 compound datatype describing one record, including field offsets.
         * @param title Descriptive table title.
         * @param createOptions Optional chunk dimensions and compression. h5Type and title arguments take precedence.
         * @return A copy of this object, for chained calls.
         */
        BasicTable create(const hid::h5t &h5Type, std::string_view title, const TableCreateOptions &createOptions = {})
            requires traits::is_mutable_file_v<FileType>
        {
            auto options   = createOptions;
            options.h5Type = h5Type;
            options.title  = h5pp::util::safe_str(title);
            return create(options);
        }

        /**
         * @brief Append records to an existing table.
         * @param data Records whose memory layout matches the table's compound datatype.
         * @param extent Number of records to write; inferred from data when omitted. Required for pointer data.
         */
        template<typename DataType>
        requires traits::is_mutable_file_v<FileType>
        BasicTable appendRecords(const DataType &data, std::optional<hsize_t> extent = std::nullopt) {
            requireWritable_("append records");
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            auto info        = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            info.assertWriteReady();
            h5pp::hdf5::writeTableRecords(data, info, info.numRecords.value(), extent);
            return *this;
        }

        /**
         * @brief Write records at an offset, extending the table if necessary.
         * @param data Records whose memory layout matches the table's compound datatype.
         * @param offset Zero-based first record to overwrite.
         * @param extent Number of records to write; inferred from data when omitted. Required for pointer data.
         */
        template<typename DataType>
        requires traits::is_mutable_file_v<FileType>
        BasicTable writeRecords(const DataType &data, hsize_t offset = 0, std::optional<hsize_t> extent = std::nullopt) {
            requireWritable_("write records");
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            auto info        = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            h5pp::hdf5::writeTableRecords(data, info, offset, extent);
            return *this;
        }

        /**
         * @brief Append records from another table, creating this table if absent.
         *
         * A missing table is created with the source datatype and title.
         *
         * @param srcInfo Source table metadata with open dataset and datatype identifiers.
         * @param selection ALL, FIRST or LAST source records.
         * @return A copy of this object, for chained calls.
         */
        BasicTable appendRecordsFrom(const h5pp::TableInfo &srcInfo, TableSelection selection = TableSelection::ALL)
            requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("append records");
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            auto tgtInfo     = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            if(not tgtInfo.tableExists.has_value() or not tgtInfo.tableExists.value()) {
                srcInfo.assertCreateReady();
                options.h5Type = srcInfo.h5Type;
                h5pp::scan::makeTableInfo(tgtInfo, fileHandle_, options, srcInfo.tableTitle.value(), plists_);
                h5pp::hdf5::createTable(tgtInfo, plists_);
            }
            tgtInfo.assertWriteReady();
            auto [offset, extent] = util::parseTableSelection(selection, srcInfo.numRecords.value());
            h5pp::hdf5::copyTableRecords(srcInfo, offset, extent, tgtInfo, tgtInfo.numRecords.value(), plists_);
            return *this;
        }

        /**
         * @brief Append records from a table at an HDF5 location.
         * @param srcLocation Source file or group identifier.
         * @param srcTablePath Table path relative to srcLocation.
         * @param selection ALL, FIRST or LAST source records.
         * @return A copy of this object, for chained calls.
         * @see appendRecordsFrom(const h5pp::TableInfo &, TableSelection)
         */
        template<typename h5x_src>
        BasicTable appendRecordsFrom(const h5x_src   &srcLocation,
                                     std::string_view srcTablePath,
                                     TableSelection   selection = TableSelection::ALL) requires traits::is_mutable_file_v<FileType>
        {
            Options options;
            options.linkPath = h5pp::util::safe_str(srcTablePath);
            auto srcInfo     = h5pp::scan::readTableInfo(srcLocation, options, plists_);
            return appendRecordsFrom(srcInfo, selection);
        }

        /**
         * @brief Read records into a returned value.
         *
         * Without offset or extent, resizable containers read all records and single-record
         * buffers read the last record. With only offset, containers read to the end and
         * single-record buffers read one record. With only extent, read the last extent records.
         * With DataType = std::optional<T>, a missing table returns std::nullopt.
         *
         * @tparam DataType Record or container type matching the compound datatype's memory layout.
         * @param offset Zero-based first record.
         * @param extent Number of records. Required for std::vector<std::byte>.
         * @see File::vlenReclaim()
         */
        template<typename DataType>
        [[nodiscard]] DataType readRecords(std::optional<hsize_t> offset = std::nullopt,
                                           std::optional<hsize_t> extent = std::nullopt) const {
            if constexpr(h5pp::traits::is_specialization_v<DataType, std::optional>) {
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

        /**
         * @brief Read ALL, FIRST or LAST records into a returned value.
         *
         * With DataType = std::optional<T>, a missing table returns std::nullopt.
         *
         * @tparam DataType Record or container type matching the compound datatype's memory layout.
         * @param selection Records to read. Non-resizable buffers must fit the selection.
         * @see File::vlenReclaim()
         */
        template<typename DataType>
        [[nodiscard]] DataType readRecords(TableSelection selection) const {
            if constexpr(h5pp::traits::is_specialization_v<DataType, std::optional>) {
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

        /**
         * @brief Read records into a supplied buffer, resizing containers to fit.
         * @param data Destination with a memory layout matching the table's compound datatype.
         * @param offset Zero-based first record.
         * @param extent Number of records. Required for std::vector<std::byte>.
         * @see readRecords(std::optional<hsize_t>, std::optional<hsize_t>)
         * @see File::vlenReclaim()
         */
        template<typename DataType>
        void readRecordsInto(DataType              &data,
                             std::optional<hsize_t> offset = std::nullopt,
                             std::optional<hsize_t> extent = std::nullopt) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath_);
            auto info        = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            h5pp::hdf5::readTableRecords(data, info, offset, extent, plists_);
            if(info.reclaimInfo.has_value()) reclaimStack_->emplace_back(info.reclaimInfo.value());
        }

        /**
         * @brief Read ALL, FIRST or LAST records into a supplied buffer.
         * @param data Destination buffer. Resizable containers are resized; other buffers must fit the selection.
         * @param selection Records to read.
         * @see File::vlenReclaim()
         */
        template<typename DataType>
        void readRecordsInto(DataType &data, TableSelection selection) const {
            Options options;
            options.linkPath      = h5pp::util::safe_str(tablePath_);
            auto info             = h5pp::scan::readTableInfo(fileHandle_, options, plists_);
            auto [offset, extent] = util::parseTableSelection(data, selection, info.numRecords, info.recordBytes);
            h5pp::hdf5::readTableRecords(data, info, offset, extent, plists_);
            if(info.reclaimInfo.has_value()) reclaimStack_->emplace_back(info.reclaimInfo.value());
        }

        /**
         * @brief Read selected fields from a range of records.
         *
         * For one field, use its C++ type or a container of that type. For multiple fields,
         * use a record type with the selected fields packed in selection order.
         * Variable-length array fields must be read one at a time.
         * With DataType = std::optional<T>, missing fields return std::nullopt.
         *
         * @param fields Field names or zero-based indices, accepted by NamesOrIndices.
         * @param offset Zero-based first record.
         * @param extent Number of records; defaults follow readRecords().
         * @see File::vlenReclaim()
         */
        template<typename DataType, typename Fields>
        [[nodiscard]] DataType readField(const Fields          &fields,
                                         std::optional<hsize_t> offset = std::nullopt,
                                         std::optional<hsize_t> extent = std::nullopt) const {
            h5pp::NamesOrIndices fieldSelection(fields);
            if constexpr(h5pp::traits::is_specialization_v<DataType, std::optional>) {
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

        /**
         * @brief Read selected fields from ALL, FIRST or LAST records.
         * @param fields Field names or zero-based indices, accepted by NamesOrIndices.
         * @param selection Records to read.
         * @return Field values, or std::nullopt for missing fields when DataType is std::optional<T>.
         * @see readField(const Fields &, std::optional<hsize_t>, std::optional<hsize_t>)
         */
        template<typename DataType, typename Fields>
        [[nodiscard]] DataType readField(const Fields &fields, TableSelection selection) const {
            h5pp::NamesOrIndices fieldSelection(fields);
            if constexpr(h5pp::traits::is_specialization_v<DataType, std::optional>) {
                if(not fieldExists(fieldSelection)) return std::nullopt;
                return readField<typename DataType::value_type>(fieldSelection, selection);
            }
            DataType data;
            readFieldInto(data, fieldSelection, selection);
            return data;
        }

        /**
         * @brief Read selected fields into a supplied buffer.
         * @param data Destination for one field or packed fields in selection order.
         * @param fields Field names or zero-based indices, accepted by NamesOrIndices.
         * @param offset Zero-based first record.
         * @param extent Number of records; defaults follow readRecords().
         * @see File::vlenReclaim()
         */
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
            if(info.reclaimInfo.has_value()) reclaimStack_->emplace_back(info.reclaimInfo.value());
        }

        /**
         * @brief Read selected fields from ALL, FIRST or LAST records into a supplied buffer.
         * @param data Destination for one field or packed fields in selection order.
         * @param fields Field names or zero-based indices, accepted by NamesOrIndices.
         * @param selection Records to read.
         * @see File::vlenReclaim()
         */
        template<typename DataType, typename Fields>
        void readFieldInto(DataType &data, const Fields &fields, TableSelection selection) const {
            static_assert(not std::is_const_v<DataType>);
            static_assert(not traits::is_h5pp_id<DataType>);
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
            if(info.reclaimInfo.has_value()) reclaimStack_->emplace_back(info.reclaimInfo.value());
        }
    };

    using Table      = BasicTable<h5pp::v2::File>;       /*!< Table operations obtained from a non-const File. */
    using ConstTable = BasicTable<const h5pp::v2::File>; /*!< Table operations obtained from a const File. */
}
