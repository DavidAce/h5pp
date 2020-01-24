
#pragma once

#include "h5ppAttributeProperties.h"
#include "h5ppConstants.h"
#include "h5ppDatasetProperties.h"
#include "h5ppEigen.h"
#include "h5ppFileCounter.h"
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
#include "h5ppTypeSfinae.h"
#include "h5ppUtils.h"
#include <cassert>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>

namespace h5pp {

    /*!
     \brief Writes and reads data to a binary hdf5-file.
    */

    class File {
        private:
        fs::path fileName; /*!< Filename and extension, e.g. output.h5 */
        fs::path filePath; /*!< Full path to the file, eg. /home/cooldude/h5project/output.h5 */

        AccessMode accessMode = AccessMode::READWRITE;
        CreateMode createMode = CreateMode::RENAME;

        size_t   logLevel     = 2;
        bool     logTimestamp = false;
        hid::h5e error_stack;
        //        bool   defaultExtendable = false; /*!< New datasets with ndims >= can be set to extendable by default. For small datasets, setting this true results in larger
        //        file size */
        unsigned int defaultCompressionLevel = 0;

        public:
        bool hasInitialized = false;

        // The following struct contains modifiable property lists.
        // This allows us to use h5pp with MPI, for instance.
        // Unmodified, these default to serial (non-MPI) use.
        // We expose these property lists here so the user may set them as needed.
        // To enable MPI, the user can call H5Pset_fapl_mpio(hid_t file_access_plist, MPI_Comm comm, MPI_Info info)
        // The user is responsible for linking to MPI and learning how to set properties for MPI usage
        PropertyLists plists;

        File() { h5pp::logger::setLogger("h5pp", logLevel, logTimestamp); }

        File(const File &other) {
            h5pp::logger::log->debug("Copy-constructing this file [{}] from given file: [{}]. Previously initialized (this): {}. Previously initialized (other): {}",
                                     fileName.string(),
                                     other.getFileName(),
                                     hasInitialized,
                                     other.hasInitialized);
            *this = other;
        }

        explicit File(fs::path   FileName_,
                      AccessMode accessMode_   = AccessMode::READWRITE,
                      CreateMode createMode_   = CreateMode::RENAME,
                      size_t     logLevel_     = 2,
                      bool       logTimestamp_ = false)
            : fileName(std::move(FileName_)), accessMode(accessMode_), createMode(createMode_), logLevel(logLevel_), logTimestamp(logTimestamp_) {
            h5pp::logger::setLogger("h5pp", logLevel, logTimestamp);
            h5pp::logger::log->debug("Constructing h5pp file. Given path: [{}]", fileName.string());

            if(accessMode_ == AccessMode::READONLY and createMode_ == CreateMode::TRUNCATE) {
                logger::log->error("Options READONLY and TRUNCATE are incompatible.");
                return;
            }
            initialize();
        }

        explicit File(fs::path FileName_, CreateMode createMode_, size_t logLevel_ = 2, bool logTimestamp_ = false)
            : File(std::move(FileName_), AccessMode::READWRITE, createMode_, logLevel_, logTimestamp_) {}

        ~File() noexcept(false) {
            auto savedLog = h5pp::logger::log->name();
            h5pp::logger::setLogger("h5pp|exit", logLevel, logTimestamp);
            //            if(h5pp::counter::ActiveFileCounter::getCount() == 1) { h5pp::type::compound::closeTypes(); }
            h5pp::counter::ActiveFileCounter::decrementCounter(fileName.string());
            if(h5pp::counter::ActiveFileCounter::getCount() == 0) {
                h5pp::logger::log->debug("Closing file: {}.", fileName.string(), h5pp::counter::ActiveFileCounter::getCount(), h5pp::counter::ActiveFileCounter::OpenFileNames());
            } else {
                h5pp::logger::log->debug("Closing file: {}. There are still {} files open: {}",
                                         getFileName(),
                                         h5pp::counter::ActiveFileCounter::getCount(),
                                         h5pp::counter::ActiveFileCounter::OpenFileNames());
            }
            H5Eprint(H5E_DEFAULT, stderr);
            h5pp::logger::setLogger(savedLog, logLevel, logTimestamp);
        }

