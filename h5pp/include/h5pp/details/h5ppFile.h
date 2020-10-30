
#pragma once

#include "h5ppConstants.h"
#include "h5ppDimensionType.h"
#include "h5ppEigen.h"
#include "h5ppEnums.h"
#include "h5ppFilesystem.h"
#include "h5ppHdf5.h"
#include "h5ppHid.h"
#include "h5ppInitListType.h"
#include "h5ppLogger.h"
#include "h5ppOptional.h"
#include "h5ppPropertyLists.h"
#include "h5ppScan.h"
#include "h5ppTypeCompoundCreate.h"
#include "h5ppUtils.h"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <string>
#include <utility>

namespace h5pp {

    /*!
     \brief Writes and reads data to a binary hdf5-file.
    */

    class File {
        private:
        fs::path             filePath;                                  /*!< Full path to the file, e.g. /path/to/project/filename.h5 */
        h5pp::FilePermission permission = h5pp::FilePermission::RENAME; /*!< Decides action on file collision and read/write permission on
                                                                           existing files. Default RENAME avoids loss of data  */
        size_t logLevel = 2; /*!< Console log level for new file objects. 0 [trace] has highest verbosity, and 5 [critical] the lowest.  */
        bool   logTimestamp = false; /*!< Add a time stamp to console log output   */
        hid::h5e     error_stack;    /*!< Holds a reference to the error stack used by HDF5   */
        unsigned int currentCompressionLevel = 0;

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

        /*! Default constructor */
        File() = default;

        explicit File(h5pp::fs::path       filePath_,
                      h5pp::FilePermission permission_   = h5pp::FilePermission::RENAME,
                      size_t               logLevel_     = 2,
                      bool                 logTimestamp_ = false)
            : filePath(std::move(filePath_)), permission(permission_), logLevel(logLevel_), logTimestamp(logTimestamp_) {
            init();
        }

        explicit File(h5pp::fs::path filePath_, unsigned int H5F_ACC_FLAGS, size_t logLevel_ = 2, bool logTimestamp_ = false)
            : filePath(std::move(filePath_)), logLevel(logLevel_), logTimestamp(logTimestamp_) {
            permission = h5pp::hdf5::convertFileAccessFlags(H5F_ACC_FLAGS);
            init();
        }

        /* Flush HDF5 file cache */
        void flush() {
            H5Fflush(openFileHandle(), H5F_scope_t::H5F_SCOPE_GLOBAL);
            h5pp::logger::log->trace("Flushing caches");
            H5garbage_collect();
            H5Eprint(H5E_DEFAULT, stderr);
        }

        /* Returns an HDF5 file handke with permission specified by File::permission */
        [[nodiscard]] hid::h5f openFileHandle() const {
            h5pp::logger::setLogger("h5pp|" + filePath.filename().string(), logLevel, logTimestamp);
            if(permission == h5pp::FilePermission::READONLY) {
                h5pp::logger::log->trace("Opening file in READONLY mode");
                hid_t fileHandle = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, plists.fileAccess);
                if(fileHandle < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error(h5pp::format("Failed to open file in read-only mode [{}]", filePath.string()));
                } else
                    return fileHandle;
            } else {
                h5pp::logger::log->trace("Opening file in READWRITE mode");
                hid_t fileHandle = H5Fopen(filePath.string().c_str(), H5F_ACC_RDWR, plists.fileAccess);
                if(fileHandle < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error(h5pp::format("Failed to open file in read-write mode [{}]", filePath.string()));
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

        void setDriver_sec2() {
            plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fapl_sec2(plists.fileAccess);
        }
        void setDriver_stdio() {
            plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fapl_stdio(plists.fileAccess);
        }
        void setDriver_core(bool writeOnClose = true, size_t bytesPerMalloc = 10240000) {
            plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fapl_core(plists.fileAccess, bytesPerMalloc, static_cast<hbool_t>(writeOnClose));
        }
#ifdef H5_HAVE_PARALLEL
        void setDriver_mpio(MPI_Comm comm, MPI_Info info) {
            plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fapl_mpio(plists.fileAccess, comm, info);
        }
#endif

        /*
         *
         * Functions for managing file location
         *
         */

        [[maybe_unused]] fs::path copyFileTo(const h5pp::fs::path &targetFilePath,
                                             const FilePermission &perm = FilePermission::COLLISION_FAIL) const {
            return h5pp::hdf5::copyFile(getFilePath(), targetFilePath, perm, plists);
        }

        [[maybe_unused]] fs::path moveFileTo(const h5pp::fs::path &targetFilePath,
                                             const FilePermission &perm = FilePermission::COLLISION_FAIL) {
            auto newPath = h5pp::hdf5::moveFile(getFilePath(), targetFilePath, perm, plists);
            if(fs::exists(newPath)) { filePath = newPath; }
            return newPath;
        }

        /*
         *
         * Functions for transfering contents between locations or files
         *
         */

        void copyLinkToFile(std::string_view      localLinkPath,
                            const h5pp::fs::path &targetFilePath,
                            std::string_view      targetLinkPath,
                            const FilePermission &targetFileCreatePermission = FilePermission::READWRITE) const {
            return h5pp::hdf5::copyLink(getFilePath(), localLinkPath, targetFilePath, targetLinkPath, targetFileCreatePermission, plists);
        }

        void copyLinkFromFile(std::string_view localLinkPath, const h5pp::fs::path &sourceFilePath, std::string_view sourceLinkPath) {
            return h5pp::hdf5::copyLink(
                sourceFilePath, sourceLinkPath, getFilePath(), localLinkPath, h5pp::FilePermission::READWRITE, plists);
        }

        template<typename h5x_tgt, typename = h5pp::type::sfinae::enable_if_is_h5_loc_t<h5x_tgt>>
        void copyLinkToLocation(std::string_view localLinkPath, const h5x_tgt &targetLocationId, std::string_view targetLinkPath) const {
            return h5pp::hdf5::copyLink(openFileHandle(), localLinkPath, targetLocationId, targetLinkPath, plists);
        }

        template<typename h5x_src, typename = h5pp::type::sfinae::enable_if_is_h5_loc_t<h5x_src>>
        void copyLinkFromLocation(std::string_view localLinkPath, const h5x_src &sourceLocationId, std::string_view sourceLinkPath) {
            return h5pp::hdf5::copyLink(sourceLocationId, sourceLinkPath, openFileHandle(), localLinkPath, plists);
        }

        void moveLinkToFile(std::string_view      localLinkPath,
                            const h5pp::fs::path &targetFilePath,
                            std::string_view      targetLinkPath,
                            const FilePermission &targetFileCreatePermission = FilePermission::READWRITE) const {
            return h5pp::hdf5::moveLink(getFilePath(), localLinkPath, targetFilePath, targetLinkPath, targetFileCreatePermission, plists);
        }

        void moveLinkFromFile(std::string_view localLinkPath, const h5pp::fs::path &sourceFilePath, std::string_view sourceLinkPath) {
            return h5pp::hdf5::moveLink(
                sourceFilePath, sourceLinkPath, getFilePath(), localLinkPath, h5pp::FilePermission::READWRITE, plists);
        }

        template<typename h5x_tgt, typename = h5pp::type::sfinae::enable_if_is_h5_loc_t<h5x_tgt>>
        void moveLinkToLocation(std::string_view localLinkPath,
                                const h5x_tgt &  targetLocationId,
                                std::string_view targetLinkPath,
                                LocationMode     locMode = LocationMode::DETECT) const {
            return h5pp::hdf5::moveLink(openFileHandle(), localLinkPath, targetLocationId, targetLinkPath, locMode, plists);
        }

        template<typename h5x_src, typename = h5pp::type::sfinae::enable_if_is_h5_loc_t<h5x_src>>
        void moveLinkFromLocation(std::string_view localLinkPath,
                                  const h5x_src &  sourceLocationId,
                                  std::string_view sourceLinkPath,
                                  LocationMode     locMode = LocationMode::DETECT) {
            return h5pp::hdf5::moveLink(sourceLocationId, sourceLinkPath, openFileHandle(), localLinkPath, locMode, plists);
        }

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

        void setCompressionLevel(unsigned int compressionLevelZeroToNine) {
            currentCompressionLevel = h5pp::hdf5::getValidCompressionLevel(compressionLevelZeroToNine);
        }
        [[nodiscard]] unsigned int getCompressionLevel() const { return currentCompressionLevel; }
        [[nodiscard]] unsigned int getCompressionLevel(std::optional<unsigned int> compressionLevel) const {
            if(compressionLevel)
                return h5pp::hdf5::getValidCompressionLevel(compressionLevel.value());
            else
                return currentCompressionLevel;
        }

        void createGroup(std::string_view group_relative_name) {
            h5pp::hdf5::createGroup(openFileHandle(), group_relative_name, std::nullopt, plists);
        }

        void resizeDataset(DsetInfo &info, const DimsType &newDimensions, std::optional<h5pp::ResizeMode> mode_override = std::nullopt) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to resize dataset on read-only file [{}]", filePath.string()));
            h5pp::hdf5::resizeDataset(info, newDimensions, mode_override);
        }

        DsetInfo
            resizeDataset(std::string_view dsetPath, const DimsType &newDimensions, std::optional<h5pp::ResizeMode> mode = std::nullopt) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to resize dataset on read-only file [{}]", filePath.string()));
            Options options;
            options.linkPath   = dsetPath;
            options.resizeMode = mode;
            auto info          = h5pp::scan::inferDsetInfo(openFileHandle(), dsetPath, options, plists);
            if(not info.dsetExists.value())
                throw std::runtime_error(h5pp::format("Failed to resize dataset [{}]: dataset does not exist", dsetPath));
            h5pp::hdf5::resizeDataset(info, newDimensions, mode);
            return info;
        }

