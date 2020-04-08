
#pragma once

#include "h5ppAttributeProperties.h"
#include "h5ppConstants.h"
#include "h5ppEigen.h"
#include "h5ppEnums.h"
#include "h5ppFilesystem.h"
#include "h5ppHdf5.h"
#include "h5ppHid.h"
#include "h5ppLogger.h"
#include "h5ppOptional.h"
#include "h5ppPermissions.h"
#include "h5ppPropertyLists.h"
#include "h5ppPtrWrapper.h"
#include "h5ppScan.h"
#include "h5ppTypeCompoundCreate.h"
#include "h5ppUtils.h"
#include <cassert>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <iomanip>
#include <iostream>
#include <string>

namespace h5pp {

    /*!
     \brief Writes and reads data to a binary hdf5-file.
    */

    class File {
        private:
        fs::path             filePath; /*!< Full path to the file, eg. /home/cooldude/h5project/output.h5 */
        h5pp::FilePermission permission   = h5pp::FilePermission::RENAME;
        size_t               logLevel     = 2;
        bool                 logTimestamp = false;
        hid::h5e             error_stack;
        unsigned int         currentCompressionLevel = 0;

        void init() {
            h5pp::logger::setLogger("h5pp|init", logLevel, logTimestamp);
            h5pp::logger::log->debug("Initializing HDF5 file: [{}]", filePath.string());
            /* Set default error print output */
            error_stack                          = H5Eget_current_stack();
            herr_t turnOffAutomaticErrorPrinting = H5Eset_auto2(error_stack, nullptr, nullptr);
            if(turnOffAutomaticErrorPrinting < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to turn off H5E error printing");
            }
            // The following function can modify the resulting filePath depending on permission.
            filePath = h5pp::hdf5::createFile(filePath, permission, plists);
            h5pp::type::compound::initTypes();
            h5pp::logger::setLogger("h5pp|" + filePath.filename().string(), logLevel, logTimestamp);
            h5pp::logger::log->debug("Successfully initialized file [{}]", filePath.string());
        }

        public:
        // The following struct contains modifiable property lists.
        // This allows us to use h5pp with MPI, for instance.
        // Unmodified, these default to serial (non-MPI) use.
        // We expose these property lists here so the user may set them as needed.
        // To enable MPI, the user can call H5Pset_fapl_mpio(hid_t file_access_plist, MPI_Comm comm, MPI_Info info)
        // The user is responsible for linking to MPI and learning how to set properties for MPI usage
        PropertyLists plists;

        File() {
            h5pp::logger::setLogger("h5pp", logLevel, logTimestamp);
            h5pp::logger::log->debug("Default-constructing null file");
        }

        File(const File &other) {
            h5pp::logger::log->debug("Copy-constructing this file [{}] from given file: [{}]", filePath.string(), other.getFilePath());
            *this = other;
        }

        explicit File(const std::string &filePath_, h5pp::FilePermission permission_ = h5pp::FilePermission::RENAME, size_t logLevel_ = 2, bool logTimestamp_ = false)
            : filePath(filePath_), permission(permission_), logLevel(logLevel_), logTimestamp(logTimestamp_) {
            init();
        }

        explicit File(const std::string &filePath_, unsigned int H5F_ACC_FLAGS, size_t logLevel_ = 2, bool logTimestamp_ = false)
            : filePath(filePath_), logLevel(logLevel_), logTimestamp(logTimestamp_) {
            permission = h5pp::hdf5::convertFileAccessFlags(H5F_ACC_FLAGS);
            init();
        }

        ~File() {
            h5pp::logger::log->debug("Closing file [{}]", filePath.string());
            H5Eprint(H5E_DEFAULT, stderr);
        }