        File &operator=(const File &other) {
            h5pp::logger::log->debug("Assign-constructing this file [{}] from given file: [{}]. Previously initialized (this): {}. Previously initialized (other): {}",
                                     fileName.string(),
                                     other.getFileName(),
                                     hasInitialized,
                                     other.hasInitialized);
            if(&other != this) {
                if(hasInitialized) { h5pp::counter::ActiveFileCounter::decrementCounter(fileName.string()); }
                if(other.hasInitialized) {
                    logLevel = other.logLevel;
                    h5pp::logger::setLogger("h5pp", logLevel, logTimestamp);
                    accessMode = other.getAccessMode();
                    createMode = CreateMode::OPEN;
                    fileName   = other.fileName;
                    filePath   = other.filePath;
                    initialize();
                }
            }
            return *this;
        }

        [[nodiscard]] hid::h5f openFileHandle() const {
            try {
                if(hasInitialized) {
                    switch(accessMode) {
                        case(AccessMode::READONLY): {
                            h5pp::logger::log->trace("Opening file handle in READONLY mode");
                            hid::h5f fileHandle = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, plists.file_access);
                            if(fileHandle < 0) {
                                H5Eprint(H5E_DEFAULT, stderr);
                                throw std::runtime_error("Failed to open file in read-only mode: " + filePath.string());
                            } else {
                                return fileHandle;
                            }
                        }
                        case(AccessMode::READWRITE): {
                            h5pp::logger::log->trace("Opening file handle in READWRITE mode");
                            hid::h5f fileHandle = H5Fopen(filePath.string().c_str(), H5F_ACC_RDWR, plists.file_access);
                            if(fileHandle < 0) {
                                H5Eprint(H5E_DEFAULT, stderr);
                                throw std::runtime_error("Failed to open file in read-write mode: " + filePath.string());
                            } else {
                                return fileHandle;
                            }
                        }
                        default: throw std::runtime_error("Invalid access mode");
                    }
                } else {
                    throw std::runtime_error("File hasn't initialized");
                }

            } catch(std::exception &ex) { throw std::runtime_error("Could not open file handle: " + std::string(ex.what())); }
        }

        void initialize() {
            auto savedLog = h5pp::logger::log->name();
            h5pp::logger::setLogger("h5pp|init", logLevel, logTimestamp);
            /* Turn off error handling permanently */
            error_stack                          = H5Eget_current_stack();
            herr_t turnOffAutomaticErrorPrinting = H5Eset_auto2(error_stack, nullptr, nullptr);
            //            H5Eclose_stack(error_stack);
            if(turnOffAutomaticErrorPrinting < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to turn off H5E error printing");
            }

            setOutputFilePath();
            h5pp::type::compound::initTypes();
            //            fileCount++;
            h5pp::counter::ActiveFileCounter::incrementCounter(fileName.string());
            h5pp::logger::setLogger("h5pp|" + fileName.string(), logLevel, logTimestamp);
            //            h5pp::Logger::setLogger(savedLog,logLevel,false);
            hasInitialized = true;
        }

        void setCreateMode(CreateMode createMode_) { createMode = createMode_; }
        void setAccessMode(AccessMode accessMode_) { accessMode = accessMode_; }

        // Functions for querying the file
        [[nodiscard]] CreateMode  getCreateMode() const { return createMode; }
        [[nodiscard]] AccessMode  getAccessMode() const { return accessMode; }
        [[nodiscard]] std::string getFileName() const { return fileName.string(); }
        [[nodiscard]] std::string getFilePath() const { return filePath.string(); }

        void setLogLevel(size_t logLevelZeroToFive) {
            logLevel = logLevelZeroToFive;
            h5pp::logger::setLogLevel(logLevelZeroToFive);
        }

        // Functions related to datasets

        void setDefaultCompressionLevel(unsigned int compressionLevelZeroToNine) { defaultCompressionLevel = h5pp::hdf5::getValidCompressionLevel(compressionLevelZeroToNine); }
        [[nodiscard]] unsigned int getDefaultCompressionLevel() const { return defaultCompressionLevel; }

        template<typename DataType>
        void writeDataset(const DataType &data, const DatasetProperties &dsetProps);

        template<typename DataType>
        void writeDataset(const DataType &                    data,
                          std::string_view                    dsetName,
                          std::optional<H5D_layout_t>         layout           = std::nullopt,
                          std::optional<std::vector<hsize_t>> chunkDimensions  = std::nullopt,
                          std::optional<unsigned int>         compressionLevel = std::nullopt);

