
#pragma once

#include "h5ppAttributeProperties.h"
#include "h5ppConstants.h"
#include "h5ppDatasetProperties.h"
#include "h5ppEigen.h"
#include "h5ppFileCounter.h"
#include "h5ppHdf5.h"
#include "h5ppHid.h"
#include "h5ppLogger.h"
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

#if __has_include(<ghc/filesystem.hpp>)
    // Last resort
    #include <ghc/filesystem.hpp>
namespace h5pp {
    namespace fs = ghc::filesystem;
}
#elif __has_include(<filesystem>)
    #include <filesystem>
    #include <utility>
namespace h5pp {
    namespace fs = std::filesystem;
}
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
namespace h5pp {
    namespace fs = std::experimental::filesystem;
}
#else
    #error Could not find includes: <filesystem> or <experimental/filesystem> <ghc/filesystem>
#endif

// Include optional or experimental/optional
#if __has_include(<optional>)
    #include <optional>
#elif __has_include(<experimental/optional>)
    #include <experimental/optional>
namespace h5pp {
    constexpr const std::experimental::nullopt_t &nullopt = std::experimental::nullopt;
    template<typename T>
    using optional = std::experimental::optional<T>;
}
#else
    #error Could not find <optional> or <experimental/optional>
#endif

namespace h5pp {

    namespace tc = h5pp::type::sfinae;
    /*!
     \brief Writes and reads data to a binary hdf5-file.
    */
    enum class CreateMode { OPEN, TRUNCATE, RENAME };
    enum class AccessMode { READONLY, READWRITE };

    class File {
        private:
        fs::path FileName; /*!< Filename and extension, e.g. output.h5 */
        fs::path FilePath; /*!< Full path to the file, eg. /home/cooldude/h5project/output.h5 */

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
                                     FileName.string(),
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
            : FileName(std::move(FileName_)), accessMode(accessMode_), createMode(createMode_), logLevel(logLevel_), logTimestamp(logTimestamp_) {
            h5pp::logger::setLogger("h5pp", logLevel, logTimestamp);
            h5pp::logger::log->debug("Constructing h5pp file. Given path: [{}]", FileName.string());

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
            if(h5pp::counter::ActiveFileCounter::getCount() == 1) { h5pp::type::compound::closeTypes(); }
            h5pp::counter::ActiveFileCounter::decrementCounter(FileName.string());
            if(h5pp::counter::ActiveFileCounter::getCount() == 0) {
                h5pp::logger::log->debug("Closing file: {}.", FileName.string(), h5pp::counter::ActiveFileCounter::getCount(), h5pp::counter::ActiveFileCounter::OpenFileNames());
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
                                     FileName.string(),
                                     other.getFileName(),
                                     hasInitialized,
                                     other.hasInitialized);
            if(&other != this) {
                if(hasInitialized) { h5pp::counter::ActiveFileCounter::decrementCounter(FileName.string()); }
                if(other.hasInitialized) {
                    logLevel = other.logLevel;
                    h5pp::logger::setLogger("h5pp", logLevel, logTimestamp);
                    accessMode = other.getAccessMode();
                    createMode = CreateMode::OPEN;
                    FileName   = other.FileName;
                    FilePath   = other.FilePath;
                    initialize();
                }
            }
            return *this;
        }