        File &operator=(const File &other) {
            h5pp::logger::log->debug("Assignment to this file [{}] from given file: [{}]", filePath.string(), other.getFilePath());
            if(&other != this) {
                logLevel     = other.logLevel;
                logTimestamp = other.logTimestamp;
                permission   = other.permission;
                filePath     = other.filePath;
                plists       = other.plists;
                h5pp::logger::setLogger("h5pp|" + filePath.filename().string(), logLevel, logTimestamp);
            }
            return *this;
        }

        [[nodiscard]] hid::h5f openFileHandle() const {
            h5pp::logger::setLogger("h5pp|" + filePath.filename().string(), logLevel, logTimestamp);
            if(permission == h5pp::FilePermission::READONLY) {
                h5pp::logger::log->trace("Opening file in READONLY mode");
                hid::h5f fileHandle = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, plists.file_access);
                if(fileHandle < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to open file in read-only mode: " + filePath.string());
                } else
                    return fileHandle;
            } else {
                h5pp::logger::log->trace("Opening file in READWRITE mode");
                hid::h5f fileHandle = H5Fopen(filePath.string().c_str(), H5F_ACC_RDWR, plists.file_access);
                if(fileHandle < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to open file in read-write mode: " + filePath.string());
                } else
                    return fileHandle;
            }
        }

        /*
         *
         * Functions for file properties
         *
         */
        [[nodiscard]] h5pp::FilePermission getFilePermission() const { return permission; }
        [[nodiscard]] std::string          getFileName() const { return filePath.filename().string(); }
        [[nodiscard]] std::string          getFilePath() const { return filePath.string(); }
        void                               setFilePermission(h5pp::FilePermission permission_) { permission = permission_; }

        /*
         *
         * Functions for logging
         *
         */
        [[nodiscard]] size_t getLogLevel() const { return logLevel; }
        void                 setLogLevel(size_t logLevelZeroToFive) {
            logLevel = logLevelZeroToFive;
            h5pp::logger::setLogLevel(logLevelZeroToFive);
        }

        /*
         *
         * Functions related to datasets
         *
         */

        void setCompressionLevel(unsigned int compressionLevelZeroToNine) { currentCompressionLevel = h5pp::hdf5::getValidCompressionLevel(compressionLevelZeroToNine); }
        [[nodiscard]] unsigned int getCompressionLevel() const { return currentCompressionLevel; }
        [[nodiscard]] unsigned int getCompressionLevel(std::optional<size_t> desiredCompressionLevel) const {
            if(desiredCompressionLevel)
                return h5pp::hdf5::getValidCompressionLevel(desiredCompressionLevel.value());
            else
                return currentCompressionLevel;
        }

        void createGroup(std::string_view group_relative_name) {
            hid::h5f file = openFileHandle();
            h5pp::hdf5::createGroup(file, group_relative_name, std::nullopt, plists);
        }

//        void createDataset(DsetProperties &dsetProps) {
//            if(not dsetProps.dsetExists) {
//                h5pp::logger::log->trace("Creating dataset: [{}]", dsetProps.dsetPath.value());
//                hid::h5f file = openFileHandle();
//                h5pp::hdf5::createDataset(file, dsetProps, plists);
//            }
//        }

        void createDataset(const MetaDset &metaDset) { h5pp::hdf5::createDataset(metaDset, plists); }

        template<typename DataType>
        void createDataset(const DataType &data, std::string_view dsetPath, const DsetOptions &options) {
            if(permission == h5pp::FilePermission::READONLY) throw std::runtime_error(h5pp::format("Attempted to create dataset on read-only file [{}]", filePath.string()));
            hid::h5f file     = openFileHandle();
            auto     metaDset = h5pp::scan::newMetaDset(file, data, dsetPath, options, plists);
            h5pp::hdf5::createDataset(metaDset, plists);
        }