        template<typename PointerType, typename T, size_t N, typename... Args, typename = std::enable_if_t<std::is_pointer_v<PointerType> and std::is_integral_v<T>>>
        void writeDataset(const PointerType ptr,
                          const T (&dims)[N],
                          std::string_view                    datasetPath,
                          std::optional<H5D_layout_t>         layout           = std::nullopt,
                          std::optional<std::vector<hsize_t>> chunkDimensions  = std::nullopt,
                          std::optional<unsigned int>         compressionLevel = std::nullopt) {
            writeDataset(h5pp::PtrWrapper(ptr, dims), datasetPath, layout, chunkDimensions, compressionLevel);
        }

        template<typename PointerType, typename = std::enable_if_t<std::is_pointer_v<PointerType>>>
        void writeDataset(const PointerType                   ptr,
                          const size_t                        size,
                          std::string_view                    datasetPath,
                          std::optional<H5D_layout_t>         layout           = std::nullopt,
                          std::optional<std::vector<hsize_t>> chunkDimensions  = std::nullopt,
                          std::optional<unsigned int>         compressionLevel = std::nullopt) {
            writeDataset(h5pp::PtrWrapper(ptr, size), datasetPath, layout, chunkDimensions, compressionLevel);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readDataset(DataType &data, std::string_view dsetName) const {
            hid::h5f file      = openFileHandle();
            auto     dsetProps = h5pp::scan::getDatasetProperties_read(file, dsetName, std::nullopt, plists);
            h5pp::hdf5::readDataset(data, dsetProps);
        }

        template<typename PointerType, typename T, size_t N, typename = std::enable_if_t<std::is_array_v<PointerType> or std::is_pointer_v<PointerType>>>
        void readDataset(PointerType ptr, const T (&dims)[N], std::string_view datasetPath) {
            auto wrapper = h5pp::PtrWrapper(ptr, dims);
            readDataset(wrapper, datasetPath);
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

        // Functions related to attributes
        template<typename DataType>
        void writeAttribute(const DataType &attribute, const AttributeProperties &aprops);

        template<typename DataType>
        void writeAttribute(const DataType &data, std::string_view attrName, std::string_view linkName);

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

        // Functions for querying

        [[nodiscard]] int getDatasetRank(std::string_view datasetPath) {
            hid::h5f file    = openFileHandle();
            hid::h5d dataset = h5pp::hdf5::openLink<hid::h5d>(file, datasetPath);
            return h5pp::hdf5::getRank(dataset);
        }

        [[nodiscard]] std::vector<hsize_t> getDatasetDimensions(std::string_view datasetPath) {
            hid::h5f file    = openFileHandle();
            hid::h5d dataset = h5pp::hdf5::openLink<hid::h5d>(file, datasetPath);
            return h5pp::hdf5::getDimensions(dataset);
        }

        [[nodiscard]] bool linkExists(std::string_view link) const {
            hid::h5f file   = openFileHandle();
            bool     exists = h5pp::hdf5::checkIfLinkExists(file, link, std::nullopt, plists.link_access);
            return exists;
        }

        [[nodiscard]] std::vector<std::string> getContentsOfGroup(std::string_view groupName) const {
            hid::h5f file       = openFileHandle();
            auto     foundLinks = h5pp::hdf5::getContentsOfGroup(file, groupName);
            return foundLinks;
        }

        [[nodiscard]] TypeInfo getDatasetTypeInfo(std::string_view dsetName) const {
            hid::h5f file = openFileHandle();
            return h5pp::hdf5::getDatasetTypeInfo(file, dsetName, std::nullopt, plists.link_access);
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

        void createGroup(std::string_view group_relative_name) {
            hid::h5f file = openFileHandle();
            h5pp::hdf5::createGroup(file, group_relative_name, std::nullopt, plists);
        }

        void writeSymbolicLink(std::string_view src_path, std::string_view tgt_path) {
            hid::h5f file = openFileHandle();
            h5pp::hdf5::writeSymbolicLink(file, src_path, tgt_path, plists);
        }
        void createDataset(DatasetProperties &dsetProps) {
            if(not dsetProps.dsetExists) {
                h5pp::logger::log->trace("Creating dataset: [{}]", dsetProps.dsetName.value());
                hid::h5f file = openFileHandle();
                h5pp::hdf5::createDataset(file, dsetProps, plists);
            }
        }

        private:
        void setOutputFilePath() {
            h5pp::logger::log->trace("Attempting to set file name and path. File name [{}] path [{}]. Has initialized: {}", fileName.string(), filePath.string(), hasInitialized);

            // There are different possibilities:
            // 1) File is being initialized from another h5pp File (e.g. by copy or assignment) In that case the following applies:
            //      a) FileName =  just a filename such as myfile.h5 without parent path.
            //      b) FilePath =  an absolute path to the file such as /home/yada/yada/myFile.h5
            // 2) File did not exist previously
            //      a) FileName = a filename possibly with parent path or not, such as ../myDir/myFile.h5 or just myFile
            //      b) FilePath = empty

            // Take case 2 first and make it into a case 1
            if(filePath.empty()) {
                h5pp::logger::log->trace("File path empty. Detecting path...");
                filePath = fs::absolute(fileName);
                fileName = filePath.filename();
            }

            // Now we expect case 1 to hold.
            h5pp::logger::log->trace("Current path        : {}", fs::current_path().string());
            h5pp::logger::log->debug("Detected file name  : {}", fileName.string());
            h5pp::logger::log->debug("Detected file path  : {}", filePath.string());
            // The following function can modify the resulting filePath and fileName depending on accessmode/createmode.
            std::tie(filePath, fileName) = h5pp::hdf5::createFile(filePath, accessMode, createMode, plists);
        }
    };
}

template<typename DataType>
void h5pp::File::writeDataset(const DataType &data, const DatasetProperties &dsetProps) {
    hid::h5f file = openFileHandle();
    h5pp::hdf5::writeDataset(data, dsetProps, plists);
}

template<typename DataType>
void h5pp::File::writeDataset(const DataType &                    data,
                              std::string_view                    dsetName,
                              std::optional<H5D_layout_t>         layout,
                              std::optional<std::vector<hsize_t>> chunkDimensions,
                              std::optional<unsigned int>         compressionLevel) {
    if(accessMode == AccessMode::READONLY) { throw std::runtime_error("Attempted to write to read-only file"); }
    hid::h5f file = openFileHandle();

    if(compressionLevel)
        compressionLevel = h5pp::hdf5::getValidCompressionLevel(compressionLevel);
    else
        compressionLevel = getDefaultCompressionLevel();

    auto dsetProps = h5pp::scan::getDatasetProperties_write(file, dsetName, data, std::nullopt, layout, chunkDimensions, compressionLevel);
    // Create the dataset id and set its properties
    h5pp::hdf5::createDataset(file, dsetProps);
    h5pp::hdf5::setDatasetExtent(dsetProps);
    dsetProps.fileSpace = H5Dget_space(dsetProps.dataSet);
    if(dsetProps.layout.value() == H5D_CHUNKED) h5pp::hdf5::selectHyperslab(dsetProps.fileSpace, dsetProps.memSpace);

    h5pp::hdf5::writeDataset(data, dsetProps, plists);
}

template<typename DataType>
void h5pp::File::writeAttribute(const DataType &data, const AttributeProperties &attrProps) {
    hid::h5f file = openFileHandle();
    h5pp::hdf5::writeAttribute(data, attrProps);
}

template<typename DataType>
void h5pp::File::writeAttribute(const DataType &data, std::string_view attrName, std::string_view linkName) {
    hid::h5f file      = openFileHandle();
    auto     attrProps = h5pp::scan::getAttributeProperties_write(file, data, attrName, linkName, std::nullopt, std::nullopt, plists);
    h5pp::hdf5::createAttribute(attrProps);

#ifdef H5PP_EIGEN3
    if constexpr(h5pp::type::sfinae::is_eigen_any<DataType>::value and not h5pp::type::sfinae::is_eigen_1d<DataType>::value) {
        h5pp::logger::log->debug("Converting data to row-major storage order");
        const auto tempRowm = eigen::to_RowMajor(data); // Convert to Row Major first;
        h5pp::hdf5::writeAttribute(tempRowm, attrProps);
        return;
    }
#endif

    h5pp::hdf5::writeAttribute(data, attrProps);
}
