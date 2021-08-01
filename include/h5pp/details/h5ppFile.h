
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
#include "h5ppUtils.h"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <string>
#include <utility>

/*! \namespace h5pp
 * \brief A simple C++17 wrapper for the HDF5 library
 */
namespace h5pp {

    /*!
     \brief Writes and reads data to a binary hdf5-file.
    */

    class File {
        private:
        fs::path                        filePath;                                               /*!< Full path to the file */
        h5pp::FilePermission            permission              = h5pp::FilePermission::RENAME; /*!< File open/create policy. */
        mutable std::optional<hid::h5f> fileHandle              = std::nullopt; /*!< Keeps a file handle alive in batch operations */
        size_t                          logLevel                = 2;            /*!< Log verbosity from 0 [trace] to 6 [off] */
        bool                            logTimestamp            = false;        /*!< Add a time stamp to console log output */
        hid::h5e                        error_stack             = H5E_DEFAULT;  /*!< Holds a reference to the error stack used by HDF5 */
        unsigned int                    currentCompressionLevel = 0;            /*!< Holds the default compression level */

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

        explicit File(h5pp::fs::path       filePath_,                                    /*!< Path a new file */
                      h5pp::FilePermission permission_   = h5pp::FilePermission::RENAME, /*!< Set permission in case of file collision */
                      size_t               logLevel_     = 2,    /*!< Logging verbosity level 0 (most) to 6 (least). */
                      bool                 logTimestamp_ = false /*!< True prepends a timestamp to log output */
                      )
            : filePath(std::move(filePath_)), permission(permission_), logLevel(logLevel_), logTimestamp(logTimestamp_) {
            init();
        }

        explicit File(h5pp::fs::path filePath_,            /*!< Path a new file */
                      unsigned int   H5F_ACC_FLAGS,        /*!< Set HDF5 access flag for new files */
                      size_t         logLevel_     = 2,    /*!< Logging verbosity level 0 (most) to 6 (least). */
                      bool           logTimestamp_ = false /*!< True prepends a timestamp to log output */
                      )
            : filePath(std::move(filePath_)), logLevel(logLevel_), logTimestamp(logTimestamp_) {
            permission = h5pp::hdf5::convertFileAccessFlags(H5F_ACC_FLAGS);
            init();
        }

        /*! Flush the HDF5 internal file cache using H5Fflush */
        void flush() {
            H5Fflush(openFileHandle(), H5F_scope_t::H5F_SCOPE_GLOBAL);
            h5pp::logger::log->trace("Flushing caches");
            H5garbage_collect();
            H5Eprint(H5E_DEFAULT, stderr);
        }

        /*! Returns an HDF5 file handle
         *
         * - The file permission is set when initializing h5pp::File.
         * - Use `h5pp::setKeepFileOpened()` to keep a cached handle. Use `h5pp::setKeepFileClosed()` to close the cached handle.
         */
        [[nodiscard]] hid::h5f openFileHandle() const {
            h5pp::logger::setLogger("h5pp|" + filePath.filename().string(), logLevel, logTimestamp);
            if(fileHandle) return fileHandle.value();
            if(permission == h5pp::FilePermission::READONLY) {
                h5pp::logger::log->trace("Opening file in READONLY mode");
                hid_t fid = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, plists.fileAccess);
                if(fid < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error(h5pp::format("Failed to open file in read-only mode [{}]", filePath.string()));
                } else
                    return fid;
            } else {
                h5pp::logger::log->trace("Opening file in READWRITE mode");
                hid_t fid = H5Fopen(filePath.string().c_str(), H5F_ACC_RDWR, plists.fileAccess);
                if(fid < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error(h5pp::format("Failed to open file in read-write mode [{}]", filePath.string()));
                } else
                    return fid;
            }
        }

        /*
         *
         * Functions for file properties
         *
         */

        /*! Open and keep a cached file handle
         *
         * This is useful for quick batch operations where opening and closing the file handle would introduce a large performance penalty.
         */
        void setKeepFileOpened() const {
            // Check before setting onto self:
            // otherwise repeated calls to setKeepFileOpened() would increment the reference count,
            // without there existing a handle to decrement it --> memory leak
            if(not fileHandle) fileHandle = openFileHandle();
        }

        /*! Close a cached file handle if it exists */
        void setKeepFileClosed() const { fileHandle = std::nullopt; }

        /*! Gets the current file access permission */
        [[nodiscard]] h5pp::FilePermission getFilePermission() const { return permission; }

        /*! Gets the current file name */
        [[nodiscard]] std::string getFileName() const { return filePath.filename().string(); }

        /*! Gets the full path to the current file */
        [[nodiscard]] std::string getFilePath() const { return filePath.string(); }

        /*! Sets the default file access permission */
        void setFilePermission(h5pp::FilePermission permission_ /*!< Permission */
        ) {
            permission = permission_;
        }