        template<typename DataType,
                 typename DataDimsType  = std::initializer_list<hsize_t>,
                 typename ChunkDimsType = std::initializer_list<hsize_t>,
                 typename               = std::enable_if_t<h5pp::type::sfinae::is_integral_list_v<DataDimsType>>,
                 typename               = std::enable_if_t<h5pp::type::sfinae::is_integral_list_v<ChunkDimsType>>>
        void createDataset(const DataType &            data,
                           const DataDimsType &        dataDims,
                           std::string_view            dsetPath,
                           const hid::h5t &            h5_type,
                           std::optional<H5D_layout_t> h5_layout   = std::nullopt,
                           const ChunkDimsType &       chunkDims   = {},
                           std::optional<unsigned int> compression = std::nullopt) {
            if(permission == h5pp::FilePermission::READONLY) throw std::runtime_error(h5pp::format("Attempted to create dataset on read-only file [{}]", filePath.string()));
            DsetOptions options;
            options.h5_type     = h5_type;
            options.h5_layout   = h5_layout;
            options.dataDims    = h5pp::utils::getOptionalIterable(dataDims);
            options.chunkDims   = h5pp::utils::getOptionalIterable(chunkDims);
            options.compression = getCompressionLevel(compression);
            createDataset(data, dsetPath, options);
        }

        template<typename DataType, typename ChunkDimsType = std::initializer_list<hsize_t>, typename = std::enable_if_t<h5pp::type::sfinae::is_integral_list_v<ChunkDimsType>>>
        void createDataset(const DataType &            data,
                           std::string_view            dsetPath,
                           const hid::h5t &            h5_type,
                           std::optional<H5D_layout_t> h5_layout   = std::nullopt,
                           const ChunkDimsType &       chunkDims   = {},
                           std::optional<unsigned int> compression = std::nullopt) {
            DsetOptions options;
            options.h5_type     = h5_type;
            options.h5_layout   = h5_layout;
            options.chunkDims   = h5pp::utils::getOptionalIterable(chunkDims);
            options.compression = getCompressionLevel(compression);
            createDataset(data, dsetPath, options);
        }

        template<typename DataType, typename ChunkDimsType = std::initializer_list<hsize_t>, typename = std::enable_if_t<h5pp::type::sfinae::is_integral_list_v<ChunkDimsType>>>
        void createDataset(const DataType &            data,
                           std::string_view            dsetPath,
                           std::optional<H5D_layout_t> h5_layout   = std::nullopt,
                           const ChunkDimsType &       chunkDims   = {},
                           std::optional<unsigned int> compression = std::nullopt) {
            if(permission == h5pp::FilePermission::READONLY) throw std::runtime_error(h5pp::format("Attempted to create dataset on read-only file [{}]", filePath.string()));
            DsetOptions options;
            options.h5_layout   = h5_layout;
            options.chunkDims   = h5pp::utils::getOptionalIterable(chunkDims);
            options.compression = getCompressionLevel(compression);
            createDataset(data, dsetPath, options);
        }

        template<typename DataType>
        void writeDataset(const DataType &data, std::string_view dsetPath, const DsetOptions &options) {
            if(permission == h5pp::FilePermission::READONLY) throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            createDataset(data, dsetPath, options);
            hid::h5f file      = openFileHandle();
            auto     metaData  = h5pp::scan::getMetaData(data, options.dataDims);
            auto     metaDset  = h5pp::scan::getMetaDset(file, dsetPath, plists);
            h5pp::hdf5::writeDataset(data, metaData,metaDset, plists);
        }

        template<typename DataType,
                 typename DataDimsType  = std::initializer_list<hsize_t>,
                 typename ChunkDimsType = std::initializer_list<hsize_t>,
                 typename               = std::enable_if_t<h5pp::type::sfinae::is_integral_list_v<DataDimsType>>,
                 typename               = std::enable_if_t<h5pp::type::sfinae::is_integral_list_v<ChunkDimsType>>>
        void writeCompound(const DataType &            data,
                           const DataDimsType &        dataDims,
                           std::string_view            dsetPath,
                           const hid::h5t &            h5_type,
                           std::optional<H5D_layout_t> h5_layout   = std::nullopt,
                           const ChunkDimsType &       chunkDims   = {},
                           std::optional<unsigned int> compression = std::nullopt) {
            DsetOptions options;
            options.dataDims    = h5pp::utils::getOptionalIterable(dataDims);
            options.h5_type     = h5_type;
            options.h5_layout   = h5_layout;
            options.chunkDims   = h5pp::utils::getOptionalIterable(chunkDims);
            options.compression = getCompressionLevel(compression);
            writeDataset(data, dsetPath, options);
        }