        hid::h5f openFileHandle() const {
            try {
                if(hasInitialized) {
                    switch(accessMode) {
                        case(AccessMode::READONLY): {
                            h5pp::logger::log->trace("Opening file handle in READONLY mode");
                            hid::h5f fileHandle = H5Fopen(FilePath.string().c_str(), H5F_ACC_RDONLY, plists.file_access);
                            if(fileHandle < 0) {
                                H5Eprint(H5E_DEFAULT, stderr);
                                throw std::runtime_error("Failed to open file in read-only mode: " + FilePath.string());
                            } else {
                                return fileHandle;
                            }
                        }
                        case(AccessMode::READWRITE): {
                            h5pp::logger::log->trace("Opening file handle in READWRITE mode");
                            hid::h5f fileHandle = H5Fopen(FilePath.string().c_str(), H5F_ACC_RDWR, plists.file_access);
                            if(fileHandle < 0) {
                                H5Eprint(H5E_DEFAULT, stderr);
                                throw std::runtime_error("Failed to open file in read-write mode: " + FilePath.string());
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
            hid_t  error_stack                   = H5Eget_current_stack();
            herr_t turnOffAutomaticErrorPrinting = H5Eset_auto2(error_stack, nullptr, nullptr);
            H5Eclose_stack(error_stack);
            if(turnOffAutomaticErrorPrinting < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to turn off H5E error printing");
            }

            setOutputFilePath();
            h5pp::type::compound::initTypes();
            hasInitialized = true;
            //            fileCount++;
            h5pp::counter::ActiveFileCounter::incrementCounter(FileName.string());
            h5pp::logger::setLogger("h5pp|" + FileName.string(), logLevel, logTimestamp);
            //            h5pp::Logger::setLogger(savedLog,logLevel,false);
        }

        void setOutputFilePath() {
            h5pp::logger::log->trace("Attempting to set file name and path. File name [{}] path [{}]. Has initialized: {}", FileName.string(), FilePath.string(), hasInitialized);

            // There are different possibilities:
            // 1) File is being initialized from another h5pp File (e.g. by copy or assignment) In that case the following applies:
            //      a) FileName =  just a filename such as myfile.h5 without parent path.
            //      b) FilePath =  an absolute path to the file such as /home/yada/yada/myFile.h5
            // 2) File did not exist previously
            //      a) FileName = a filename possibly with parent path or not, such as ../myDir/myFile.h5 or just myFile
            //      b) FilePath = empty

            // Take case 2 first and make it into a case 1
            if(FilePath.empty()) {
                h5pp::logger::log->trace("File path empty. Detecting path...");
                FilePath = fs::absolute(FileName);
                FileName = FilePath.filename();
            }

            // Now we expect case 1 to hold.

            fs::path currentDir = fs::current_path();
            h5pp::logger::log->trace("Current path        : {}", fs::current_path().string());
            h5pp::logger::log->debug("Detected file name  : {}", FileName.string());
            h5pp::logger::log->debug("Detected file path  : {}", FilePath.string());

            try {
                if(fs::create_directories(FilePath.parent_path())) {
                    h5pp::logger::log->trace("Created directory: {}", FilePath.parent_path().string());
                } else {
                    h5pp::logger::log->trace("Directory already exists: {}", FilePath.parent_path().string());
                }
            } catch(std::exception &ex) { throw std::runtime_error("Failed to create directory: " + std::string(ex.what())); }

            switch(createMode) {
                case CreateMode::OPEN: {
                    h5pp::logger::log->debug("File mode [OPEN]: Opening file [{}]", FilePath.string());
                    try {
                        if(fileIsValid(FilePath)) {
                            hid_t file;
                            switch(accessMode) {
                                case(AccessMode::READONLY): file = H5Fopen(FilePath.string().c_str(), H5F_ACC_RDONLY, plists.file_access); break;
                                case(AccessMode::READWRITE): file = H5Fopen(FilePath.string().c_str(), H5F_ACC_RDWR, plists.file_access); break;
                                default: throw std::runtime_error("Invalid access mode");
                            }
                            if(file < 0) {
                                H5Eprint(H5E_DEFAULT, stderr);
                                throw std::runtime_error("Failed to open file: [" + FilePath.string() + "]");
                            }

                            H5Fclose(file);
                            FilePath = fs::canonical(FilePath);
                        } else {
                            throw std::runtime_error("Invalid file: [" + FilePath.string() + "]");
                        }
                    } catch(std::exception &ex) { throw std::runtime_error("Failed to open hdf5 file: " + std::string(ex.what())); }
                    break;
                }
                case CreateMode::TRUNCATE: {
                    h5pp::logger::log->debug("File mode [TRUNCATE]: Overwriting file if it exists: [{}]", FilePath.string());
                    try {
                        hid_t file = H5Fcreate(FilePath.string().c_str(), H5F_ACC_TRUNC, plists.file_create, plists.file_access);
                        if(file < 0) {
                            H5Eprint(H5E_DEFAULT, stderr);
                            throw std::runtime_error("Failed to create file: [" + FilePath.string() + "]");
                        }
                        H5Fclose(file);
                        FilePath = fs::canonical(FilePath);
                    } catch(std::exception &ex) { throw std::runtime_error("Failed to create hdf5 file: " + std::string(ex.what())); }
                    break;
                }
                case CreateMode::RENAME: {
                    try {
                        h5pp::logger::log->debug("File mode [RENAME]: Finding new file name if previous file exists: [{}]", FilePath.string());
                        if(fileIsValid(FilePath)) {
                            FilePath = getNewFileName(FilePath);
                            h5pp::logger::log->info("Previous file exists. Choosing new file name: [{}] ---> [{}]", FileName.string(), FilePath.filename().string());
                            FileName = FilePath.filename();
                        }
                        hid_t file = H5Fcreate(FilePath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plists.file_access);
                        if(file < 0) {
                            H5Eprint(H5E_DEFAULT, stderr);
                            throw std::runtime_error("Failed to create file: [" + FilePath.string() + "]");
                        }
                        H5Fclose(file);
                        FilePath = fs::canonical(FilePath);
                    } catch(std::exception &ex) { throw std::runtime_error("Failed to create renamed hdf5 file : " + std::string(ex.what())); }
                    break;
                }
                default: {
                    h5pp::logger::log->error("File Mode not set. Choose  CreateMode::<OPEN|TRUNCATE|RENAME>");
                    throw std::runtime_error("File Mode not set. Choose  CreateMode::<OPEN|TRUNCATE|RENAME>");
                }
            }
        }

        void setCreateMode(CreateMode createMode_) { createMode = createMode_; }
        void setAccessMode(AccessMode accessMode_) { accessMode = accessMode_; }
        //        void enableDefaultExtendable() { defaultExtendable = true; }
        //        void disableDefaultExtendable() { defaultExtendable = false; }

        // Functions for querying the file

        CreateMode getCreateMode() const { return createMode; }
        AccessMode getAccessMode() const { return accessMode; }

        std::string getFileName() const { return FileName.string(); }
        std::string getFilePath() const { return FilePath.string(); }

        void setLogLevel(size_t logLevelZeroToFive) {
            logLevel = logLevelZeroToFive;
            h5pp::logger::setLogLevel(logLevelZeroToFive);
        }

        // Functions related to datasets

        void setDefaultCompressionLevel(unsigned int compressionLevelZeroToNine) { defaultCompressionLevel = h5pp::hdf5::getValidCompressionLevel(compressionLevelZeroToNine); }
        unsigned int getDefaultCompressionLevel() const { return defaultCompressionLevel; }

        template<typename DataType>
        void writeDataset(const DataType &data, const DatasetProperties &dsetProps);

        template<typename DataType>
        void writeDataset(const DataType &                    data,
                          std::string_view                    dsetName,
                          std::optional<H5D_layout_t>         layout           = std::nullopt,
                          std::optional<std::vector<hsize_t>> chunkDimensions  = std::nullopt,
                          std::optional<unsigned int>         compressionLevel = std::nullopt);

        template<typename PointerType, typename T, size_t N, typename... Args, typename = std::enable_if_t<std::is_pointer_v<PointerType> and std::is_integral<T>::value>>
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

        template<typename DataType>
        void readDataset(DataType &data, std::string_view dsetName) const;

        template<typename DataType>
        DataType readDataset(std::string_view datasetPath) const;

        template<typename PointerType, typename T, size_t N, typename = std::enable_if_t<std::is_pointer_v<PointerType>>>
        void readDataset(PointerType ptr, const T (&dims)[N], std::string_view datasetPath) {
            auto wrapper = h5pp::PtrWrapper(ptr, dims);
            readDataset(wrapper, datasetPath);
        }

        template<typename PointerType, typename = std::enable_if_t<std::is_pointer_v<PointerType>>>
        void readDataset(PointerType ptr, size_t size, std::string_view datasetPath) {
            auto wrapper = h5pp::PtrWrapper(ptr, size);
            readDataset(wrapper, datasetPath);
        }

        // Functions related to attributes
        template<typename DataType>
        void writeAttribute(const DataType &attribute, const AttributeProperties &aprops);

        template<typename DataType>
        void writeAttribute(const DataType &attribute, std::string_view attrName, std::string_view linkName);

        inline std::vector<std::string> getAttributeNames(std::string_view linkPath) const {
            hid::h5f                 file           = openFileHandle();
            std::vector<std::string> attributeNames = h5pp::hdf5::getAttributeNames(file, linkPath, std::nullopt, plists);
            return attributeNames;
        }

        template<typename DataType>
        void readAttribute(DataType &data, std::string_view attrName, std::string_view linkName) const;

        template<typename DataType>
        DataType readAttribute(std::string_view attrName, std::string_view linkName) const;

        // Functions for querying
        std::vector<hsize_t> getDatasetDims(std::string_view datasetPath) {
            hid::h5f file    = openFileHandle();
            hid::h5d dataset = h5pp::hdf5::openObject<hid::h5d>(file, datasetPath);
            return h5pp::hdf5::getDimensions(dataset);
        }

        bool linkExists(std::string_view link) const {
            hid::h5f file   = openFileHandle();
            bool     exists = h5pp::hdf5::checkIfLinkExists(file, link, std::nullopt, plists);
            return exists;
        }

        std::vector<std::string> getContentsOfGroup(std::string_view groupName) const {
            hid::h5f file       = openFileHandle();
            auto     foundLinks = h5pp::hdf5::getContentsOfGroup(file, groupName);
            return foundLinks;
        }

        bool fileIsValid() const { return fileIsValid(FilePath); }

        static bool fileIsValid(const fs::path &fileName) { return fs::exists(fileName) and H5Fis_hdf5(fileName.string().c_str()) > 0; }

        inline void createGroup(std::string_view group_relative_name) {
            hid::h5f file = openFileHandle();
            h5pp::hdf5::createGroup(file, group_relative_name, std::nullopt, plists);
        }

        inline void writeSymbolicLink(std::string_view src_path, std::string_view tgt_path) {
            hid::h5f file = openFileHandle();
            h5pp::hdf5::writeSymbolicLink(file, src_path, tgt_path, plists);
        }

        private:
        fs::path getNewFileName(const fs::path &fileName) const {
            int      i           = 1;
            fs::path newFileName = fileName;
            while(fs::exists(newFileName)) { newFileName.replace_filename(fileName.stem().string() + "-" + std::to_string(i++) + fileName.extension().string()); }
            return newFileName;
        }

        void createDataset(hid_t file, DatasetProperties &dsetProps) {
            if(not dsetProps.dsetExists) {
                h5pp::logger::log->trace("Creating dataset: [{}]", dsetProps.dsetName.value());
                h5pp::hdf5::createDataset(file, dsetProps, plists);
            }
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
void h5pp::File::readDataset(DataType &data, std::string_view dsetName) const {
    hid::h5f file      = openFileHandle();
    auto     dsetProps = h5pp::scan::getDatasetProperties_read(file, dsetName, std::nullopt, plists);
    h5pp::hdf5::readDataset(data, dsetProps);
}

template<typename DataType>
DataType h5pp::File::readDataset(std::string_view datasetPath) const {
    DataType data;
    readDataset(data, datasetPath);
    return data;
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
    if constexpr(tc::is_eigen_any<DataType>::value and not tc::is_eigen_1d<DataType>::value) {
        h5pp::logger::log->debug("Converting data to row-major storage order");
        const auto tempRowm = eigen::to_RowMajor(data); // Convert to Row Major first;
        h5pp::hdf5::writeAttribute(tempRowm, attrProps);
        return;
    }
#endif

    h5pp::hdf5::writeAttribute(data, attrProps);
}

template<typename DataType>
void h5pp::File::readAttribute(DataType &data, std::string_view attrName, std::string_view linkName) const {
    hid::h5f file      = openFileHandle();
    auto     attrProps = h5pp::scan::getAttributeProperties_read(file, attrName, linkName, std::nullopt, std::nullopt, plists);
    h5pp::hdf5::readAttribute(data, attrProps);
}

template<typename DataType>
DataType h5pp::File::readAttribute(std::string_view attrName, std::string_view linkName) const {
    DataType data;
    readAttribute(data, attrName, linkName);
    return data;
}