        /*! Sets how HDF5 will close a file internally
         *
         * From the [HDF5 documentation](https://portal.hdfgroup.org/display/HDF5/H5P_SET_FCLOSE_DEGREE):
         *
         *  `H5F_close_degree_t`::
         *  - `H5F_CLOSE_DEFAULT` : Use the degree pre-defined by underlining VFL
         *  - `H5F_CLOSE_WEAK`    : file closes only after all opened objects are closed
         *  - `H5F_CLOSE_SEMI`    : if no opened objects, file is close; otherwise, file close fails
         *  - `H5F_CLOSE_STRONG`  : if there are opened objects, close them first, then close file
         * */
        void setCloseDegree(H5F_close_degree_t degree) {
            if(plists.fileAccess == H5P_DEFAULT) plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fclose_degree(plists.fileAccess, degree);
            if(fileHandle) { // Refresh if the filehandle being kept is open
                fileHandle = std::nullopt;
                fileHandle = openFileHandle();
            }
        }

        /*! Sets the HDF5 file driver to `H5FD_CORE`
         *
         * `H5FD_CORE`: This driver performs I/O directly to memory and can be used to create small temporary files
         *              that never exist on permanent storage.
         *
         */
        void setDriver_core(bool   writeOnClose   = false,   /*!< Optionally save to storage when the file is closed. */
                            size_t bytesPerMalloc = 10240000 /*!< Size, in bytes, of memory increments. */
        ) {
            if(plists.fileAccess == H5P_DEFAULT) plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fapl_core(plists.fileAccess, bytesPerMalloc, static_cast<hbool_t>(writeOnClose));
            if(fileHandle) { // Refresh if the filehandle being kept is open
                fileHandle = std::nullopt;
                fileHandle = openFileHandle();
            }
        }

        /*! Sets the HDF5 file driver to `H5FD_SEC2`
         *
         * `H5FD_SEC2` : This is the default driver which uses Posix file-system functions like read and write to
         *             perform I/O to a single file.
         */
        void setDriver_sec2() {
            if(plists.fileAccess == H5P_DEFAULT) plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fapl_sec2(plists.fileAccess);
            if(fileHandle) { // Refresh if the filehandle being kept is open
                fileHandle = std::nullopt;
                fileHandle = openFileHandle();
            }
        }

        /*! Sets the HDF5 file driver to `H5FD_STDIO`
         *
         * `H5FD_STDIO`: This driver uses functions from 'stdio.h' to perform buffered I/O to a single file.
         */
        void setDriver_stdio() {
            if(plists.fileAccess == H5P_DEFAULT) plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fapl_stdio(plists.fileAccess);
            if(fileHandle) { // Refresh if the filehandle being kept is open
                fileHandle = std::nullopt;
                fileHandle = openFileHandle();
            }
        }

#ifdef H5_HAVE_PARALLEL
        /*! Sets the HDF5 file driver to `H5FD_MPIO`
         *
         * `H5FD_MPIO`: Parallel files accessed via the MPI I/O layer.
         *              The standard HDF5 file driver for parallel file systems.
         */
        void setDriver_mpio(MPI_Comm comm, MPI_Info info) {
            plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fapl_mpio(plists.fileAccess, comm, info);
            if(fileHandle) { // Refresh if the filehandle being kept is open
                fileHandle = std::nullopt;
                fileHandle = openFileHandle();
            }
        }
#endif

        /*
         *
         * Functions for managing file location
         *
         */

        /*! Make a copy of this file at a different path.
         *
         * No change to the current file.
         */
        [[maybe_unused]] fs::path
            copyFileTo(const h5pp::fs::path &targetFilePath,                       /*!< Copy to this path */
                       const FilePermission &perm = FilePermission::COLLISION_FAIL /*!< File access permission at the new path */

            ) const {
            return h5pp::hdf5::copyFile(getFilePath(), targetFilePath, perm, plists);
        }

        /*! Move the current file to a new path.
         *
         * The current file is re-opened at the new path.
         */
        [[maybe_unused]] fs::path
            moveFileTo(const h5pp::fs::path &targetFilePath,                       /*!< The new path */
                       const FilePermission &perm = FilePermission::COLLISION_FAIL /*!< File access permission at the new path */
            ) {
            auto newPath = h5pp::hdf5::moveFile(getFilePath(), targetFilePath, perm, plists);
            if(fs::exists(newPath)) { filePath = newPath; }
            return newPath;
        }

        /*
         *
         * Functions for transferring contents
         *
         */