        template<typename DataType,
                 typename DataDimsType  = std::initializer_list<hsize_t>,
                 typename ChunkDimsType = std::initializer_list<hsize_t>,
                 typename               = std::enable_if_t<h5pp::type::sfinae::is_integral_list_v<DataDimsType>>,
                 typename               = std::enable_if_t<h5pp::type::sfinae::is_integral_list_v<ChunkDimsType>>>
        void writeDataset(const DataType &            data,
                          const DataDimsType &        dataDims,
                          std::string_view            dsetPath,
                          std::optional<H5D_layout_t> h5_layout   = std::nullopt,
                          const ChunkDimsType &       chunkDims   = {},
                          std::optional<unsigned int> compression = std::nullopt) {
            DsetOptions options;
            options.dataDims    = h5pp::utils::getOptionalIterable(dataDims);
            options.h5_layout   = h5_layout;
            options.chunkDims   = h5pp::utils::getOptionalIterable(chunkDims);
            options.compression = getCompressionLevel(compression);
            writeDataset(data, dsetPath, options);
        }

        template<typename PointerType,
                 typename DataDimsType  = std::initializer_list<hsize_t>,
                 typename ChunkDimsType = std::initializer_list<hsize_t>,
                 typename               = std::enable_if_t<h5pp::type::sfinae::is_integral_list_v<DataDimsType>>,
                 typename               = std::enable_if_t<h5pp::type::sfinae::is_integral_list_v<ChunkDimsType>>,
                 typename               = std::enable_if_t<std::is_pointer_v<PointerType>>>
        void writeCompound(const PointerType           ptr,
                           const DataDimsType &        dataDims,
                           std::string_view            dsetPath,
                           const hid::h5t &            h5_type,
                           std::optional<H5D_layout_t> h5_layout   = std::nullopt,
                           const ChunkDimsType &       chunkDims   = {},
                           std::optional<unsigned int> compression = std::nullopt) {
            DsetOptions options;
            options.dataDims    = h5pp::utils::getOptionalIterable(dataDims);
            options.h5_type     = h5_type;
            options.h5_layout   = h5_layout;
            options.chunkDims   = h5pp::utils::getOptionalIterable(chunkDims);
            options.compression = getCompressionLevel(compression);
            writeDataset(h5pp::PtrWrapper(ptr, dataDims), dsetPath, options);
        }

        template<typename DataType, typename ChunkDimsType = std::initializer_list<hsize_t>, typename = std::enable_if_t<h5pp::type::sfinae::is_integral_list_v<ChunkDimsType>>>
        void writeDataset(const DataType &            data,
                           std::string_view            dsetPath,
                           std::optional<hid::h5t>     h5_type,
                           std::optional<H5D_layout_t> h5_layout   = std::nullopt,
                           const ChunkDimsType &       chunkDims   = {},
                           std::optional<unsigned int> compression = std::nullopt) {
            DsetOptions options;
            options.h5_type     = h5_type;
            options.h5_layout   = h5_layout;
            options.chunkDims   = h5pp::utils::getOptionalIterable(chunkDims);
            options.compression = getCompressionLevel(compression);
            writeDataset(data, dsetPath, options);
        }