        void createDataset(DsetInfo &info) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to create dataset on read-only file [{}]", filePath.string()));
            h5pp::hdf5::createDataset(info, plists);
        }

        DsetInfo createDataset(const Options &options) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to create dataset on read-only file [{}]", filePath.string()));
            options.assertWellDefined();
            if(not options.linkPath) throw std::runtime_error(h5pp::format("Error creating dataset: No dataset path specified"));
            if(not options.dataDims)
                throw std::runtime_error(
                    h5pp::format("Error creating dataset [{}]: Dimensions or size not specified", options.linkPath.value()));
            if(not options.h5Type)
                throw std::runtime_error(h5pp::format("Error creating dataset [{}]: HDF5 type not specified", options.linkPath.value()));
            auto dsetInfo = h5pp::scan::inferDsetInfo(openFileHandle(), options, plists);
            h5pp::File::createDataset(dsetInfo);
            return dsetInfo;
        }

        DsetInfo createDataset(std::optional<hid::h5t>     h5Type,
                               std::string_view            dsetPath,
                               const DimsType &            dsetDims,
                               std::optional<H5D_layout_t> h5Layout      = std::nullopt,
                               const OptDimsType &         dsetDimsChunk = std::nullopt,
                               const OptDimsType &         dsetDimsMax   = std::nullopt,
                               std::optional<unsigned int> compression   = std::nullopt) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to create dataset on read-only file [{}]", filePath.string()));
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dsetDims;
            options.dsetDimsChunk = dsetDimsChunk;
            options.dsetDimsMax   = dsetDimsMax;
            options.h5Type        = std::move(h5Type);
            options.h5Layout      = h5Layout;
            options.compression   = getCompressionLevel(compression);
            return createDataset(options);
        }

        template<typename DataType>
        DsetInfo createDataset(const DataType &data, const Options &options) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to create dataset on read-only file [{}]", filePath.string()));
            auto dsetInfo = h5pp::scan::inferDsetInfo(openFileHandle(), data, options, plists);
            h5pp::File::createDataset(dsetInfo);
            return dsetInfo;
        }

        template<typename DataType, typename = h5pp::type::sfinae::enable_if_not_h5_type<DataType>>
        DsetInfo createDataset(const DataType &            data,
                               std::string_view            dsetPath,
                               const OptDimsType &         dataDims      = std::nullopt,
                               std::optional<H5D_layout_t> h5Layout      = std::nullopt,
                               const OptDimsType &         dsetDimsChunk = std::nullopt,
                               const OptDimsType &         dsetDimsMax   = std::nullopt,
                               std::optional<unsigned int> compression   = std::nullopt) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to create dataset on read-only file [{}]", filePath.string()));
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetDimsChunk = dsetDimsChunk;
            options.dsetDimsMax   = dsetDimsMax;
            options.h5Type        = h5pp::util::getH5Type<DataType>();
            options.h5Layout      = h5Layout;
            options.compression   = getCompressionLevel(compression);
            // If dsetdims is a nullopt we can infer its dimensions from the given dataset
            return createDataset(data, options);
        }

        template<typename DataType>
        void writeDataset(const DataType &data, DsetInfo &dsetInfo, const Options &options = Options()) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            if(not dsetInfo.dsetExists or not dsetInfo.dsetExists.value()) createDataset(dsetInfo);
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            resizeDataset(dsetInfo, dataInfo.dataDims.value());
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists);
        }

        template<typename DataType>
        void writeDataset(const DataType &data, DataInfo &dataInfo, DsetInfo &dsetInfo, const Options &options = Options()) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            // The infos passed should be parsed and ready to write with
            if(dsetInfo.hasLocId())
                h5pp::scan::readDsetInfo(dsetInfo, dsetInfo.getLocId(), options, plists);
            else
                h5pp::scan::readDsetInfo(dsetInfo, openFileHandle(), options, plists);

            h5pp::scan::scanDataInfo(dataInfo, data, options);
            if(not dsetInfo.dsetExists or not dsetInfo.dsetExists.value()) createDataset(dsetInfo);
            resizeDataset(dsetInfo, dataInfo.dataDims.value());
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists);
        }

        template<typename DataType>
        DsetInfo writeDataset(const DataType &data, const Options &options) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            options.assertWellDefined();
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            auto dsetInfo = h5pp::scan::inferDsetInfo(
                openFileHandle(), data, options, plists); // Creates if it doesn't exist, otherwise it just fills the meta data
            writeDataset(data, dataInfo, dsetInfo);
            return dsetInfo;
        }

        template<typename DataType>
        DsetInfo writeDataset(
            const DataType &            data,                    /*!< Eigen, stl-like object or pointer to data buffer */
            std::string_view            dsetPath,                /*!< Path to HDF5 dataset relative to the file root */
            const OptDimsType &         dataDims = std::nullopt, /*!< Data dimensions hint. Required for pointer data */
            std::optional<H5D_layout_t> h5Layout =
                std::nullopt, /*!< (On create) Layout of dataset. Choose between H5D_CHUNKED,H5D_COMPACT and H5D_CONTIGUOUS */
            const OptDimsType &dsetDimsChunk = std::nullopt, /*!< (On create) Chunking dimensions. Only valid for H5D_CHUNKED datasets */
            const OptDimsType &dsetDimsMax   = std::nullopt, /*!< (On create) Maximum dimensions. Only valid for H5D_CHUNKED datasets */
            std::optional<hid::h5t>   h5Type = std::nullopt, /*!< (On create) Type of dataset. Override automatic type detection. */
            std::optional<ResizeMode> resizeMode =
                std::nullopt, /*!< Type of resizing if needed. Choose INCREASE_ONLY, RESIZE_TO_FIT,DO_NOT_RESIZE */
            std::optional<unsigned int> compression =
                std::nullopt) /*!< (On create) Compression level 0-9, 0 = off, 9 is gives best compression and is slowest */
        {
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetDimsChunk = dsetDimsChunk;
            options.dsetDimsMax   = dsetDimsMax;
            options.h5Layout      = h5Layout;
            options.h5Type        = std::move(h5Type);
            options.resizeMode    = resizeMode;
            options.compression   = getCompressionLevel(compression);
            return writeDataset(data, options);
        }

        template<typename DataType>
        DsetInfo writeDataset(
            const DataType &            data,                    /*!< Eigen, stl-like object or pointer to data buffer */
            std::string_view            dsetPath,                /*!< Path to HDF5 dataset relative to the file root */
            hid::h5t &                  h5Type,                  /*!< (On create) Type of dataset. Override automatic type detection. */
            const OptDimsType &         dataDims = std::nullopt, /*!< Data dimensions hint. Required for pointer data */
            std::optional<H5D_layout_t> h5Layout =
                std::nullopt, /*!< (On create) Layout of dataset. Choose between H5D_CHUNKED,H5D_COMPACT and H5D_CONTIGUOUS */
            const OptDimsType &dsetDimsChunk = std::nullopt, /*!< (On create) Chunking dimensions. Only valid for H5D_CHUNKED datasets */
            const OptDimsType &dsetDimsMax   = std::nullopt, /*!< (On create) Maximum dimensions. Only valid for H5D_CHUNKED datasets */
            std::optional<ResizeMode> resizeMode =
                std::nullopt, /*!< Type of resizing if needed. Choose INCREASE_ONLY, RESIZE_TO_FIT,DO_NOT_RESIZE */
            std::optional<unsigned int> compression =
                std::nullopt) /*!< (On create) Compression level 0-9, 0 = off, 9 is gives best compression and is slowest */
        {
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetDimsChunk = dsetDimsChunk;
            options.dsetDimsMax   = dsetDimsMax;
            options.h5Layout      = h5Layout;
            options.h5Type        = h5Type;
            options.resizeMode    = resizeMode;
            options.compression   = getCompressionLevel(compression);
            return writeDataset(data, options);
        }

        template<typename DataType>
        DsetInfo writeDataset(
            const DataType &   data,     /*!< Eigen, stl-like object or pointer to data buffer */
            std::string_view   dsetPath, /*!< Path to HDF5 dataset relative to the file root */
            H5D_layout_t       h5Layout, /*!< (On create) Layout of dataset. Choose between H5D_CHUNKED,H5D_COMPACT and H5D_CONTIGUOUS */
            const OptDimsType &dataDims      = std::nullopt, /*!< Data dimensions hint. Required for pointer data */
            const OptDimsType &dsetDimsChunk = std::nullopt, /*!< (On create) Chunking dimensions. Only valid for H5D_CHUNKED datasets */
            const OptDimsType &dsetDimsMax   = std::nullopt, /*!< (On create) Maximum dimensions. Only valid for H5D_CHUNKED datasets */
            std::optional<hid::h5t>   h5Type = std::nullopt, /*!< (On create) Type of dataset. Override automatic type detection. */
            std::optional<ResizeMode> resizeMode =
                std::nullopt, /*!< Type of resizing if needed. Choose INCREASE_ONLY, RESIZE_TO_FIT,DO_NOT_RESIZE */
            std::optional<unsigned int> compression =
                std::nullopt) /*!< (On create) Compression level 0-9, 0 = off, 9 is gives best compression and is slowest */
        {
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetDimsChunk = dsetDimsChunk;
            options.dsetDimsMax   = dsetDimsMax;
            options.h5Layout      = h5Layout;
            options.h5Type        = std::move(h5Type);
            options.resizeMode    = resizeMode;
            options.compression   = getCompressionLevel(compression);
            return writeDataset(data, options);
        }

        template<typename DataType>
        DsetInfo writeDataset_compact(const DataType &        data,
                                      std::string_view        dsetPath,
                                      const OptDimsType &     dataDims = std::nullopt,
                                      std::optional<hid::h5t> h5Type   = std::nullopt) {
            Options options;
            options.linkPath    = dsetPath;
            options.dataDims    = dataDims;
            options.h5Layout    = H5D_COMPACT;
            options.h5Type      = std::move(h5Type);
            options.compression = 0;
            return writeDataset(data, options);
        }

        template<typename DataType>
        DsetInfo writeDataset_contiguous(const DataType &        data,
                                         std::string_view        dsetPath,
                                         const OptDimsType &     dataDims = std::nullopt,
                                         std::optional<hid::h5t> h5Type   = std::nullopt) {
            Options options; // Get optional iterable should have three different return states, nullopt, empty or nonempty, ´,
            options.linkPath    = dsetPath;
            options.dataDims    = dataDims;
            options.h5Layout    = H5D_CONTIGUOUS;
            options.h5Type      = std::move(h5Type);
            options.compression = 0;
            return writeDataset(data, options);
        }

        template<typename DataType>
        DsetInfo writeDataset_chunked(const DataType &            data,
                                      std::string_view            dsetPath,
                                      const OptDimsType &         dataDims      = std::nullopt,
                                      const OptDimsType &         dsetDimsChunk = std::nullopt,
                                      const OptDimsType &         dsetDimsMax   = std::nullopt,
                                      std::optional<hid::h5t>     h5Type        = std::nullopt,
                                      std::optional<unsigned int> compression   = std::nullopt) {
            Options options; // Get optional iterable should have three different return states, nullopt, empty or nonempty, ´,
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetDimsChunk = dsetDimsChunk;
            options.dsetDimsMax   = dsetDimsMax;
            options.h5Layout      = H5D_CHUNKED;
            options.h5Type        = std::move(h5Type);
            options.compression   = getCompressionLevel(compression);
            return writeDataset(data, options);
        }

        template<typename DataType>
        DsetInfo writeDataset_compressed(const DataType &data, std::string_view dsetPath, std::optional<unsigned int> compression = 3) {
            Options options; // Get optional iterable should have three different return states, nullopt, empty or nonempty, ´,
            options.linkPath    = dsetPath;
            options.h5Layout    = H5D_CHUNKED;
            options.compression = getCompressionLevel(compression);
            return writeDataset(data, options);
        }

        void writeSymbolicLink(std::string_view src_path, std::string_view tgt_path) {
            hid::h5f file = openFileHandle();
            h5pp::hdf5::writeSymbolicLink(file, src_path, tgt_path, plists);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readDataset(DataType &data, const DataInfo &dataInfo, const DsetInfo &dsetInfo) const {
            h5pp::hdf5::readDataset(data, dataInfo, dsetInfo, plists);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        DataType readDataset(const DataInfo &dataInfo, const DsetInfo &dsetInfo) const {
            DataType data;
            readDataset(data, dataInfo, dsetInfo);
            return data;
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readDataset(DataType &data, const DsetInfo &dsetInfo, const Options &options = Options()) const {
            h5pp::hdf5::resizeData(data, dsetInfo);
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            readDataset(data, dataInfo, dsetInfo);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        DataType readDataset(const DsetInfo &dsetInfo, const Options &options = Options()) const {
            DataType data;
            readDataset(data, dsetInfo, options);
            return data;
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        DataType readDataset(const DsetInfo &dsetInfo, const DimsType &dataDims) const {
            DataType data;
            Options  options;
            options.dataDims = dataDims;
            readDataset(data, dsetInfo, options);
            return data;
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readDataset(DataType &data, const Options &options) const {
            options.assertWellDefined();
            auto dsetInfo = h5pp::scan::readDsetInfo(openFileHandle(), options, plists);
            if(dsetInfo.dsetExists and not dsetInfo.dsetExists.value())
                throw std::runtime_error(h5pp::format("Cannot read dataset [{}]: It does not exist", options.linkPath.value()));

            h5pp::hdf5::resizeData(data, dsetInfo);
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            h5pp::hdf5::readDataset(data, dataInfo, dsetInfo, plists);
        }

        template<typename DataType>
        void readDataset(DataType &data, std::string_view dsetPath, const OptDimsType &dataDims = std::nullopt) const {
            Options options;
            options.linkPath = dsetPath;
            options.dataDims = dataDims;
            readDataset(data, options);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        DataType readDataset(std::string_view datasetPath, const OptDimsType &dataDims = std::nullopt) const {
            DataType data;
            readDataset(data, datasetPath, dataDims);
            return data;
        }

        template<typename DataType>
        void appendToDataset(DataType &data, const DataInfo &dataInfo, DsetInfo &dsetInfo, size_t axis) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            h5pp::hdf5::extendDataset(dsetInfo, dataInfo, axis);
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists);
        }

        template<typename DataType>
        void appendToDataset(DataType &data, DsetInfo &dsetInfo, size_t axis, const OptDimsType &dataDims = std::nullopt) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            Options options;
            options.dataDims = dataDims;
            auto dataInfo    = h5pp::scan::scanDataInfo(data, options);
            appendToDataset(data, dataInfo, dsetInfo, axis);
        }

        template<typename DataType>
        DsetInfo appendToDataset(DataType &data, size_t axis, const Options &options = Options()) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            options.assertWellDefined();
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            auto dsetInfo = h5pp::scan::readDsetInfo(openFileHandle(), options, plists);
            appendToDataset(data, dataInfo, dsetInfo, axis);
            return dsetInfo;
        }

        template<typename DataType>
        DsetInfo appendToDataset(DataType &data, std::string_view dsetPath, size_t axis, const OptDimsType &dataDims = std::nullopt) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            Options options;
            options.linkPath = dsetPath;
            options.dataDims = dataDims;
            return appendToDataset(data, axis, options);
        }

        /*
         *
         * Functions related to attributes
         *
         */

        void createAttribute(AttrInfo &attrInfo) { h5pp::hdf5::createAttribute(attrInfo); }

        template<typename DataType>
        AttrInfo createAttribute(const DataType &data, AttrInfo &attrInfo, const Options &options = Options()) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to create attribute on read-only file [{}]", filePath.string()));
            if(attrInfo.hasLocId())
                h5pp::scan::inferAttrInfo(attrInfo, attrInfo.getLocId(), data, options, plists);
            else
                h5pp::scan::inferAttrInfo(attrInfo, openFileHandle(), data, options, plists);

            h5pp::hdf5::createAttribute(attrInfo);
            return attrInfo;
        }

        template<typename DataType>
        AttrInfo createAttribute(const DataType &data, const Options &options) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to create attribute on read-only file [{}]", filePath.string()));
            auto attrInfo = h5pp::scan::inferAttrInfo(openFileHandle(), data, options, plists);
            h5pp::hdf5::createAttribute(attrInfo);
            return attrInfo;
        }

        template<typename DataType>
        void createAttribute(const DataType &data, const DimsType &dataDims, std::string_view attrName, std::string_view linkPath) {
            Options options;
            options.linkPath = linkPath;
            options.attrName = attrName;
            options.dataDims = dataDims;
            createAttribute(data, options);
        }

        template<typename DataType>
        void writeAttribute(const DataType &data, DataInfo &dataInfo, AttrInfo &attrInfo, const Options &options = Options()) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            if(attrInfo.hasLocId())
                h5pp::scan::inferAttrInfo(attrInfo, attrInfo.getLocId(), data, options, plists);
            else
                h5pp::scan::inferAttrInfo(attrInfo, openFileHandle(), data, options, plists);
            h5pp::scan::scanDataInfo(dataInfo, data, options);
            h5pp::hdf5::writeAttribute(data, dataInfo, attrInfo);
        }

        template<typename DataType>
        void writeAttribute(const DataType &data, const Options &options) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            options.assertWellDefined();
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            auto attrInfo = createAttribute(data, options);
            h5pp::hdf5::writeAttribute(data, dataInfo, attrInfo);
        }

        template<typename DataType>
        void writeAttribute(const DataType &        data,
                            std::string_view        attrName,
                            std::string_view        linkPath,
                            const OptDimsType &     dataDims = std::nullopt,
                            std::optional<hid::h5t> h5Type   = std::nullopt) {
            Options options;
            options.linkPath = linkPath;
            options.attrName = attrName;
            options.dataDims = dataDims;
            options.h5Type   = std::move(h5Type);
            writeAttribute(data, options);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readAttribute(DataType &data, const h5pp::AttrInfo &attrInfo, const Options &options = Options()) const {
            if(attrInfo.linkExists and not attrInfo.linkExists.value())
                throw std::runtime_error(h5pp::format("Could not read attribute [{}] in link [{}]: "
                                                      "Link does not exist",
                                                      attrInfo.attrName.value(),
                                                      attrInfo.linkPath.value()));

            if(attrInfo.attrExists and not attrInfo.attrExists.value())
                throw std::runtime_error(h5pp::format("Could not read attribute [{}] in link [{}]: "
                                                      "Attribute does not exist",
                                                      attrInfo.attrName.value(),
                                                      attrInfo.linkPath.value()));

            h5pp::hdf5::resizeData(data, attrInfo);
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            h5pp::hdf5::readAttribute(data, dataInfo, attrInfo);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readAttribute(DataType &data, const Options &options) const {
            options.assertWellDefined();
            auto attrInfo = h5pp::scan::readAttrInfo(openFileHandle(), options, plists);
            readAttribute(data, attrInfo, options);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readAttribute(DataType &         data,
                           std::string_view   attrName,
                           std::string_view   linkPath,
                           const OptDimsType &dataDims = std::nullopt) const {
            Options options;
            options.linkPath = linkPath;
            options.attrName = attrName;
            options.dataDims = dataDims;
            readAttribute(data, options);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        [[nodiscard]] DataType
            readAttribute(std::string_view attrName, std::string_view linkPath, const OptDimsType &dataDims = std::nullopt) const {
            DataType data;
            readAttribute(data, attrName, linkPath, dataDims);
            return data;
        }

        [[nodiscard]] inline std::vector<std::string> getAttributeNames(std::string_view linkPath) const {
            return h5pp::hdf5::getAttributeNames(openFileHandle(), linkPath, std::nullopt, plists.linkAccess);
        }

        /*
         *
         *
         * Functions related to tables
         *
         *
         */

        TableInfo createTable(const hid::h5t &                  h5Type,
                              std::string_view                  tablePath,
                              std::string_view                  tableTitle,
                              const OptDimsType &               chunkDims        = std::nullopt,
                              const std::optional<unsigned int> compressionLevel = std::nullopt

        ) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            Options options;
            options.linkPath      = h5pp::util::safe_str(tablePath);
            options.h5Type        = h5Type;
            options.dsetDimsChunk = chunkDims;
            options.compression   = compressionLevel;
            auto tableInfo        = h5pp::scan::getTableInfo(openFileHandle(), options, tableTitle, plists);
            h5pp::hdf5::createTable(tableInfo, plists);
            h5pp::scan::readTableInfo(tableInfo, tableInfo.getLocId(), options, plists);
            return tableInfo;
        }

        template<typename DataType>
        TableInfo writeTableRecords(const DataType &data, std::string_view tablePath, hsize_t startIdx = 0) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            auto info        = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            if(not info.tableExists.value())
                throw std::runtime_error(h5pp::format("Cannot write to table [{}]: it does not exist", tablePath));
            h5pp::hdf5::writeTableRecords(data, info, startIdx);
            return info;
        }

        template<typename DataType>
        TableInfo appendTableRecords(const DataType &data, std::string_view tablePath) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            auto info        = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            if(not info.tableExists.value())
                throw std::runtime_error(h5pp::format("Cannot append to table [{}]: it does not exist", tablePath));
            h5pp::hdf5::appendTableRecords(data, info);
            return info;
        }

        void appendTableRecords(const h5pp::TableInfo &srcInfo, hsize_t srcStartIdx, hsize_t numRecordsToAppend, h5pp::TableInfo &tgtInfo) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            if(not tgtInfo.numRecords)
                throw std::runtime_error(h5pp::format("Cannot append records to table: Target TableInfo has undefined field [numRecords]"));
            hsize_t tgtStartIdx = tgtInfo.numRecords.value();
            numRecordsToAppend  = std::min(srcInfo.numRecords.value() - srcStartIdx, numRecordsToAppend);
            h5pp::hdf5::copyTableRecords(srcInfo, srcStartIdx, numRecordsToAppend, tgtInfo, tgtStartIdx);
        }

        void appendTableRecords(const h5pp::TableInfo &srcInfo, TableSelection srcTableSelection, h5pp::TableInfo &tgtInfo) {
            if(not tgtInfo.numRecords)
                throw std::runtime_error("Cannot append records to table: Target TableInfo has undefined field [numRecords]");
            copyTableRecords(srcInfo, srcTableSelection, tgtInfo, tgtInfo.numRecords.value());
        }

        TableInfo appendTableRecords(const h5pp::TableInfo &           srcInfo,
                                     TableSelection                    srcTableSelection,
                                     std::string_view                  tgtTablePath,
                                     const OptDimsType &               chunkDims        = std::nullopt,
                                     const std::optional<unsigned int> compressionLevel = std::nullopt) {
            Options options;
            options.linkPath      = h5pp::util::safe_str(tgtTablePath);
            options.dsetDimsChunk = chunkDims;
            options.compression   = compressionLevel;
            auto tgtInfo          = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            if(not tgtInfo.tableExists or not tgtInfo.tableExists.value())
                tgtInfo =
                    createTable(srcInfo.h5Type.value(), tgtInfo.tablePath.value(), srcInfo.tableTitle.value(), chunkDims, compressionLevel);

            appendTableRecords(srcInfo, srcTableSelection, tgtInfo);
            return tgtInfo;
        }

        template<typename h5x_src, typename = h5pp::type::sfinae::enable_if_is_h5_loc_t<h5x_src>>
        TableInfo appendTableRecords(const h5x_src &                   srcLocation,
                                     std::string_view                  srcTablePath,
                                     TableSelection                    srcTableSelection,
                                     std::string_view                  tgtTablePath,
                                     const OptDimsType &               chunkDims        = std::nullopt,
                                     const std::optional<unsigned int> compressionLevel = std::nullopt) {
            Options options;
            options.linkPath = h5pp::util::safe_str(srcTablePath);
            auto srcInfo     = h5pp::scan::readTableInfo(srcLocation, options, plists);
            return appendTableRecords(srcInfo, srcTableSelection, tgtTablePath, chunkDims, compressionLevel);
        }

        void copyTableRecords(const h5pp::TableInfo &srcInfo,
                              hsize_t                srcStartIdx,
                              hsize_t                numRecordsToCopy,
                              h5pp::TableInfo &      tgtInfo,
                              hsize_t                tgtStartIdx) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            if(not srcInfo.numRecords) throw std::runtime_error("Source TableInfo has undefined field [numRecords]");
            numRecordsToCopy = std::min(srcInfo.numRecords.value() - srcStartIdx, numRecordsToCopy);
            h5pp::hdf5::copyTableRecords(srcInfo, srcStartIdx, numRecordsToCopy, tgtInfo, tgtStartIdx);
        }

        void copyTableRecords(const h5pp::TableInfo &srcInfo,
                              TableSelection         srcTableSelection,
                              h5pp::TableInfo &      tgtInfo,
                              hsize_t                tgtStartIdx) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            if(not srcInfo.numRecords)
                throw std::runtime_error("Cannot append records from table: Source TableInfo has undefined field [numRecords]");
            if(not tgtInfo.numRecords)
                throw std::runtime_error("Cannot append records to table: Target TableInfo has undefined field [numRecords]");
            if(srcInfo.numRecords.value() == 0) throw std::runtime_error("Cannot append records from table: Source table is empty");
            hsize_t srcStartIdx        = 0;
            hsize_t numRecordsToAppend = 0;
            switch(srcTableSelection) {
                case h5pp::TableSelection::ALL: numRecordsToAppend = srcInfo.numRecords.value(); break;
                case h5pp::TableSelection::FIRST: numRecordsToAppend = 1; break;
                case h5pp::TableSelection::LAST:
                    numRecordsToAppend = 1;
                    srcStartIdx        = srcInfo.numRecords.value() - 1;
                    break;
            }
            h5pp::hdf5::copyTableRecords(srcInfo, srcStartIdx, numRecordsToAppend, tgtInfo, tgtStartIdx);
        }

        TableInfo copyTableRecords(const h5pp::TableInfo &           srcInfo,
                                   TableSelection                    tableSelection,
                                   std::string_view                  tgtTablePath,
                                   hsize_t                           tgtStartIdx,
                                   const OptDimsType &               chunkDims        = std::nullopt,
                                   const std::optional<unsigned int> compressionLevel = std::nullopt) {
            Options options;
            options.linkPath      = h5pp::util::safe_str(tgtTablePath);
            options.dsetDimsChunk = chunkDims;
            options.compression   = compressionLevel;
            auto tgtInfo          = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            if(not tgtInfo.tableExists or not tgtInfo.tableExists.value())
                tgtInfo =
                    createTable(srcInfo.h5Type.value(), tgtInfo.tablePath.value(), srcInfo.tableTitle.value(), chunkDims, compressionLevel);

            copyTableRecords(srcInfo, tableSelection, tgtInfo, tgtStartIdx);
            return tgtInfo;
        }

        TableInfo copyTableRecords(const h5pp::TableInfo &           srcInfo,
                                   hsize_t                           srcStartIdx,
                                   hsize_t                           numRecordsToCopy,
                                   std::string_view                  tgtTablePath,
                                   hsize_t                           tgtStartIdx,
                                   const OptDimsType &               chunkDims   = std::nullopt,
                                   const std::optional<unsigned int> compression = std::nullopt) {
            Options options;
            options.linkPath      = h5pp::util::safe_str(tgtTablePath);
            options.dsetDimsChunk = chunkDims;
            options.compression   = getCompressionLevel(compression);
            auto tgtInfo          = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            if(not tgtInfo.tableExists or not tgtInfo.tableExists.value())
                tgtInfo =
                    createTable(srcInfo.h5Type.value(), tgtInfo.tablePath.value(), srcInfo.tableTitle.value(), chunkDims, compression);

            copyTableRecords(srcInfo, srcStartIdx, numRecordsToCopy, tgtInfo, tgtStartIdx);
            return tgtInfo;
        }

        template<typename h5x_src, typename = h5pp::type::sfinae::enable_if_is_h5_loc_t<h5x_src>>
        TableInfo copyTableRecords(const h5x_src &                   srcLocation,
                                   std::string_view                  srcTablePath,
                                   TableSelection                    srcTableSelection,
                                   std::string_view                  tgtTablePath,
                                   hsize_t                           tgtStartIdx,
                                   const std::optional<hsize_t>      chunkDims   = std::nullopt,
                                   const std::optional<unsigned int> compression = std::nullopt) {
            Options options;
            options.linkPath = h5pp::util::safe_str(srcTablePath);
            auto srcInfo     = h5pp::scan::readTableInfo(srcLocation, options, plists);
            return copyTableRecords(srcInfo, srcTableSelection, tgtTablePath, tgtStartIdx, chunkDims, compression);
        }

        template<typename h5x_src, typename = h5pp::type::sfinae::enable_if_is_h5_loc_t<h5x_src>>
        TableInfo copyTableRecords(const h5x_src &                   srcLocation,
                                   std::string_view                  srcTablePath,
                                   hsize_t                           srcStartIdx,
                                   hsize_t                           numRecordsToCopy,
                                   std::string_view                  tgtTablePath,
                                   hsize_t                           tgtStartIdx,
                                   const std::optional<hsize_t>      chunkDims        = std::nullopt,
                                   const std::optional<unsigned int> compressionLevel = std::nullopt) {
            Options options;
            options.linkPath = h5pp::util::safe_str(srcTablePath);
            auto srcInfo     = h5pp::scan::readTableInfo(srcLocation, options, plists);
            return copyTableRecords(srcInfo, srcStartIdx, numRecordsToCopy, tgtTablePath, tgtStartIdx, chunkDims, compressionLevel);
        }

        TableInfo copyTableRecords(std::string_view                  srcTablePath,
                                   hsize_t                           srcStartIdx,
                                   hsize_t                           numRecords,
                                   std::string_view                  tgtTablePath,
                                   hsize_t                           tgtStartIdx,
                                   const OptDimsType &               chunkDims        = std::nullopt,
                                   const std::optional<unsigned int> compressionLevel = std::nullopt) {
            Options tgt_options, src_options;
            src_options.linkPath      = h5pp::util::safe_str(srcTablePath);
            tgt_options.linkPath      = h5pp::util::safe_str(tgtTablePath);
            tgt_options.dsetDimsChunk = chunkDims;
            tgt_options.compression   = compressionLevel;
            auto srcInfo              = h5pp::scan::readTableInfo(openFileHandle(), src_options, plists);
            auto tgtInfo              = h5pp::scan::readTableInfo(openFileHandle(), tgt_options, plists);
            if(not tgtInfo.tableExists or not tgtInfo.tableExists.value())
                tgtInfo =
                    createTable(srcInfo.h5Type.value(), tgtInfo.tablePath.value(), srcInfo.tableTitle.value(), chunkDims, compressionLevel);
            copyTableRecords(srcInfo, srcStartIdx, numRecords, tgtInfo, tgtStartIdx);
            return tgtInfo;
        }

        template<typename DataType>
        void readTableRecords(DataType &            data,
                              std::string_view      tablePath,
                              std::optional<size_t> startIdx   = std::nullopt,
                              std::optional<size_t> numRecords = std::nullopt) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            auto info        = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            if(info.tableExists and not info.tableExists.value())
                throw std::runtime_error(
                    h5pp::format("Could not read records from table [{}]: it does not exist", util::safe_str(tablePath)));
            h5pp::hdf5::readTableRecords(data, info, startIdx, numRecords);
        }

        template<typename DataType>
        void readTableRecords(DataType &data, std::string_view tablePath, h5pp::TableSelection tableSelection) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            auto info        = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            if(info.tableExists and not info.tableExists.value())
                throw std::runtime_error(
                    h5pp::format("Could not read records from table [{}]: it does not exist", util::safe_str(tablePath)));

            hsize_t startIdx   = 0;
            hsize_t numRecords = 0;

            switch(tableSelection) {
                case h5pp::TableSelection::ALL:
                    startIdx   = 0;
                    numRecords = info.numRecords.value();
                    break;
                case h5pp::TableSelection::FIRST:
                    startIdx   = 0;
                    numRecords = 1;
                    break;
                case h5pp::TableSelection::LAST:
                    startIdx   = info.numRecords.value() - 1;
                    numRecords = 1;
                    break;
            }
            h5pp::hdf5::readTableRecords(data, info, startIdx, numRecords);
        }

        template<typename DataType>
        DataType readTableRecords(std::string_view      tablePath,
                                  std::optional<size_t> startIdx   = std::nullopt,
                                  std::optional<size_t> numRecords = std::nullopt) const {
            DataType data;
            readTableRecords(data, tablePath, startIdx, numRecords);
            return data;
        }

        template<typename DataType>
        DataType readTableRecords(std::string_view tablePath, h5pp::TableSelection tableSelection) const {
            DataType data;
            readTableRecords(data, tablePath, tableSelection);
            return data;
        }

        template<typename DataType>
        void readTableField(DataType &            data,
                            const TableInfo &     info,
                            NamesOrIndices &&     fieldNamesOrIndices,
                            std::optional<size_t> startIdx   = std::nullopt,
                            std::optional<size_t> numRecords = std::nullopt) const {
            auto variant_index = fieldNamesOrIndices.index();
            if(variant_index == 0)
                return h5pp::hdf5::readTableField(data, info, fieldNamesOrIndices.get_value<0>(), startIdx, numRecords);
            else if(variant_index == 1)
                return h5pp::hdf5::readTableField(data, info, fieldNamesOrIndices.get_value<1>(), startIdx, numRecords);
            else
                throw std::runtime_error("No field names or indices have been specified");
        }

        template<typename DataType>
        void readTableField(DataType &            data,
                            std::string_view      tablePath,
                            NamesOrIndices &&     fieldNamesOrIndices,
                            std::optional<size_t> startIdx   = std::nullopt,
                            std::optional<size_t> numRecords = std::nullopt) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            auto info        = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            readTableField(data, info, std::forward<NamesOrIndices>(fieldNamesOrIndices), startIdx, numRecords);
        }

        template<typename DataType>
        DataType readTableField(std::string_view      tablePath,
                                NamesOrIndices &&     fieldNamesOrIndices,
                                std::optional<size_t> startIdx   = std::nullopt,
                                std::optional<size_t> numRecords = std::nullopt) const {
            DataType data;
            readTableField(data, tablePath, std::forward<NamesOrIndices>(fieldNamesOrIndices), startIdx, numRecords);
            return data;
        }

        template<typename DataType>
        void readTableField(DataType &       data,
                            std::string_view tablePath,
                            NamesOrIndices &&fieldNamesOrIndices,
                            TableSelection   tableSelection) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            auto info        = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            if(info.tableExists and not info.tableExists.value())
                throw std::runtime_error(
                    h5pp::format("Could not read records from table [{}]: it does not exist", util::safe_str(tablePath)));

            hsize_t startIdx   = 0;
            hsize_t numRecords = 0;
            switch(tableSelection) {
                case h5pp::TableSelection::ALL:
                    startIdx   = 0;
                    numRecords = info.numRecords.value();
                    break;
                case h5pp::TableSelection::FIRST:
                    startIdx   = 0;
                    numRecords = 1;
                    break;
                case h5pp::TableSelection::LAST:
                    startIdx   = info.numRecords.value() - 1;
                    numRecords = 1;
                    break;
            }
            readTableField(data, info, std::forward<NamesOrIndices>(fieldNamesOrIndices), startIdx, numRecords);
        }

        template<typename DataType>
        DataType readTableField(std::string_view tablePath, NamesOrIndices &&fieldNamesOrIndices, TableSelection tableSelection) const {
            DataType data;
            readTableField(data, tablePath, std::forward<NamesOrIndices>(fieldNamesOrIndices), tableSelection);
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
            auto dataset = h5pp::hdf5::openLink<hid::h5d>(openFileHandle(), datasetPath);
            return h5pp::hdf5::getRank(dataset);
        }

        [[nodiscard]] std::vector<hsize_t> getDatasetDimensions(std::string_view datasetPath) const {
            auto dataset = h5pp::hdf5::openLink<hid::h5d>(openFileHandle(), datasetPath);
            return h5pp::hdf5::getDimensions(dataset);
        }
        [[nodiscard]] std::optional<std::vector<hsize_t>> getDatasetMaxDimensions(std::string_view datasetPath) const {
            auto dataset = h5pp::hdf5::openLink<hid::h5d>(openFileHandle(), datasetPath);
            return h5pp::hdf5::getMaxDimensions(dataset);
        }

        [[nodiscard]] std::optional<std::vector<hsize_t>> getDatasetChunkDimensions(std::string_view datasetPath) const {
            auto dataset = h5pp::hdf5::openLink<hid::h5d>(openFileHandle(), datasetPath);
            return h5pp::hdf5::getChunkDimensions(dataset);
        }

        [[nodiscard]] bool linkExists(std::string_view link) const {
            return h5pp::hdf5::checkIfLinkExists(openFileHandle(), link, std::nullopt, plists.linkAccess);
        }

        [[nodiscard]] std::vector<std::string>
            findLinks(std::string_view searchKey = "", std::string_view searchRoot = "/", long maxHits = -1, long maxDepth = -1) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_UNKNOWN>(openFileHandle(), searchKey, searchRoot, maxHits, maxDepth, plists.linkAccess);
        }

        [[nodiscard]] std::vector<std::string>
            findDatasets(std::string_view searchKey = "", std::string_view searchRoot = "/", long maxHits = -1, long maxDepth = -1) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_DATASET>(openFileHandle(), searchKey, searchRoot, maxHits, maxDepth, plists.linkAccess);
        }

        [[nodiscard]] std::vector<std::string>
            findGroups(std::string_view searchKey = "", std::string_view searchRoot = "/", long maxHits = -1, long maxDepth = -1) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_GROUP>(openFileHandle(), searchKey, searchRoot, maxHits, maxDepth, plists.linkAccess);
        }

        [[nodiscard]] DsetInfo getDatasetInfo(std::string_view dsetPath) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(dsetPath);
            return h5pp::scan::readDsetInfo(openFileHandle(), options, plists);
        }

        [[nodiscard]] TableInfo getTableInfo(std::string_view tablePath) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            return h5pp::scan::readTableInfo(openFileHandle(), options, plists);
        }

        [[nodiscard]] TypeInfo getTypeInfoDataset(std::string_view dsetPath) const {
            return h5pp::hdf5::getTypeInfo(openFileHandle(), dsetPath, std::nullopt, plists.linkAccess);
        }

        [[nodiscard]] AttrInfo getAttributeInfo(std::string_view linkPath, std::string_view attrName) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(linkPath);
            options.attrName = h5pp::util::safe_str(attrName);
            return h5pp::scan::readAttrInfo(openFileHandle(), options, plists);
        }

        [[nodiscard]] TypeInfo getTypeInfoAttribute(std::string_view linkPath, std::string_view attrName) const {
            return h5pp::hdf5::getTypeInfo(openFileHandle(), linkPath, attrName, std::nullopt, std::nullopt, plists.linkAccess);
        }

        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes(std::string_view linkPath) const {
            return h5pp::hdf5::getTypeInfo_allAttributes(openFileHandle(), linkPath, std::nullopt, plists.linkAccess);
        }

        template<typename InfoType>
        [[nodiscard]] InfoType getInfo(std::string_view linkPath) const {
            if constexpr(std::is_same_v<InfoType, DsetInfo>)
                return getDatasetInfo(linkPath);
            else if constexpr(std::is_same_v<InfoType, TableInfo>)
                return getTableInfo(linkPath);
            else if constexpr(std::is_same_v<InfoType, TypeInfo>)
                return getTypeInfoDataset(linkPath);
            else
                static_assert(h5pp::type::sfinae::invalid_type_v<InfoType>,
                              "Template function [h5pp::File::getInfo<InfoType>(std::string_view linkPath)] "
                              "requires InfoType: [h5pp::DsetInfo], [h5pp::TableInfo] or [h5pp::TypeInfo]");
        }

        template<typename InfoType>
        [[nodiscard]] InfoType getInfo(std::string_view linkPath, std::string_view attrName) const {
            if constexpr(std::is_same_v<InfoType, AttrInfo>)
                return getAttributeInfo(linkPath, attrName);
            else if constexpr(std::is_same_v<InfoType, TypeInfo>)
                return getTypeInfoAttribute(linkPath, attrName);
            else
                static_assert(h5pp::type::sfinae::invalid_type_v<InfoType>,
                              "Template function [h5pp::File::getInfo<InfoType>(std::string_view linkPath, std::string_view attrName)] "
                              "requires InfoType: [h5pp::AttrInfo] or [h5pp::TypeInfo]");
        }

        [[nodiscard]] bool fileIsValid() const { return h5pp::hdf5::fileIsValid(filePath); }
    };
}