        /*! Copy a link (dataset/table/group) into another file. */
        void copyLinkToFile(std::string_view      localLinkPath,                   /*!< Path to link in this file */
                            const h5pp::fs::path &targetFilePath,                  /*!< Path to file to copy into */
                            std::string_view      targetLinkPath,                  /*!< Path to link in the target file  */
                            const FilePermission &perm = FilePermission::READWRITE /*!< File access permission at the target path */
        ) const {
            return h5pp::hdf5::copyLink(getFilePath(), localLinkPath, targetFilePath, targetLinkPath, perm, plists);
        }

        /*! Copy a link (dataset/table/group) from another file into this. */
        void copyLinkFromFile(std::string_view      localLinkPath,  /*!< Path to link in this file */
                              const h5pp::fs::path &sourceFilePath, /*!< Path to file to copy from */
                              std::string_view      sourceLinkPath  /*!< Path to link in the source file */
        ) {
            return h5pp::hdf5::copyLink(sourceFilePath,
                                        sourceLinkPath,
                                        getFilePath(),
                                        localLinkPath,
                                        h5pp::FilePermission::READWRITE,
                                        plists);
        }

        /*! Copy a link (dataset/table/group) from this file to any hid::h5x location (group or file). */
        template<typename h5x_tgt, typename = h5pp::type::sfinae::enable_if_is_h5_loc_t<h5x_tgt>>
        void copyLinkToLocation(std::string_view localLinkPath,    /*!< Path to link in this file */
                                const h5x_tgt   &targetLocationId, /*!< Target hid::h5x location handle (group or file) */
                                std::string_view targetLinkPath    /*!< Path to link in the target file */
        ) const {
            return h5pp::hdf5::copyLink(openFileHandle(), localLinkPath, targetLocationId, targetLinkPath, plists);
        }

        /*! Copy a link (dataset/table/group) from any hid::h5x location (group or file) into this file */
        template<typename h5x_src, typename = h5pp::type::sfinae::enable_if_is_h5_loc_t<h5x_src>>
        void copyLinkFromLocation(std::string_view localLinkPath,    /*!< Path to link in this file */
                                  const h5x_src   &sourceLocationId, /*!< Source hid::h5x location handle (group or file) */
                                  std::string_view sourceLinkPath    /*!< Path to link in the source file */
        ) {
            return h5pp::hdf5::copyLink(sourceLocationId, sourceLinkPath, openFileHandle(), localLinkPath, plists);
        }

        /*! Move a link (dataset/table/group) into another file.
         *
         *  **NOTE:** The link is deleted from this file, but the storage space is not recovered.
         *  This is a fundamental limitation of HDF5.
         */
        void moveLinkToFile(std::string_view      localLinkPath,                   /*!< Path to link in this file */
                            const h5pp::fs::path &targetFilePath,                  /*!< Path to file to move into */
                            std::string_view      targetLinkPath,                  /*!< Path to link in the target file  */
                            const FilePermission &perm = FilePermission::READWRITE /*!< File access permission at the target path */

        ) const {
            return h5pp::hdf5::moveLink(getFilePath(), localLinkPath, targetFilePath, targetLinkPath, perm, plists);
        }

        /*! Move a link (dataset/table/group) from another file into this.
         *
         *  **NOTE:** The link is deleted from the other file, but the storage space is not recovered.
         *  This is a fundamental limitation of HDF5.
         */
        void moveLinkFromFile(std::string_view      localLinkPath,  /*!< Path to link in this file */
                              const h5pp::fs::path &sourceFilePath, /*!< Path to file to copy from */
                              std::string_view      sourceLinkPath  /*!< Path to link in the source file */
        ) {
            return h5pp::hdf5::moveLink(sourceFilePath,
                                        sourceLinkPath,
                                        getFilePath(),
                                        localLinkPath,
                                        h5pp::FilePermission::READWRITE,
                                        plists);
        }

        /*! Move a link (dataset/table/group) from this file to any hid::h5x location (group or file).
         *
         *  **NOTE:** The link is deleted from the target location, but the storage space is not recovered.
         *  This is a fundamental limitation of HDF5.
         */
        template<typename h5x_tgt, typename = h5pp::type::sfinae::enable_if_is_h5_loc_t<h5x_tgt>>
        void moveLinkToLocation(
            std::string_view localLinkPath,                 /*!< Path to link in this file */
            const h5x_tgt   &targetLocationId,              /*!< Target hid::h5x location handle (group or file) */
            std::string_view targetLinkPath,                /*!< Path to link in the target file */
            LocationMode     locMode = LocationMode::DETECT /*!< Specify whether targetLocationId is in this file or another */

        ) const {
            return h5pp::hdf5::moveLink(openFileHandle(), localLinkPath, targetLocationId, targetLinkPath, locMode, plists);
        }