        template<typename DataType, typename ChunkDimsType = std::initializer_list<hsize_t>, typename = h5pp::type::sfinae::is_iterable_or_nullopt<ChunkDimsType>>
        void writeDataset(const DataType &            data,
                          std::string_view            dsetPath,
                          std::optional<H5D_layout_t> h5_layout   = std::nullopt,
                          const ChunkDimsType &       chunkDims   = {},
                          std::optional<unsigned int> compression = std::nullopt) {
            DsetOptions options;
            options.h5_layout   = h5_layout;
            options.chunkDims   = h5pp::utils::getOptionalIterable(chunkDims);
            options.compression = getCompressionLevel(compression);
            writeDataset(data, dsetPath, options);
        }

        template<typename PointerType,
                 typename DataDimsType  = std::initializer_list<hsize_t>,
                 typename ChunkDimsType = std::initializer_list<hsize_t>,
                 typename               = h5pp::type::sfinae::is_iterable_or_nullopt<DataDimsType>,
                 typename               = h5pp::type::sfinae::is_iterable_or_nullopt<ChunkDimsType>,
                 typename               = std::enable_if_t<std::is_pointer_v<PointerType>>>
        void writeDataset(const PointerType           ptr,
                          const DataDimsType &        dataDims,
                          std::string_view            dsetPath,
                          std::optional<H5D_layout_t> h5_layout   = std::nullopt,
                          const ChunkDimsType &       chunkDims   = {},
                          std::optional<unsigned int> compression = std::nullopt) {
            DsetOptions options;
            options.h5_layout   = h5_layout;
            options.dataDims    = h5pp::utils::getOptionalIterable(dataDims);
            options.chunkDims   = h5pp::utils::getOptionalIterable(chunkDims);
            options.compression = getCompressionLevel(compression);
            writeDataset(h5pp::PtrWrapper(ptr, dataDims), dsetPath, options);
        }

        template<typename PointerType,
                 typename ChunkDimsType = std::initializer_list<hsize_t>,
                 typename               = h5pp::type::sfinae::is_iterable_or_nullopt<ChunkDimsType>,
                 typename               = std::enable_if_t<std::is_pointer_v<PointerType>>>
        void writeDataset(const PointerType           ptr,
                          const hsize_t &             dataDim,
                          std::string_view            dsetPath,
                          std::optional<H5D_layout_t> h5_layout        = std::nullopt,
                          std::optional<hsize_t>      chunkDim         = std::nullopt,
                          std::optional<unsigned int> compression = std::nullopt) {
            DsetOptions options;
            options.h5_layout   = h5_layout;
            options.dataDims    = h5pp::utils::getOptionalIterable(dataDim);
            options.chunkDims   = h5pp::utils::getOptionalIterable(chunkDim);
            options.compression = getCompressionLevel(compression);
            writeDataset(h5pp::PtrWrapper(ptr, dataDim), dsetPath, options);
        }


        void writeSymbolicLink(std::string_view src_path, std::string_view tgt_path) {
            hid::h5f file = openFileHandle();
            h5pp::hdf5::writeSymbolicLink(file, src_path, tgt_path, plists);
        }

//        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
//        void readDataset(DataType &data, std::string_view dsetPath) const {
//            hid::h5f file      = openFileHandle();
//            auto     dsetProps = h5pp::scan::getDatasetProperties_read(file, dsetPath, std::nullopt, plists);
//            h5pp::hdf5::readDataset(data, dsetProps);
//        }
//

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readDataset(DataType &data, std::string_view dsetPath) const {
            hid::h5f file      = openFileHandle();
            auto     metaDset  = h5pp::scan::getMetaDset(file, dsetPath, plists);
            auto     metaData  = h5pp::scan::getMetaData(data,metaDset);
            h5pp::hdf5::readDataset(data, metaData,metaDset, plists);
        }

        template<typename PointerType, typename T, size_t N, typename = std::enable_if_t<std::is_array_v<PointerType> or std::is_pointer_v<PointerType>>>
        void readDataset(PointerType ptr, const T (&dims)[N], std::string_view datasetPath) {
            auto wrapper = h5pp::PtrWrapper(ptr, dims);
            readDataset( wrapper, datasetPath);
        }

