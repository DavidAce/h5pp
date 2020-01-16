
#pragma once

#include "h5ppAnalyze.h"
#include "h5ppAttributeProperties.h"
#include "h5ppConstants.h"
#include "h5ppDataProperties.h"
#include "h5ppDatasetProperties.h"
#include "h5ppFileCounter.h"
#include "h5ppHdf5.h"
#include "h5ppHid.h"
#include "h5ppLogger.h"
#include "h5ppPropertyLists.h"
#include "h5ppRawArrayWrapper.h"
#include "h5ppStdIsDetected.h"
#include "h5ppTextra.h"
#include "h5ppTypeCompoundCreate.h"
#include "h5ppTypeScan.h"
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

    namespace tc = h5pp::Type::Scan;
    /*!
     \brief Writes and reads data to a binary hdf5-file.
    */
    enum class CreateMode { OPEN, TRUNCATE, RENAME };
    enum class AccessMode { READONLY, READWRITE };

    class File {
        private:
        mutable herr_t retval = 0;
        fs::path       FileName; /*!< Filename and extension, e.g. output.h5 */
        fs::path       FilePath; /*!< Full path to the file, eg. /home/cooldude/h5project/output.h5 */

        AccessMode accessMode = AccessMode::READWRITE;
        CreateMode createMode = CreateMode::RENAME;

        size_t   logLevel     = 2;
        bool     logTimestamp = false;
        Hid::h5e error_stack;
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

        File() { h5pp::Logger::setLogger("h5pp", logLevel, logTimestamp); }

        File(const File &other) {
            h5pp::Logger::log->debug("Copy-constructing this file [{}] from given file: [{}]. Previously initialized (this): {}. Previously initialized (other): {}",
                                     FileName.string(),
                                     other.getFileName(),
                                     hasInitialized,
                                     other.hasInitialized);
            *this = other;
        }

        explicit File(std::string_view FileName_,
                      AccessMode       accessMode_   = AccessMode::READWRITE,
                      CreateMode       createMode_   = CreateMode::RENAME,
                      size_t           logLevel_     = 2,
                      bool             logTimestamp_ = false)
            : FileName(FileName_), accessMode(accessMode_), createMode(createMode_), logLevel(logLevel_), logTimestamp(logTimestamp_) {
            h5pp::Logger::setLogger("h5pp", logLevel, logTimestamp);
            h5pp::Logger::log->debug("Constructing h5pp file. Given path: [{}]", FileName.string());

            if(accessMode_ == AccessMode::READONLY and createMode_ == CreateMode::TRUNCATE) {
                Logger::log->error("Options READONLY and TRUNCATE are incompatible.");
                return;
            }
            initialize();
        }

        explicit File(std::string_view FileName_, CreateMode createMode_, size_t logLevel_ = 2, bool logTimestamp_ = false)
            : File(FileName_, AccessMode::READWRITE, createMode_, logLevel_, logTimestamp_) {}

        ~File() noexcept(false) {
            auto savedLog = h5pp::Logger::log->name();
            h5pp::Logger::setLogger("h5pp|exit", logLevel, logTimestamp);
            try {
                if(h5pp::Counter::ActiveFileCounter::getCount() == 1) { h5pp::Type::Compound::closeTypes(); }
                h5pp::Counter::ActiveFileCounter::decrementCounter(FileName.string());
                if(h5pp::Counter::ActiveFileCounter::getCount() == 0) {
                    h5pp::Logger::log->debug(
                        "Closing file: {}.", FileName.string(), h5pp::Counter::ActiveFileCounter::getCount(), h5pp::Counter::ActiveFileCounter::OpenFileNames());
                } else {
                    h5pp::Logger::log->debug("Closing file: {}. There are still {} files open: {}",
                                             getFileName(),
                                             h5pp::Counter::ActiveFileCounter::getCount(),
                                             h5pp::Counter::ActiveFileCounter::OpenFileNames());
                }
            } catch(std::exception &err) {
                H5Eprint(H5E_DEFAULT, stderr);
                h5pp::Logger::log->error("h5pp file destructor failed | file: [{}] | Reason: {}", getFilePath(), err.what());
                throw std::runtime_error("h5pp file destructor failed | file: [" + std::string(getFilePath()) + "] | Reason: " + std::string(err.what()));
            } catch(...) {
                H5Eprint(H5E_DEFAULT, stderr);
                h5pp::Logger::log->error("h5pp file destructor failed | file: [{}]", getFilePath());
                throw std::runtime_error("h5pp file destructor failed | file: [" + std::string(getFilePath()) + "]");
            }
            h5pp::Logger::setLogger(savedLog, logLevel, logTimestamp);
        }

        File &operator=(const File &other) {
            h5pp::Logger::log->debug("Assign-constructing this file [{}] from given file: [{}]. Previously initialized (this): {}. Previously initialized (other): {}",
                                     FileName.string(),
                                     other.getFileName(),
                                     hasInitialized,
                                     other.hasInitialized);
            if(&other != this) {
                if(hasInitialized) { h5pp::Counter::ActiveFileCounter::decrementCounter(FileName.string()); }
                if(other.hasInitialized) {
                    logLevel = other.logLevel;
                    h5pp::Logger::setLogger("h5pp", logLevel, logTimestamp);
                    accessMode = other.getAccessMode();
                    createMode = CreateMode::OPEN;
                    FileName   = other.FileName;
                    FilePath   = other.FilePath;
                    initialize();
                }
            }
            return *this;
        }

        Hid::h5f openFileHandle() const {
            try {
                if(hasInitialized) {
                    switch(accessMode) {
                        case(AccessMode::READONLY): {
                            h5pp::Logger::log->trace("Opening file handle in READONLY mode");
                            Hid::h5f fileHandle = H5Fopen(FilePath.string().c_str(), H5F_ACC_RDONLY, plists.file_access);
                            if(fileHandle < 0) {
                                H5Eprint(H5E_DEFAULT, stderr);
                                throw std::runtime_error("Failed to open file in read-only mode: " + FilePath.string());
                            } else {
                                return fileHandle;
                            }
                        }
                        case(AccessMode::READWRITE): {
                            h5pp::Logger::log->trace("Opening file handle in READWRITE mode");
                            Hid::h5f fileHandle = H5Fopen(FilePath.string().c_str(), H5F_ACC_RDWR, plists.file_access);
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
            auto savedLog = h5pp::Logger::log->name();
            h5pp::Logger::setLogger("h5pp|init", logLevel, logTimestamp);
            /* Turn off error handling permanently */
            hid_t  error_stack                   = H5Eget_current_stack();
            herr_t turnOffAutomaticErrorPrinting = H5Eset_auto2(error_stack, nullptr, nullptr);
            H5Eclose_stack(error_stack);
            if(turnOffAutomaticErrorPrinting < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to turn off H5E error printing");
            }

            setOutputFilePath();
            h5pp::Type::Compound::initTypes();
            hasInitialized = true;
            //            fileCount++;
            h5pp::Counter::ActiveFileCounter::incrementCounter(FileName.string());
            h5pp::Logger::setLogger("h5pp|" + FileName.string(), logLevel, logTimestamp);
            //            h5pp::Logger::setLogger(savedLog,logLevel,false);
        }

        void setOutputFilePath() {
            h5pp::Logger::log->trace("Attempting to set file name and path. File name [{}] path [{}]. Has initialized: {}", FileName.string(), FilePath.string(), hasInitialized);

            // There are different possibilities:
            // 1) File is being initialized from another h5pp File (e.g. by copy or assignment) In that case the following applies:
            //      a) FileName =  just a filename such as myfile.h5 without parent path.
            //      b) FilePath =  an absolute path to the file such as /home/yada/yada/myFile.h5
            // 2) File did not exist previously
            //      a) FileName = a filename possibly with parent path or not, such as ../myDir/myFile.h5 or just myFile
            //      b) FilePath = empty

            // Take case 2 first and make it into a case 1
            if(FilePath.empty()) {
                h5pp::Logger::log->trace("File path empty. Detecting path...");
                FilePath = fs::absolute(FileName);
                FileName = FilePath.filename();
            }

            // Now we expect case 1 to hold.

            fs::path currentDir = fs::current_path();
            h5pp::Logger::log->trace("Current path        : {}", fs::current_path().string());
            h5pp::Logger::log->debug("Detected file name  : {}", FileName.string());
            h5pp::Logger::log->debug("Detected file path  : {}", FilePath.string());

            try {
                if(fs::create_directories(FilePath.parent_path())) {
                    h5pp::Logger::log->trace("Created directory: {}", FilePath.parent_path().string());
                } else {
                    h5pp::Logger::log->trace("Directory already exists: {}", FilePath.parent_path().string());
                }
            } catch(std::exception &ex) { throw std::runtime_error("Failed to create directory: " + std::string(ex.what())); }

            switch(createMode) {
                case CreateMode::OPEN: {
                    h5pp::Logger::log->debug("File mode [OPEN]: Opening file [{}]", FilePath.string());
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
                    h5pp::Logger::log->debug("File mode [TRUNCATE]: Overwriting file if it exists: [{}]", FilePath.string());
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
                        h5pp::Logger::log->debug("File mode [RENAME]: Finding new file name if previous file exists: [{}]", FilePath.string());
                        if(fileIsValid(FilePath)) {
                            FilePath = getNewFileName(FilePath);
                            h5pp::Logger::log->info("Previous file exists. Choosing new file name: [{}] ---> [{}]", FileName.string(), FilePath.filename().string());
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
                    h5pp::Logger::log->error("File Mode not set. Choose  CreateMode::<OPEN|TRUNCATE|RENAME>");
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
            h5pp::Logger::setLogLevel(logLevelZeroToFive);
        }

        // Functions related to datasets

        void setDefaultCompressionLevel(unsigned int compressionLevelZeroToNine) { defaultCompressionLevel = h5pp::Hdf5::getValidCompressionLevel(compressionLevelZeroToNine); }
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
            writeDataset(h5pp::Wrapper::RawArrayWrapper(ptr, dims), datasetPath, layout, chunkDimensions, compressionLevel);
        }

        template<typename PointerType, typename... Args, typename = std::enable_if_t<std::is_pointer_v<PointerType>>>
        void writeDataset(const PointerType                   ptr,
                          const size_t                        size,
                          std::string_view                    datasetPath,
                          std::optional<H5D_layout_t>         layout           = std::nullopt,
                          std::optional<std::vector<hsize_t>> chunkDimensions  = std::nullopt,
                          std::optional<unsigned int>         compressionLevel = std::nullopt) {
            writeDataset(h5pp::Wrapper::RawArrayWrapper(ptr, size), datasetPath, layout, chunkDimensions, compressionLevel);
        }

        template<typename DataType>
        void readDataset(DataType &data, std::string_view dsetName) const;

        template<typename DataType>
        DataType readDataset(std::string_view datasetPath) const;

        template<typename DataType, typename T, size_t N>
        void readDataset(DataType *data, const T (&dims)[N], std::string_view datasetPath);

        template<typename DataType, typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
        void readDataset(DataType *data, T size, std::string_view datasetPath) {
            return readDataset(data, {size}, datasetPath);
        }

        // Functions related to attributes
        template<typename DataType>
        void writeAttribute(const DataType &attribute, const AttributeProperties &aprops);

        template<typename DataType>
        void writeAttribute(const DataType &attribute, std::string_view attributeName, std::string_view linkPath);

        template<typename DataType>
        void writeAttributeToFile(const DataType &attribute, std::string_view attributeName);

        inline std::vector<std::string_view> getAttributeNames(std::string_view linkPath) const {
            Hid::h5f                      file           = openFileHandle();
            std::vector<std::string_view> attributeNames = h5pp::Hdf5::getAttributeNames(file, linkPath, std::nullopt, plists);
            return attributeNames;
        }

        template<typename DataType>
        void readAttribute(DataType &data, std::string_view attributeName, std::string_view linkPath) const;

        template<typename DataType>
        DataType readAttribute(std::string_view attributeName, std::string_view linkPath) const;

        // Functions for querying
        std::vector<size_t> getDatasetDims(std::string_view datasetPath) {
            Hid::h5f             file = openFileHandle();
            std::vector<hsize_t> dims;
            Hid::h5o             dataset  = h5pp::Hdf5::openLink(file, datasetPath);
            Hid::h5s             memspace = H5Dget_space(dataset);
            int                  ndims    = H5Sget_simple_extent_ndims(memspace);
            dims.resize(ndims);
            H5Sget_simple_extent_dims(memspace, dims.data(), nullptr);

            std::vector<size_t> retdims(dims.begin(), dims.end());
            return retdims;
        }

        bool linkExists(std::string_view link) const {
            Hid::h5f file   = openFileHandle();
            bool     exists = h5pp::Hdf5::checkIfLinkExists(file, link, std::nullopt, plists);
            return exists;
        }

        std::vector<std::string> getContentsOfGroup(std::string_view groupName) const {
            Hid::h5f file = openFileHandle();
            h5pp::Logger::log->trace("Getting contents of group: [{}]", groupName);
            auto foundLinks = h5pp::Hdf5::getContentsOfGroup(file, groupName);
            return foundLinks;
        }

        bool fileIsValid() const { return fileIsValid(FilePath); }

        static bool fileIsValid(const fs::path &fileName) { return fs::exists(fileName) and H5Fis_hdf5(fileName.string().c_str()) > 0; }

        inline void createGroup(std::string_view group_relative_name) {
            Hid::h5f file = openFileHandle();
            h5pp::Hdf5::createGroup(file, group_relative_name);
        }

        inline void writeSymbolicLink(std::string_view src_path, std::string_view tgt_path) {
            Hid::h5f file = openFileHandle();
            h5pp::Logger::log->trace("Creating symbolik link: [{}] --> [{}]", src_path, tgt_path);
            h5pp::Hdf5::writeSymbolicLink(file, src_path, tgt_path);
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
                h5pp::Logger::log->trace("Creating dataset: [{}]", dsetProps.dsetName.value());
                h5pp::Hdf5::createDataset(file, dsetProps, plists);
            }
        }
    };
}

template<typename DataType>
void h5pp::File::writeDataset(const DataType &data, const DatasetProperties &dsetProps) {
    Hid::h5f file = openFileHandle();
    h5pp::Hdf5::writeDataset(file, data, dsetProps, plists);
}

template<typename DataType>
void h5pp::File::writeDataset(const DataType &                    data,
                              std::string_view                    dsetName,
                              std::optional<H5D_layout_t>         layout,
                              std::optional<std::vector<hsize_t>> chunkDimensions,
                              std::optional<unsigned int>         compressionLevel) {
    if(accessMode == AccessMode::READONLY) { throw std::runtime_error("Attempted to write to read-only file"); }
    Hid::h5f file = openFileHandle();

    if(compressionLevel)
        compressionLevel = h5pp::Hdf5::getValidCompressionLevel(compressionLevel);
    else
        compressionLevel = getDefaultCompressionLevel();

    auto dsetProps = h5pp::Analyze::getDatasetProperties_write(file, dsetName, data, std::nullopt, layout, chunkDimensions, compressionLevel);
    // Create the dataset id and set its properties
    h5pp::Hdf5::createDataset(file, dsetProps);
    h5pp::Hdf5::setDatasetExtent(dsetProps);
    dsetProps.fileSpace = H5Dget_space(dsetProps.dataSet);
    if(dsetProps.layout.value() == H5D_CHUNKED) h5pp::Hdf5::selectHyperslab(dsetProps.fileSpace, dsetProps.memSpace);

#ifdef H5PP_EIGEN3
    if constexpr(tc::is_eigen_type<DataType>::value and not tc::is_eigen_1d<DataType>::value) {
        h5pp::Logger::log->debug("Converting data to row-major storage order");
        const auto tempRowm = Textra::to_RowMajor(data); // Convert to Row Major first;
        h5pp::Hdf5::writeDataset(file, tempRowm, dsetProps, plists);
        return;
    }
#endif
    h5pp::Hdf5::writeDataset(file, data, dsetProps);
    H5Eprint(H5E_DEFAULT, stderr);
}

template<typename DataType>
void h5pp::File::readDataset(DataType &data, std::string_view dsetName) const {
    Hid::h5f file = openFileHandle();
    try {
        auto props = h5pp::Analyze::getDatasetProperties_read(file, dsetName, std::nullopt, plists);
        h5pp::Logger::log->debug(
            "Reading dataset: [{}] | size {} | bytes {} | ndims {} | dims {}", dsetName, props.size.value(), props.bytes.value(), props.ndims.value(), props.dims.value());
#ifdef H5PP_EIGEN3
        if constexpr(tc::is_eigen_core<DataType>::value) {
            if(data.IsRowMajor) {
                // Data is RowMajor in HDF5, user gave a RowMajor container so no need to swap layout.
                data.resize(dims[0], dims[1]);
                retval = H5LTread_dataset(file, dsetName.c_str(), datatype, data.data());
                if(retval < 0) { throw std::runtime_error("Failed to read Eigen Matrix rowmajor dataset"); }
            } else {
                // Data is RowMajor in HDF5, user gave a ColMajor container we need to swap layout.
                Eigen::Matrix<typename DataType::Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> matrixRowmajor;
                matrixRowmajor.resize(dims[0], dims[1]); // Data is transposed in HDF5!
                retval = H5LTread_dataset(file, dsetName.c_str(), datatype, matrixRowmajor.data());
                if(retval < 0) { throw std::runtime_error("Failed to read Eigen Matrix colmajor dataset"); }
                data = matrixRowmajor;
            }
        } else if constexpr(tc::is_eigen_tensor<DataType>()) {
            auto eigenDims = Textra::copy_dims<DataType::NumDimensions>(dims);
            if constexpr(DataType::Options == Eigen::RowMajor) {
                // Data is RowMajor in HDF5, user gave a RowMajor container so no need to swap layout.
                data.resize(eigenDims);
                retval = H5LTread_dataset(file, dsetName.c_str(), datatype, data.data());
                if(retval < 0) { throw std::runtime_error("Failed to read Eigen Tensor rowmajor dataset"); }
            } else {
                // Data is RowMajor in HDF5, user gave a ColMajor container we need to swap layout.
                Eigen::Tensor<typename DataType::Scalar, DataType::NumIndices, Eigen::RowMajor> tensorRowmajor(eigenDims);
                H5LTread_dataset(file, dsetName.c_str(), datatype, tensorRowmajor.data());
                if(retval < 0) { throw std::runtime_error("Failed to read Eigen Tensor colmajor dataset"); }
                data = Textra::to_ColMajor(tensorRowmajor);
            }
        } else
#endif // H5PP_EIGEN3

            if constexpr(tc::is_std_vector<DataType>::value) {
            assert(props.ndims == 1 and "Vector cannot take 2D datasets");
            data.resize(props.dims.value()[0]);
            H5LTread_dataset(file, std::string(dsetName).c_str(), props.dataType, data.data());
            if(retval < 0) { throw std::runtime_error("Failed to read std::vector dataset"); }
        } else if constexpr(std::is_same<std::string, DataType>::value) {
            assert(props.ndims == 1 and "std string needs to have 1 dimension");
            hsize_t stringsize = H5Dget_storage_size(props.dataSet);
            data.resize(stringsize);
            retval = H5LTread_dataset(file, std::string(dsetName).c_str(), props.dataType, data.data());
            if(retval < 0) { throw std::runtime_error("Failed to read std::string dataset"); }

        } else if constexpr(std::is_arithmetic<DataType>::value) {
            retval = H5LTread_dataset(file, std::string(dsetName).c_str(), props.dataType, &data);
            if(retval < 0) { throw std::runtime_error("Failed to read arithmetic type dataset"); }

        } else {
            Logger::log->error("Attempted to read dataset of unknown type. Name: [{}] | Type: [{}]", dsetName, Type::Scan::type_name<DataType>());
            throw std::runtime_error("Attempted to read dataset of unknown type");
        }

    } catch(std::exception &ex) {
        H5Eprint(H5E_DEFAULT, stderr);
        throw std::runtime_error(
            h5pp::Logger::format("readDataset failed. Dataset name [{}] | Link [{}] | type [{}] | Reason: {}", dsetName, Type::Scan::type_name<DataType>(), ex.what()));
    }
}

template<typename DataType, typename T, size_t N>
void h5pp::File::readDataset(DataType *data, const T (&dims)[N], std::string_view datasetPath) {
    // This function takes a pointer and a specifiation of dimensions.

#ifdef H5PP_EIGEN3
    // Easiest thing to do is to wrap this in an Eigen::Tensor and send to readDataset
    auto dimsizes   = Textra::copy_dims<N>(dims);
    auto tensorWrap = Eigen::TensorMap<Eigen::Tensor<DataType, N>>(data, dimsizes);
    readDataset(tensorWrap, datasetPath);
#else
    Hid::h5f file = openFileHandle();
    try {
        Hid::h5o dataset  = h5pp::Hdf5::openLink(file, datasetPath);
        Hid::h5s memspace = H5Dget_space(dataset);
        Hid::h5s datatype = H5Dget_type(dataset);

        std::array<hsize_t, N> h5dims;
        H5Sget_simple_extent_dims(memspace, h5dims.data(), nullptr);
        int     ndims = (int) N;
        hsize_t size  = 1;
        for(size_t idx = 0; idx < ndims; idx++) size *= h5dims[idx];
        for(size_t idx = 0; idx < ndims; idx++)
            if(h5dims[idx] != dims[idx])
                throw std::runtime_error("Dimension mismatch in index " + std::to_string(idx) + ": " + std::to_string(h5dims[idx]) + "!=" + std::to_string(dims[idx]));
        h5pp::Logger::log->debug("Reading dataset: [{}] | size {} | ndims {} | dims {}", datasetPath, size, ndims, h5dims);
        retval = H5LTread_dataset(file, std::string(datasetPath).c_str(), datatype, data);
        if(retval < 0) throw std::runtime_error("Failed to read arithmetic type dataset");
    } catch(std::exception &ex) {
        H5Eprint(H5E_DEFAULT, stderr);
        throw std::runtime_error(
            h5pp::Logger::format("readDataset failed. Dataset name [{}] | Link [{}] | type [{}] | Reason: {}", datasetPath, Type::Scan::type_name<DataType>(), ex.what()));
    }
#endif
}

template<typename DataType>
DataType h5pp::File::readDataset(std::string_view datasetPath) const {
    DataType data;
    readDataset(data, datasetPath);
    return data;
}

template<typename DataType>
void h5pp::File::writeAttributeToFile(const DataType &attribute, std::string_view attributeName) {
    Hid::h5f file     = openFileHandle();
    Hid::h5d datatype = h5pp::Utils::getH5DataType<DataType>();
    auto     size     = h5pp::Utils::getSize(attribute);
    auto     byteSize = h5pp::Utils::getBytesTotal<DataType>(attribute);
    auto     ndims    = h5pp::Utils::getRank<DataType>();
    auto     dims     = h5pp::Utils::getDimensions(attribute);
    Hid::h5s memspace = h5pp::Utils::getMemSpace(ndims, dims);
    h5pp::Logger::log->debug(
        "Writing attribute to file: [{}] | size {} | bytes {} | ndims {} | dims {} | type {}", attributeName, size, byteSize, ndims, dims, Type::Scan::type_name<DataType>());

    //    if constexpr(tc::is_text<DataType>()) { h5pp::Utils::setStringSize(datatype, size); }

    Hid::h5a attributeId = H5Acreate(file, std::string(attributeName).c_str(), datatype, memspace, H5P_DEFAULT, H5P_DEFAULT);
    if constexpr(tc::hasMember_c_str<DataType>::value) {
        retval = H5Awrite(attributeId, datatype, attribute.c_str());
    } else if constexpr(tc::hasMember_data<DataType>::value) {
        retval = H5Awrite(attributeId, datatype, attribute.data());
    } else {
        retval = H5Awrite(attributeId, datatype, &attribute);
    }
    if(retval < 0) {
        H5Eprint(H5E_DEFAULT, stderr);
        throw std::runtime_error("Failed to write attribute [ " + std::string(attributeName) + " ] to file");
    }
}

template<typename DataType>
void h5pp::File::writeAttribute(const DataType &attribute, const AttributeProperties &aprops) {
    Hid::h5f file = openFileHandle();
    if(h5pp::Hdf5::checkIfLinkExists(file, aprops.linkPath)) {
        if(not h5pp::Hdf5::checkIfAttributeExists(file, aprops.linkPath, aprops.attrName)) {
            Hid::h5o linkObject  = h5pp::Hdf5::openLink(file, aprops.linkPath);
            Hid::h5a attributeId = H5Acreate(linkObject, aprops.attrName.c_str(), aprops.dataType, aprops.memSpace, H5P_DEFAULT, H5P_DEFAULT);
            h5pp::Logger::log->debug(
                "Writing attribute: [{}] | size {} | ndims {} | dims {} | type {}", aprops.attrName, aprops.size, aprops.ndims, aprops.dims, Type::Scan::type_name<DataType>());
            try {
                if constexpr(tc::hasMember_c_str<DataType>::value) {
                    retval = H5Awrite(attributeId, aprops.dataType, attribute.c_str());
                } else if constexpr(tc::hasMember_data<DataType>::value) {
                    retval = H5Awrite(attributeId, aprops.dataType, attribute.data());
                } else {
                    retval = H5Awrite(attributeId, aprops.dataType, &attribute);
                }
                if(retval < 0) { throw std::runtime_error("Failed to write attribute. Attribute name: [ " + aprops.attrName + " ]"); }
            } catch(std::exception &ex) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Link [ " + aprops.linkPath + " ]: " + std::string(ex.what()));
            }
        }
    } else {
        std::string error = "Link " + aprops.linkPath + " does not exist, yet attribute is being written.";
        h5pp::Logger::log->critical(error);
        throw(std::logic_error(error));
    }
}

template<typename DataType>
void h5pp::File::writeAttribute(const DataType &attribute, std::string_view attributeName, std::string_view linkPath) {
    AttributeProperties aprops;
    aprops.dataType = h5pp::Utils::getH5DataType<DataType>();
    aprops.size     = h5pp::Utils::getSize(attribute);
    aprops.byteSize = h5pp::Utils::getBytesTotal<DataType>(attribute);
    aprops.ndims    = h5pp::Utils::getRank<DataType>();
    aprops.dims     = h5pp::Utils::getDimensions(attribute);
    aprops.attrName = attributeName;
    aprops.linkPath = linkPath;
    aprops.memSpace = h5pp::Utils::getMemSpace(aprops.size, aprops.ndims, aprops.dims);
#ifdef H5PP_EIGEN3
    if constexpr(tc::is_eigen_type<DataType>::value and not tc::is_eigen_1d<DataType>::value) {
        h5pp::Logger::log->debug("Converting data to row-major storage order");
        const auto tempRowm = Textra::to_RowMajor(attribute); // Convert to Row Major first;
        writeAttribute(tempRowm, aprops);
    } else
#endif
        if constexpr(tc::is_text<DataType>()) {
        //        aprops.size = h5pp::Utils::setStringSize(aprops.dataType, aprops.size);
        writeAttribute(attribute, aprops);
    } else {
        writeAttribute(attribute, aprops);
    }
}

template<typename DataType>
void h5pp::File::readAttribute(DataType &data, std::string_view attributeName, std::string_view linkPath) const {
    Hid::h5f             file           = openFileHandle();
    Hid::h5o             link           = h5pp::Hdf5::openLink(file, linkPath);
    Hid::h5a             link_attribute = H5Aopen_name(link, std::string(attributeName).c_str());
    Hid::h5s             memspace       = H5Aget_space(link_attribute);
    Hid::h5t             datatype       = H5Aget_type(link_attribute);
    int                  ndims          = H5Sget_simple_extent_ndims(memspace);
    std::vector<hsize_t> dims(ndims);
    H5Sget_simple_extent_dims(memspace, dims.data(), nullptr);
    hsize_t size = 1;
    for(const auto &dim : dims) size *= dim;

    h5pp::Logger::log->debug(
        "Reading attribute: [{}] | link {} | size {} | ndims {} | dims {} | type {}", attributeName, linkPath, size, ndims, dims, Type::Scan::type_name<DataType>());
    h5pp::Utils::assertBytesPerElemMatch<DataType>(datatype);

#ifdef H5PP_EIGEN3
    if constexpr(tc::is_eigen_core<DataType>::value) {
        if(data.IsRowMajor) {
            // Data is RowMajor in HDF5, user gave a RowMajor container so no need to swap layout.
            data.resize(dims[0], dims[1]);
            if(H5Aread(link_attribute, datatype, data.data()) < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to read Eigen Matrix rowmajor dataset");
            }
        } else {
            // Data is RowMajor in HDF5, user gave a ColMajor container so we need to swap the layout.
            Eigen::Matrix<typename DataType::Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> matrixRowmajor;
            matrixRowmajor.resize(dims[0], dims[1]); // Data is transposed in HDF5!
            if(H5Aread(link_attribute, datatype, matrixRowmajor.data()) < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to read Eigen Matrix colmajor dataset");
            }
            data = matrixRowmajor;
        }

    } else if constexpr(tc::is_eigen_tensor<DataType>()) {
        auto eigenDims = Textra::copy_dims<DataType::NumDimensions>(dims);
        if constexpr(DataType::Options == Eigen::RowMajor) {
            // Data is RowMajor in HDF5, user gave a RowMajor container so no need to swap layout.
            data.resize(eigenDims);
            if(H5Aread(link_attribute, datatype, data.data()) < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to read Eigen Tensor rowmajor dataset");
            }
        } else {
            // Data is RowMajor in HDF5, user gave a ColMajor container so we need to swap the layout.
            Eigen::Tensor<typename DataType::Scalar, DataType::NumIndices, Eigen::RowMajor> tensorRowmajor(eigenDims);
            if(H5Aread(link_attribute, datatype, tensorRowmajor.data()) < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to read Eigen Tensor colmajor dataset");
            }
            data = Textra::to_ColMajor(tensorRowmajor);
        }
    } else
#endif
        if constexpr(tc::is_std_vector<DataType>::value) {
        if(ndims != 1) throw std::runtime_error("Vector cannot take datatypes with dimension: " + std::to_string(ndims));
        data.resize(dims[0]);
        if(H5Aread(link_attribute, datatype, data.data()) < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to read std::vector dataset");
        }
    } else if constexpr(tc::is_std_array<DataType>::value) {
        if(ndims != 1) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Array cannot take datatypes with dimension: " + std::to_string(ndims));
        }
        if(H5Aread(link_attribute, datatype, data.data()) < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to read std::vector dataset");
        }
    } else if constexpr(std::is_same<std::string, DataType>::value) {
        if(ndims != 1) throw std::runtime_error("std::string expected ndims 1. Got: " + std::to_string(ndims));
        hsize_t stringsize = H5Tget_size(datatype);
        data.resize(stringsize);
        if(H5Aread(link_attribute, datatype, data.data()) < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to read std::string dataset");
        }

    } else if constexpr(std::is_arithmetic<DataType>::value or tc::is_StdComplex<DataType>()) {
        if(H5Aread(link_attribute, datatype, &data) < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to read arithmetic type dataset");
        }
    } else {
        Logger::log->error("Attempted to read attribute of unknown type. Name: [{}] | Type: [{}]", attributeName, Type::Scan::type_name<DataType>());
        throw std::runtime_error("Attempted to read dataset of unknown type");
    }
}

template<typename DataType>
DataType h5pp::File::readAttribute(std::string_view attributeName, std::string_view linkPath) const {
    DataType data;
    readAttribute(data, attributeName, linkPath);
    return data;
}