        /*! Move a link (dataset/table/group) from this file to any hid::h5x location (group or file).
         *
         *  **NOTE:** The link is deleted from the target location, but the storage space is not recovered.
         *  This is a fundamental limitation of HDF5.
         */
        template<typename h5x_src, typename = h5pp::type::sfinae::enable_if_is_h5_loc_t<h5x_src>>
        void moveLinkFromLocation(
            std::string_view localLinkPath,                 /*!< Path to link in this file */
            const h5x_src   &sourceLocationId,              /*!< Source hid::h5x location handle (group or file) */
            std::string_view sourceLinkPath,                /*!< Path to link in the source file */
            LocationMode     locMode = LocationMode::DETECT /*!< Specify whether targetLocationId is in this file or another */
        ) {
            return h5pp::hdf5::moveLink(sourceLocationId, sourceLinkPath, openFileHandle(), localLinkPath, locMode, plists);
        }

        /*
         *
         * Functions for logging
         *
         */

        /*! Get the current log level
         *
         * From 0 (highest) to 6 (off)
         * */
        [[nodiscard]] size_t getLogLevel() const { return logLevel; }

        /*! Set console log level
         *
         * From 0 (highest) to 6 (off)
         */
        void setLogLevel(size_t logLevelZeroToSix /*!< Log level */
        ) {
            logLevel = logLevelZeroToSix;
            h5pp::logger::setLogLevel(logLevelZeroToSix);
        }

        /*
         *
         * Functions related to compression
         *
         */

        /*! Set compression level
         *
         * Uses ZLIB compression level 0 (off) to 9 (highest)
         * Levels 2 to 5 are recommended for good performance/compression ratio
         */
        void setCompressionLevel(unsigned int compressionLevelZeroToNine /*!< Compression level */
        ) {
            currentCompressionLevel = h5pp::hdf5::getValidCompressionLevel(compressionLevelZeroToNine);
        }

        /*! Get current default compression level */
        [[nodiscard]] unsigned int getCompressionLevel() const { return currentCompressionLevel; }

        /*! Get a *valid* compression level given an optionally suggested level.
         *
         * Example 1: Passing compressionLevel > 9 returns 9 if ZLIB compression is enabled.
         *
         * Example 2: If compressionLevel == std::nullopt, the current compression level is returned.
         */
        [[nodiscard]] unsigned int getCompressionLevel(std::optional<unsigned int> compressionLevel /*!< Suggested compression level */
                                                       ) const {
            if(compressionLevel)
                return h5pp::hdf5::getValidCompressionLevel(compressionLevel.value());
            else
                return currentCompressionLevel;
        }


        /*
         *
         * Functions related to groups and datasets
         *
         */


        void createGroup(std::string_view group_relative_name) {
            h5pp::hdf5::createGroup(openFileHandle(), group_relative_name, std::nullopt, plists);
        }