        template<typename PointerType, typename = std::enable_if_t<std::is_array_v<PointerType> or std::is_pointer_v<PointerType>>>
        void readDataset(PointerType ptr, size_t size, std::string_view datasetPath) {
            auto wrapper = h5pp::PtrWrapper(ptr, size);
            readDataset(wrapper, datasetPath);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        DataType readDataset(std::string_view datasetPath) const {
            DataType data;
            readDataset(data, datasetPath);
            return data;
        }

        /*
         *
         * Functions related to attributes
         *
         */
        template<typename DataType>
        void writeAttribute(const DataType &data, const AttributeProperties &attrProps) {
            if(permission == h5pp::FilePermission::READONLY) throw std::runtime_error("Attempted to write to read-only file [" + filePath.filename().string() + "]");
            hid::h5f file = openFileHandle();
            h5pp::hdf5::writeAttribute(data, attrProps);
        }

        template<typename DataType>
        void writeAttribute(const DataType &data, std::string_view attrName, std::string_view linkName) {
            if(permission == h5pp::FilePermission::READONLY) throw std::runtime_error("Attempted to write to read-only file [" + filePath.filename().string() + "]");
            hid::h5f file      = openFileHandle();
            auto     attrProps = h5pp::scan::getAttributeProperties_write(file, data, attrName, linkName, std::nullopt, std::nullopt, std::nullopt, plists);
            h5pp::hdf5::createAttribute(attrProps);
            h5pp::hdf5::writeAttribute(data, attrProps);
        }

        template<typename DataType>
        void writeAttribute(const DataType &data, const hid::h5t &customH5Type, std::string_view attrName, std::string_view linkName) {
            if(permission == h5pp::FilePermission::READONLY) throw std::runtime_error("Attempted to write to read-only file [" + filePath.filename().string() + "]");
            hid::h5f file      = openFileHandle();
            auto     attrProps = h5pp::scan::getAttributeProperties_write(file, data, attrName, linkName, std::nullopt, std::nullopt, customH5Type, plists);
            h5pp::hdf5::createAttribute(attrProps);
            h5pp::hdf5::writeAttribute(data, attrProps);
        }

        [[nodiscard]] inline std::vector<std::string> getAttributeNames(std::string_view linkPath) const {
            hid::h5f                 file           = openFileHandle();
            std::vector<std::string> attributeNames = h5pp::hdf5::getAttributeNames(file, linkPath, std::nullopt, plists.link_access);
            return attributeNames;
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readAttribute(DataType &data, std::string_view attrName, std::string_view linkName) const {
            hid::h5f file      = openFileHandle();
            auto     attrProps = h5pp::scan::getAttributeProperties_read(file, attrName, linkName, std::nullopt, std::nullopt, plists);
            h5pp::hdf5::readAttribute(data, attrProps);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        [[nodiscard]] DataType readAttribute(std::string_view attrName, std::string_view linkName) const {
            DataType data;
            readAttribute(data, attrName, linkName);
            return data;
        }

        /*
         *
         *
         * Functions related to tables
         *
         *
         */

        void createTable(const hid::h5t &                  entryH5Type,
                         std::string_view                  tableName,
                         std::string_view                  tableTitle,
                         const std::optional<hsize_t>      desiredChunkSize        = std::nullopt,
                         const std::optional<unsigned int> desiredCompressionLevel = std::nullopt

        ) {
            if(permission == h5pp::FilePermission::READONLY) throw std::runtime_error("Attempted to write to read-only file [" + filePath.filename().string() + "]");
            hid::h5f file       = openFileHandle();
            auto     tableProps = h5pp::scan::getTableProperties_bootstrap(entryH5Type, tableName, tableTitle, desiredChunkSize, desiredCompressionLevel);
            h5pp::hdf5::createTable(file, tableProps, plists);
        }

        template<typename DataType>
        void appendTableEntries(const DataType &data, std::string_view tableName) {
            if(permission == h5pp::FilePermission::READONLY) throw std::runtime_error("Attempted to write to read-only file [" + filePath.filename().string() + "]");
            hid::h5f file       = openFileHandle();
            auto     tableProps = h5pp::scan::getTableProperties_write(file, data, tableName, plists);
            h5pp::hdf5::appendTableEntries(file, data, tableProps);
        }

        template<typename DataType>
        void readTableEntries(DataType &data, std::string_view tableName, std::optional<size_t> startEntry = std::nullopt, std::optional<size_t> numEntries = std::nullopt) const {
            hid::h5f file       = openFileHandle();
            auto     tableProps = h5pp::scan::getTableProperties_read(file, tableName, plists);
            h5pp::hdf5::readTableEntries(file, data, tableProps, startEntry, numEntries);
        }

        template<typename DataType>
        DataType readTableEntries(std::string_view tableName, std::optional<size_t> startEntry = std::nullopt, std::optional<size_t> numEntries = std::nullopt) const {
            DataType data;
            readTableEntries(data, tableName, startEntry, numEntries);
            return data;
        }

        /*
         *
         *
         * Functions for querying
         *
         *
         */

        [[nodiscard]] int getDatasetRank(std::string_view datasetPath) const {
            hid::h5f file    = openFileHandle();
            hid::h5d dataset = h5pp::hdf5::openLink<hid::h5d>(file, datasetPath);
            return h5pp::hdf5::getRank(dataset);
        }

        [[nodiscard]] std::vector<hsize_t> getDatasetDimensions(std::string_view datasetPath) const {
            hid::h5f file    = openFileHandle();
            hid::h5d dataset = h5pp::hdf5::openLink<hid::h5d>(file, datasetPath);
            return h5pp::hdf5::getDimensions(dataset);
        }

        [[nodiscard]] bool linkExists(std::string_view link) const { return h5pp::hdf5::checkIfLinkExists(openFileHandle(), link, std::nullopt, plists.link_access); }

        [[nodiscard]] std::vector<std::string> findLinks(std::string_view searchKey = "", std::string_view searchRoot = "/", long maxSearchHits = -1) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_UNKNOWN>(openFileHandle(), searchKey, searchRoot, maxSearchHits, plists.link_access);
        }

        [[nodiscard]] std::vector<std::string> findDatasets(std::string_view searchKey = "", std::string_view searchRoot = "/", long maxSearchHits = -1) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_DATASET>(openFileHandle(), searchKey, searchRoot, maxSearchHits, plists.link_access);
        }

        [[nodiscard]] std::vector<std::string> findGroups(std::string_view searchKey = "", std::string_view searchRoot = "/", long maxSearchHits = -1) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_GROUP>(openFileHandle(), searchKey, searchRoot, maxSearchHits, plists.link_access);
        }

        [[nodiscard]] TypeInfo getDatasetTypeInfo(std::string_view dsetPath) const {
            return h5pp::hdf5::getDatasetTypeInfo(openFileHandle(), dsetPath, std::nullopt, plists.link_access);
        }

        [[nodiscard]] TypeInfo getAttributeTypeInfo(std::string_view linkName, std::string_view attrName) const {
            hid::h5f file = openFileHandle();
            return h5pp::hdf5::getAttributeTypeInfo(file, linkName, attrName, std::nullopt, std::nullopt, plists.link_access);
        }

        [[nodiscard]] std::vector<TypeInfo> getAttributeTypeInfoAll(std::string_view linkName) const {
            hid::h5f file = openFileHandle();
            return h5pp::hdf5::getAttributeTypeInfoAll(file, linkName, std::nullopt, plists.link_access);
        }

        [[nodiscard]] bool fileIsValid() const { return h5pp::hdf5::fileIsValid(filePath); }
    };
}