        void resizeDataset(DsetInfo &info, const DimsType &newDimensions, std::optional<h5pp::ResizePolicy> mode_override = std::nullopt) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to resize dataset on read-only file [{}]", filePath.string()));
            h5pp::hdf5::resizeDataset(info, newDimensions, mode_override);
        }

        DsetInfo
            resizeDataset(std::string_view dsetPath, const DimsType &newDimensions, std::optional<h5pp::ResizePolicy> mode = std::nullopt) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to resize dataset on read-only file [{}]", filePath.string()));
            Options options;
            options.linkPath     = dsetPath;
            options.resizePolicy = mode;
            auto info            = h5pp::scan::inferDsetInfo(openFileHandle(), dsetPath, options, plists);
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
            auto dsetInfo = h5pp::scan::makeDsetInfo(openFileHandle(), options, plists);
            h5pp::File::createDataset(dsetInfo);
            return dsetInfo;
        }

        DsetInfo createDataset(std::optional<hid::h5t>     h5Type,
                               std::string_view            dsetPath,
                               const DimsType             &dsetDims,
                               std::optional<H5D_layout_t> h5Layout      = std::nullopt,
                               const OptDimsType          &dsetDimsChunk = std::nullopt,
                               const OptDimsType          &dsetDimsMax   = std::nullopt,
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
        DsetInfo createDataset(const DataType             &data,
                               std::string_view            dsetPath,
                               const OptDimsType          &dataDims      = std::nullopt,
                               std::optional<H5D_layout_t> h5Layout      = std::nullopt,
                               const OptDimsType          &dsetDimsChunk = std::nullopt,
                               const OptDimsType          &dsetDimsMax   = std::nullopt,
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
            // Fill missing metadata in given dset
            if(dsetInfo.hasLocId())
                h5pp::scan::readDsetInfo(dsetInfo, dsetInfo.getLocId(), options, plists);
            else
                h5pp::scan::readDsetInfo(dsetInfo, openFileHandle(), options, plists);
            if(not dsetInfo.dsetExists or not dsetInfo.dsetExists.value()) createDataset(dsetInfo);
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            // Resize dataset to fit the given data (or a selection therein)
            h5pp::hdf5::resizeDataset(dsetInfo, dataInfo);
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists);
        }

        template<typename DataType>
        void writeDataset(const DataType &data, DataInfo &dataInfo, DsetInfo &dsetInfo, const Options &options = Options()) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            // Fill missing metadata in dsetInfo
            if(dsetInfo.hasLocId())
                h5pp::scan::readDsetInfo(dsetInfo, dsetInfo.getLocId(), options, plists);
            else
                h5pp::scan::readDsetInfo(dsetInfo, openFileHandle(), options, plists);
            // Fill missing metadata in dataInfo
            h5pp::scan::scanDataInfo(dataInfo, data, options);
            if(not dsetInfo.dsetExists or not dsetInfo.dsetExists.value()) createDataset(dsetInfo);
            // Resize dataset to fit the given data (or a selection therein)
            h5pp::hdf5::resizeDataset(dsetInfo, dataInfo);
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists);
        }

        template<typename DataType>
        DsetInfo writeDataset(const DataType &data, const Options &options) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            options.assertWellDefined();
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            auto dsetInfo = h5pp::scan::inferDsetInfo(openFileHandle(), data, options, plists);
            writeDataset(data, dataInfo, dsetInfo);
            return dsetInfo;
        }

        /* clang-format off */
        template<typename DataType>
        DsetInfo writeDataset(
            const DataType &            data,                          /*!< Eigen, stl-like object or pointer to data buffer */
            std::string_view            dsetPath,                      /*!< Path to HDF5 dataset relative to the file root */
            const OptDimsType &         dataDims       = std::nullopt, /*!< Data dimensions hint. Required for pointer data */
            std::optional<H5D_layout_t> h5Layout       = std::nullopt, /*!< (On create) Layout of dataset. Choose between H5D_CHUNKED,H5D_COMPACT and H5D_CONTIGUOUS */
            const OptDimsType &         dsetDimsChunk  = std::nullopt, /*!< (On create) Chunking dimensions. Only valid for H5D_CHUNKED datasets */
            const OptDimsType &         dsetDimsMax    = std::nullopt, /*!< (On create) Maximum dimensions. Only valid for H5D_CHUNKED datasets */
            std::optional<hid::h5t>     h5Type         = std::nullopt, /*!< (On create) Type of dataset. Override automatic type detection. */
            std::optional<ResizePolicy> resizePolicy   = std::nullopt, /*!< Type of resizing if needed. Choose GROW, FIT,OFF */
            std::optional<unsigned int> compression    = std::nullopt) /*!< (On create) Compression level 0-9, 0 = off, 9 is gives best compression and is slowest */
        {
            /* clang-format on */
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetDimsChunk = dsetDimsChunk;
            options.dsetDimsMax   = dsetDimsMax;
            options.h5Layout      = h5Layout;
            options.h5Type        = std::move(h5Type);
            options.resizePolicy  = resizePolicy;
            options.compression   = getCompressionLevel(compression);
            return writeDataset(data, options);
        }

        template<typename DataType>
        DsetInfo writeDataset(
            /* clang-format off */
            const DataType &            data,                         /*!< Eigen, stl-like object or pointer to data buffer */
            std::string_view            dsetPath,                     /*!< Path to HDF5 dataset relative to the file root */
            hid::h5t &                  h5Type,                       /*!< (On create) Type of dataset. Override automatic type detection. */
            const OptDimsType &         dataDims      = std::nullopt, /*!< Data dimensions hint. Required for pointer data */
            std::optional<H5D_layout_t> h5Layout      = std::nullopt, /*!< (On create) Layout of dataset. Choose between H5D_CHUNKED,H5D_COMPACT and H5D_CONTIGUOUS */
            const OptDimsType &         dsetDimsChunk = std::nullopt, /*!< (On create) Chunking dimensions. Only valid for H5D_CHUNKED datasets */
            const OptDimsType &         dsetDimsMax   = std::nullopt, /*!< (On create) Maximum dimensions. Only valid for H5D_CHUNKED datasets */
            std::optional<ResizePolicy> resizePolicy  = std::nullopt, /*!< Type of resizing if needed. Choose GROW, FIT, OFF */
            std::optional<unsigned int> compression   = std::nullopt  /*!< (On create) Compression level 0-9, 0 = off, 9 is gives best compression and is slowest */
            /* clang-format on */
        ) {
            //            hid::h5t test {4};
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetDimsChunk = dsetDimsChunk;
            options.dsetDimsMax   = dsetDimsMax;
            options.h5Layout      = h5Layout;
            options.h5Type        = h5Type;
            options.resizePolicy  = resizePolicy;
            options.compression   = getCompressionLevel(compression);
            return writeDataset(data, options);
        }

        template<typename DataType>
        DsetInfo writeDataset(
            /* clang-format off */
            const DataType &            data,                         /*!< Eigen, stl-like object or pointer to data buffer */
            std::string_view            dsetPath,                     /*!< Path to HDF5 dataset relative to the file root */
            H5D_layout_t                h5Layout,                     /*!< (On create) Layout of dataset. Choose between H5D_CHUNKED,H5D_COMPACT and H5D_CONTIGUOUS */
            const OptDimsType &         dataDims      = std::nullopt, /*!< Data dimensions hint. Required for pointer data */
            const OptDimsType &         dsetDimsChunk = std::nullopt, /*!< (On create) Chunking dimensions. Only valid for H5D_CHUNKED datasets */
            const OptDimsType &         dsetDimsMax   = std::nullopt, /*!< (On create) Maximum dimensions. Only valid for H5D_CHUNKED datasets */
            std::optional<hid::h5t>     h5Type        = std::nullopt, /*!< (On create) Type of dataset. Override automatic type detection. */
            std::optional<ResizePolicy> resizePolicy  = std::nullopt, /*!< Type of resizing if needed. Choose GROW, FIT, OFF */
            std::optional<unsigned int> compression   = std::nullopt  /*!< (On create) Compression level 0-9, 0 = off, 9 is gives best compression and is slowest */
            /* clang-format on */
        ) {
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetDimsChunk = dsetDimsChunk;
            options.dsetDimsMax   = dsetDimsMax;
            options.h5Layout      = h5Layout;
            options.h5Type        = std::move(h5Type);
            options.resizePolicy  = resizePolicy;
            options.compression   = getCompressionLevel(compression);
            return writeDataset(data, options);
        }

        template<typename DataType>
        DsetInfo writeDataset_compact(const DataType         &data,
                                      std::string_view        dsetPath,
                                      const OptDimsType      &dataDims = std::nullopt,
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
        DsetInfo writeDataset_contiguous(const DataType         &data,
                                         std::string_view        dsetPath,
                                         const OptDimsType      &dataDims = std::nullopt,
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
        DsetInfo writeDataset_chunked(const DataType             &data,
                                      std::string_view            dsetPath,
                                      const OptDimsType          &dataDims      = std::nullopt,
                                      const OptDimsType          &dsetDimsChunk = std::nullopt,
                                      const OptDimsType          &dsetDimsMax   = std::nullopt,
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

        template<typename DataType>
        DsetInfo writeHyperslab(const DataType  &data,      /*!< Eigen, stl-like object or pointer to data buffer */
                                std::string_view dsetPath,  /*!< Path to HDF5 dataset relative to the file root */
                                const Hyperslab &hyperslab) /*!< Write data to a hyperslab selection */
        {
            Options options;
            options.linkPath     = dsetPath;
            options.dsetSlab     = hyperslab;
            options.resizePolicy = ResizePolicy::OFF;
            auto dsetInfo        = h5pp::scan::readDsetInfo(openFileHandle(), options, plists);
            if(not dsetInfo.dsetExists or not dsetInfo.dsetExists.value())
                throw std::runtime_error(h5pp::format("Could not write hyperslab: dataset [{}] does not exist", dsetPath));
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            // Resize dataset to fit the given data (or a selection therein)
            h5pp::hdf5::resizeDataset(dsetInfo, dataInfo);
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists);
            return dsetInfo;
        }

        void writeSymbolicLink(std::string_view src_path, std::string_view tgt_path) {
            h5pp::hdf5::writeSymbolicLink(openFileHandle(), src_path, tgt_path, std::nullopt, plists);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readDataset(DataType &data, DataInfo &dataInfo, const DsetInfo &dsetInfo) const {
            h5pp::hdf5::resizeData(data, dataInfo, dsetInfo);
            h5pp::hdf5::readDataset(data, dataInfo, dsetInfo, plists);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        [[nodiscard]] DataType readDataset(DataInfo &dataInfo, const DsetInfo &dsetInfo) const {
            DataType data;
            readDataset(data, dataInfo, dsetInfo);
            return data;
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readDataset(DataType &data, const DsetInfo &dsetInfo, const Options &options = Options()) const {
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            readDataset(data, dataInfo, dsetInfo);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        [[nodiscard]] DataType readDataset(const DsetInfo &dsetInfo, const Options &options = Options()) const {
            DataType data;
            readDataset(data, dsetInfo, options);
            return data;
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        [[nodiscard]] DataType readDataset(const DsetInfo &dsetInfo, const DimsType &dataDims) const {
            DataType data;
            Options  options;
            options.dataDims = dataDims;
            readDataset(data, dsetInfo, options);
            return data;
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readDataset(DataType &data, const Options &options) const {
            options.assertWellDefined();
            // Generate the metadata for the dataset on file
            auto dsetInfo = h5pp::scan::readDsetInfo(openFileHandle(), options, plists);
            if(dsetInfo.dsetExists and not dsetInfo.dsetExists.value())
                throw std::runtime_error(h5pp::format("Cannot read dataset [{}]: It does not exist", options.linkPath.value()));
            // Generate the metadata for given data
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            // Resize the given data container so that it fits the selection in the dataset
            h5pp::hdf5::resizeData(data, dataInfo, dsetInfo);
            // Read
            h5pp::hdf5::readDataset(data, dataInfo, dsetInfo, plists);
        }
        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        [[nodiscard]] DataType readDataset(std::string_view dsetPath, const Options &options) const {
            Options options_internal  = options;
            options_internal.linkPath = dsetPath;
            DataType data;
            readDataset(data, options_internal);
            return data;
        }

        template<typename DataType>
        void readDataset(DataType               &data,
                         std::string_view        dsetPath,
                         const OptDimsType      &dataDims = std::nullopt,
                         std::optional<hid::h5t> h5Type   = std::nullopt) const {
            Options options;
            options.linkPath = dsetPath;
            options.dataDims = dataDims;
            options.h5Type   = std::move(h5Type);
            readDataset(data, options);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        [[nodiscard]] DataType readDataset(std::string_view        datasetPath,
                                           const OptDimsType      &dataDims = std::nullopt,
                                           std::optional<hid::h5t> h5Type   = std::nullopt) const {
            DataType data;
            readDataset(data, datasetPath, dataDims, std::move(h5Type));
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

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readHyperslab(DataType &data, std::string_view dsetPath, const Hyperslab &hyperslab) const {
            Options options;
            options.linkPath = dsetPath;
            options.dsetSlab = hyperslab;
            readDataset(data, options);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        [[nodiscard]] DataType readHyperslab(std::string_view dsetPath, const Hyperslab &hyperslab) const {
            DataType data;
            readHyperslab(data, dsetPath, hyperslab);
            return data;
        }

        /*
         *
         * Functions related to attributes
         *
         */

        void createAttribute(AttrInfo &attrInfo, const Options &options = Options()) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to create attribute on read-only file [{}]", filePath.string()));
            if(attrInfo.hasLocId())
                h5pp::scan::inferAttrInfo(attrInfo, attrInfo.getLocId(), options, plists);
            else
                h5pp::scan::inferAttrInfo(attrInfo, openFileHandle(), options, plists);

            h5pp::hdf5::createAttribute(attrInfo);
        }

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
        AttrInfo createAttribute(const DataType &data, const DimsType &dataDims, std::string_view attrName, std::string_view linkPath) {
            Options options;
            options.linkPath = linkPath;
            options.attrName = attrName;
            options.dataDims = dataDims;
            return createAttribute(data, options);
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
        AttrInfo writeAttribute(const DataType &data, const Options &options) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            options.assertWellDefined();
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            auto attrInfo = createAttribute(data, options);
            h5pp::hdf5::writeAttribute(data, dataInfo, attrInfo);
            return attrInfo;
        }

        template<typename DataType>
        AttrInfo writeAttribute(const DataType         &data,
                                std::string_view        attrName,
                                std::string_view        linkPath,
                                const OptDimsType      &dataDims = std::nullopt,
                                std::optional<hid::h5t> h5Type   = std::nullopt) {
            Options options;
            options.linkPath = linkPath;
            options.attrName = attrName;
            options.dataDims = dataDims;
            options.h5Type   = std::move(h5Type);
            return writeAttribute(data, options);
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
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            h5pp::hdf5::resizeData(data, dataInfo, attrInfo);
            h5pp::hdf5::readAttribute(data, dataInfo, attrInfo);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readAttribute(DataType &data, const Options &options) const {
            options.assertWellDefined();
            auto attrInfo = h5pp::scan::readAttrInfo(openFileHandle(), options, plists);
            readAttribute(data, attrInfo, options);
        }

        template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
        void readAttribute(DataType          &data,
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

        void createTable(TableInfo &info, const Options &options = Options()) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            if(info.hasLocId())
                h5pp::scan::inferTableInfo(info, info.getLocId(), options, plists);
            else
                h5pp::scan::inferTableInfo(info, openFileHandle(), options, plists);
            h5pp::hdf5::createTable(info, plists);
        }

        TableInfo createTable(const hid::h5t                   &h5Type,
                              std::string_view                  tablePath,
                              std::string_view                  tableTitle,
                              const OptDimsType                &chunkDims        = std::nullopt,
                              const std::optional<unsigned int> compressionLevel = std::nullopt

        ) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            Options options;
            options.linkPath      = h5pp::util::safe_str(tablePath);
            options.h5Type        = h5Type;
            options.dsetDimsChunk = chunkDims;
            options.compression   = compressionLevel;
            auto info             = h5pp::scan::makeTableInfo(openFileHandle(), options, tableTitle, plists);
            h5pp::hdf5::createTable(info, plists);
            h5pp::scan::readTableInfo(info, info.getLocId(), options, plists);
            return info;
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

        TableInfo appendTableRecords(const h5pp::TableInfo            &srcInfo,
                                     TableSelection                    srcTableSelection,
                                     std::string_view                  tgtTablePath,
                                     const OptDimsType                &chunkDims        = std::nullopt,
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
        TableInfo appendTableRecords(const h5x_src                    &srcLocation,
                                     std::string_view                  srcTablePath,
                                     TableSelection                    srcTableSelection,
                                     std::string_view                  tgtTablePath,
                                     const OptDimsType                &chunkDims        = std::nullopt,
                                     const std::optional<unsigned int> compressionLevel = std::nullopt) {
            Options options;
            options.linkPath = h5pp::util::safe_str(srcTablePath);
            auto srcInfo     = h5pp::scan::readTableInfo(srcLocation, options, plists);
            return appendTableRecords(srcInfo, srcTableSelection, tgtTablePath, chunkDims, compressionLevel);
        }

        void copyTableRecords(const h5pp::TableInfo &srcInfo,
                              hsize_t                srcStartIdx,
                              hsize_t                numRecordsToCopy,
                              h5pp::TableInfo       &tgtInfo,
                              hsize_t                tgtStartIdx) {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("Attempted to write on read-only file [{}]", filePath.string()));
            if(not srcInfo.numRecords) throw std::runtime_error("Source TableInfo has undefined field [numRecords]");
            numRecordsToCopy = std::min(srcInfo.numRecords.value() - srcStartIdx, numRecordsToCopy);
            h5pp::hdf5::copyTableRecords(srcInfo, srcStartIdx, numRecordsToCopy, tgtInfo, tgtStartIdx);
        }

        void copyTableRecords(const h5pp::TableInfo &srcInfo,
                              TableSelection         srcTableSelection,
                              h5pp::TableInfo       &tgtInfo,
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

        TableInfo copyTableRecords(const h5pp::TableInfo            &srcInfo,
                                   TableSelection                    tableSelection,
                                   std::string_view                  tgtTablePath,
                                   hsize_t                           tgtStartIdx,
                                   const OptDimsType                &chunkDims        = std::nullopt,
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

        TableInfo copyTableRecords(const h5pp::TableInfo            &srcInfo,
                                   hsize_t                           srcStartIdx,
                                   hsize_t                           numRecordsToCopy,
                                   std::string_view                  tgtTablePath,
                                   hsize_t                           tgtStartIdx,
                                   const OptDimsType                &chunkDims   = std::nullopt,
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
        TableInfo copyTableRecords(const h5x_src                    &srcLocation,
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
        TableInfo copyTableRecords(const h5x_src                    &srcLocation,
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
                                   const OptDimsType                &chunkDims        = std::nullopt,
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
        void readTableRecords(DataType             &data,
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
        [[nodiscard]] DataType readTableRecords(std::string_view      tablePath,
                                                std::optional<size_t> startIdx   = std::nullopt,
                                                std::optional<size_t> numRecords = std::nullopt) const {
            DataType data;
            readTableRecords(data, tablePath, startIdx, numRecords);
            return data;
        }

        template<typename DataType>
        [[nodiscard]] DataType readTableRecords(std::string_view tablePath, h5pp::TableSelection tableSelection) const {
            DataType data;
            readTableRecords(data, tablePath, tableSelection);
            return data;
        }

        template<typename DataType>
        void readTableField(DataType             &data,
                            const TableInfo      &info,
                            NamesOrIndices      &&fieldNamesOrIndices,
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
        void readTableField(DataType             &data,
                            std::string_view      tablePath,
                            NamesOrIndices      &&fieldNamesOrIndices,
                            std::optional<size_t> startIdx   = std::nullopt,
                            std::optional<size_t> numRecords = std::nullopt) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            auto info        = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            readTableField(data, info, std::forward<NamesOrIndices>(fieldNamesOrIndices), startIdx, numRecords);
        }

        template<typename DataType>
        [[nodiscard]] DataType readTableField(std::string_view      tablePath,
                                              NamesOrIndices      &&fieldNamesOrIndices,
                                              std::optional<size_t> startIdx   = std::nullopt,
                                              std::optional<size_t> numRecords = std::nullopt) const {
            DataType data;
            readTableField(data, tablePath, std::forward<NamesOrIndices>(fieldNamesOrIndices), startIdx, numRecords);
            return data;
        }

        template<typename DataType>
        void readTableField(DataType        &data,
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
        [[nodiscard]] DataType
            readTableField(std::string_view tablePath, NamesOrIndices &&fieldNamesOrIndices, TableSelection tableSelection) const {
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

        [[nodiscard]] bool linkExists(std::string_view linkPath) const {
            return h5pp::hdf5::checkIfLinkExists(openFileHandle(), linkPath, plists.linkAccess);
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
