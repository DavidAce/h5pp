
#pragma once

#include "h5ppConstants.h"
#include "h5ppDimensionType.h"
#include "h5ppEigen.h"
#include "h5ppEnums.h"
#include "h5ppError.h"
#include "h5ppFilesystem.h"
#include "h5ppHdf5.h"
#include "h5ppHid.h"
#include "h5ppInitListType.h"
#include "h5ppLogger.h"
#include "h5ppOptional.h"
#include "h5ppPropertyLists.h"
#include "h5ppScan.h"
#include "h5ppUtils.h"
#include "h5ppVarr.h"
#include "h5ppVersion.h"
#include "h5ppVstr.h"
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
        fs::path                                  filePath;                                      /*!< Full path to the file */
        h5pp::FileAccess                          fileAccess         = h5pp::FileAccess::RENAME; /*!< File open/create policy. */
        mutable std::optional<hid::h5f>           fileHandle         = std::nullopt;   /*!< Keeps a file handle alive in batch operations */
        mutable LogLevel                          logLevel           = LogLevel::info; /*!< Log verbosity from 0 [trace] to 6 [off] */
        bool                                      logTimestamp       = false;          /*!< Add a time stamp to console log output */
        hid::h5e                                  error_stack        = H5E_DEFAULT;    /*!< Reference to the error stack used by HDF5 */
        int                                       currentCompression = -1; /*!< Compression level (-1 is off, 0 is none, 9 is max) */
        mutable std::vector<ReclaimInfo::Reclaim> reclaimStack;            /*!< Stores alloc metadata from variable-length reads to free */
        void                                      init() {
            h5pp::logger::setLogger("h5pp|init", logLevel, logTimestamp);
            h5pp::logger::log->debug("Accessing file: [{}]", filePath.string());

            /* Set default error print output */
            error_stack                          = H5Eget_current_stack();
            herr_t turnOffAutomaticErrorPrinting = H5Eset_auto2(error_stack, nullptr, nullptr);
            if(turnOffAutomaticErrorPrinting < 0) throw h5pp::runtime_error("Failed to turn off H5E error printing");

            // The following function can modify the resulting filePath depending on permission.
            filePath = h5pp::hdf5::createFile(filePath, fileAccess, plists);
        }

        public:
        // The following struct contains modifiable property lists.
        // This allows us to use h5pp with MPI, for instance.
        // Unmodified, these default to serial (non-MPI) use.
        // We expose these property lists here so the user may set them as needed.
        // To enable MPI, the user can call H5Pset_fapl_mpio(hid_t file_access_plist, MPI_Comm comm, MPI_Info info)
        // The user is responsible for linking to MPI and learning how to set properties for MPI usage
        PropertyLists plists = PropertyLists();

        /*! Default constructor */
        File() = default;
        template<typename LogLevelType = LogLevel>
        explicit File(h5pp::fs::path       filePath_,                                /*!< Path a new file */
                      h5pp::FileAccess     fileAccess_   = h5pp::FileAccess::RENAME, /*!< Set file access permission in case of collision */
                      LogLevelType         logLevel_     = LogLevel::info,           /*!< Logging verbosity level 0 (most) to 6 (least). */
                      bool                 logTimestamp_ = false,                    /*!< True prepends a timestamp to log output */
                      const PropertyLists &plists_       = PropertyLists())
            : filePath(std::move(filePath_)), fileAccess(fileAccess_), logLevel(Num2Level(logLevel_)), logTimestamp(logTimestamp_),
              plists(plists_) {
            init();
        }
        template<typename LogLevelType = LogLevel>
        explicit File(h5pp::fs::path       filePath_,                      /*!< Path a new file */
                      unsigned int         H5F_ACC_FLAGS,                  /*!< Set HDF5 access flag for new files */
                      LogLevelType         logLevel_     = LogLevel::info, /*!< Logging verbosity level 0 (most) to 6 (least). */
                      bool                 logTimestamp_ = false,          /*!< True prepends a timestamp to log output */
                      const PropertyLists &plists_       = PropertyLists())
            : filePath(std::move(filePath_)), logLevel(Num2Level(logLevel_)), logTimestamp(logTimestamp_), plists(plists_) {
            fileAccess = h5pp::hdf5::convertFileAccessFlags(H5F_ACC_FLAGS);
            init();
        }

        /*! Flush the HDF5 internal file cache using H5Fflush */
        void flush() {
            H5Fflush(openFileHandle(), H5F_scope_t::H5F_SCOPE_GLOBAL);
            h5pp::logger::log->trace("Flushing caches");
            H5garbage_collect();
            H5Eprint(H5E_DEFAULT, stderr);
        }
        /*! Calls H5Treclaim(...) on any data that HDF5 may have allocated for variable-length data during the last reads */
        void vlenReclaim() const {
            for(auto &item : reclaimStack) item.reclaim();
            reclaimStack.clear();
        }

        /*!  Drop all tracked claims to allocated variable-length data (users should call H5Treclaim or free manually)  */
        void vlenDropReclaims() const {
            for(auto &item : reclaimStack) item.drop();
            reclaimStack.clear();
        }

        /*! Enable tracking of variable-length data allocations (e.g. when reading tables containgin H5T_VLEN members) */
        void vlenEnableReclaimsTracking() { plists.vlenTrackReclaims = true; }

        /*! Disable tracking of variable-length data allocations (users should call H5Treclaim or free manually) */
        void vlenDisableReclaimsTracking() { plists.vlenTrackReclaims = false; }

        /*! Returns an HDF5 file handle
         *
         * - The file permission is set when initializing h5pp::File.
         * - Use `h5pp::setKeepFileOpened()` to keep a cached handle. Use `h5pp::setKeepFileClosed()` to close the cached handle.
         */
        [[nodiscard]] hid::h5f openFileHandle() const {
            h5pp::logger::setLogger("h5pp|" + filePath.filename().string(), logLevel, logTimestamp);
            if(fileHandle) return fileHandle.value();
            // Give the option to override the close degree
            // When a file handle is closed, the default in h5pp is to first close all associated id's, and then close the file
            // (H5F_CLOSE_STRONG) Setting H5F_CLOSE_WEAK keeps the file handle alive until associated id's are closed.
            if(fileAccess == h5pp::FileAccess::READONLY) {
                h5pp::logger::log->trace("Opening file with READONLY access");
                hid_t fid = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, plists.fileAccess);
                if(fid < 0) throw h5pp::runtime_error("Failed to open file with read-only access [{}]", filePath.string());
                else return fid;
            } else {
                h5pp::logger::log->trace("Opening file with READWRITE access");
                hid_t fid = H5Fopen(filePath.string().c_str(), H5F_ACC_RDWR, plists.fileAccess);
                if(fid < 0) throw h5pp::runtime_error("Failed to open file with read-write access [{}]", filePath.string());
                else return fid;
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

        struct FileHandleToken {
            const h5pp::File &file_;
            FileHandleToken(const h5pp::File &file) : file_(file) {
                hid::h5f temphandle = file_.openFileHandle();
                file_.fileHandle    = temphandle;
            }
            ~FileHandleToken() { file_.fileHandle = std::nullopt; }
        };
        FileHandleToken getFileHandleToken() { return FileHandleToken(*this); }

        void setKeepFileOpened() const {
            // Check before setting onto self:
            // otherwise repeated calls to setKeepFileOpened() would increment the reference count,
            // without there existing a handle to decrement it --> memory leak
            if(not fileHandle) fileHandle = openFileHandle();
        }

        /*! Close a cached file handle if it exists */
        void setKeepFileClosed() const { fileHandle = std::nullopt; }

        /*! Gets the current file access permission */
        [[nodiscard]] h5pp::FileAccess getFileAccess() const { return fileAccess; }
        [[nodiscard]] h5pp::FileAccess getFilePermission() const {
            h5pp::logger::log->info("Deprecation notice: FilePermission has been renamed to FileAccess in h5pp version 1.10. "
                                    "FilePermission will be removed in a future version.");
            return fileAccess;
        }

        /*! Gets the current file name */
        [[nodiscard]] std::string getFileName() const { return filePath.filename().string(); }

        /*! Gets the full path to the current file */
        [[nodiscard]] std::string getFilePath() const { return filePath.string(); }

        /*! Sets the default file access permission */
        void setFileAccess(h5pp::FileAccess fileAccess_) { fileAccess = fileAccess_; }
        void setFilePermission(h5pp::FileAccess fileAccess_) {
            h5pp::logger::log->info("Deprecation notice: FilePermission has been renamed to FileAccess in h5pp version 1.10. "
                                    "FilePermission will be removed in a future version.");
            fileAccess = fileAccess_;
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
            copyFileTo(const h5pp::fs::path &targetFilePath,                   /*!< Copy to this path */
                       const FileAccess     &perm = FileAccess::COLLISION_FAIL /*!< File access permission at the new path */

            ) const {
            return h5pp::hdf5::copyFile(getFilePath(), targetFilePath, perm, plists);
        }

        /*! Move the current file to a new path.
         *
         * The current file is re-opened at the new path.
         */
        [[maybe_unused]] fs::path
            moveFileTo(const h5pp::fs::path &targetFilePath,                   /*!< The new path */
                       const FileAccess     &perm = FileAccess::COLLISION_FAIL /*!< File access permission at the new path */
            ) {
            auto newPath = h5pp::hdf5::moveFile(getFilePath(), targetFilePath, perm, plists);
            if(fs::exists(newPath)) filePath = newPath;
            return newPath;
        }

        /*
         *
         * Functions for transferring contents
         *
         */

        /*! Copy a link (dataset/table/group) into another file. */
        void copyLinkToFile(std::string_view      localLinkPath,               /*!< Path to link in this file */
                            const h5pp::fs::path &targetFilePath,              /*!< Path to file to copy into */
                            std::string_view      targetLinkPath,              /*!< Path to link in the target file  */
                            const FileAccess     &perm = FileAccess::READWRITE /*!< File access permission at the target path */
        ) const {
            return h5pp::hdf5::copyLink(getFilePath(), localLinkPath, targetFilePath, targetLinkPath, perm, plists);
        }

        /*! Copy a link (dataset/table/group) from another file into this. */
        void copyLinkFromFile(std::string_view      localLinkPath,  /*!< Path to link in this file */
                              const h5pp::fs::path &sourceFilePath, /*!< Path to file to copy from */
                              std::string_view      sourceLinkPath  /*!< Path to link in the source file */
        ) {
            return h5pp::hdf5::copyLink(sourceFilePath, sourceLinkPath, getFilePath(), localLinkPath, h5pp::FileAccess::READWRITE, plists);
        }

        /*! Copy a link (dataset/table/group) from this file to any hid::h5x location (group or file). */
        template<typename h5x_tgt>
        void copyLinkToLocation(std::string_view localLinkPath,    /*!< Path to link in this file */
                                const h5x_tgt   &targetLocationId, /*!< Target hid::h5x location handle (group or file) */
                                std::string_view targetLinkPath    /*!< Path to link in the target file */
        ) const {
            static_assert(type::sfinae::is_h5pp_loc_id<h5x_tgt>);
            return h5pp::hdf5::copyLink(openFileHandle(), localLinkPath, targetLocationId, targetLinkPath, plists);
        }

        /*! Copy a link (dataset/table/group) from any hid::h5x location (group or file) into this file */
        template<typename h5x_src>
        void copyLinkFromLocation(std::string_view localLinkPath,    /*!< Path to link in this file */
                                  const h5x_src   &sourceLocationId, /*!< Source hid::h5x location handle*/
                                  std::string_view sourceLinkPath    /*!< Path to link in the source file */
        ) {
            static_assert(type::sfinae::is_h5pp_loc_id<h5x_src>);
            return h5pp::hdf5::copyLink(sourceLocationId, sourceLinkPath, openFileHandle(), localLinkPath, plists);
        }

        /*! Move a link (dataset/table/group) into another file.
         *
         *  **NOTE:** The link is deleted from this file, but the storage space is not recovered.
         *  This is a fundamental limitation of HDF5.
         */
        void moveLinkToFile(std::string_view      localLinkPath,               /*!< Path to link in this file */
                            const h5pp::fs::path &targetFilePath,              /*!< Path to file to move into */
                            std::string_view      targetLinkPath,              /*!< Path to link in the target file  */
                            const FileAccess     &perm = FileAccess::READWRITE /*!< File access permission at the target path */

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
            return h5pp::hdf5::moveLink(sourceFilePath, sourceLinkPath, getFilePath(), localLinkPath, h5pp::FileAccess::READWRITE, plists);
        }

        /*! Move a link (dataset/table/group) from this file to any hid::h5x location (group or file).
         *
         *  **NOTE:** The link is deleted from the target location, but the storage space is not recovered.
         *  This is a fundamental limitation of HDF5.
         */
        template<typename h5x_tgt>
        void moveLinkToLocation(
            std::string_view localLinkPath,                 /*!< Path to link in this file */
            const h5x_tgt   &targetLocationId,              /*!< Target hid::h5x location handle (group or file) */
            std::string_view targetLinkPath,                /*!< Path to link in the target file */
            LocationMode     locMode = LocationMode::DETECT /*!< Specify whether targetLocationId is in this file or another */

        ) const {
            static_assert(type::sfinae::is_h5pp_loc_id<h5x_tgt>);
            return h5pp::hdf5::moveLink(openFileHandle(), localLinkPath, targetLocationId, targetLinkPath, locMode, plists);
        }

        /*! Move a link (dataset/table/group) from this file to any hid::h5x location (group or file).
         *
         *  **NOTE:** The link is deleted from the target location, but the storage space is not recovered.
         *  This is a fundamental limitation of HDF5.
         */
        template<typename h5x_src>
        void moveLinkFromLocation(
            std::string_view localLinkPath,                 /*!< Path to link in this file */
            const h5x_src   &sourceLocationId,              /*!< Source hid::h5x location handle (group or file) */
            std::string_view sourceLinkPath,                /*!< Path to link in the source file */
            LocationMode     locMode = LocationMode::DETECT /*!< Specify whether targetLocationId is in this file or another */
        ) {
            static_assert(type::sfinae::is_h5pp_loc_id<h5x_src>);
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
        [[nodiscard]] LogLevel getLogLevel() const { return logLevel; }

        /*! Set console log level
         *
         * From 0 (highest) to 6 (off)
         */
        template<typename LogLevelType>
        void setLogLevel(LogLevelType logLevelZeroToSix) const {
            logLevel = Num2Level(logLevelZeroToSix);
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
        void setCompressionLevel(unsigned int compressionZeroToNine /*!< Compression level */
        ) {
            currentCompression = h5pp::hdf5::getValidCompressionLevel(compressionZeroToNine);
        }

        /*! Get current default compression level */
        [[nodiscard]] int getCompressionLevel() const { return currentCompression; }

        /*! Get a *valid* compression level given an optionally suggested level.
         *
         * Example 1: Passing compression > 9 returns 9 if ZLIB compression is enabled.
         *
         * Example 2: If compression == std::nullopt, the current compression level is returned.
         */
        [[nodiscard]] int getCompressionLevel(const std::optional<int> compression /*!< Suggested compression level */
        ) const {
            if(compression) return h5pp::hdf5::getValidCompressionLevel(compression.value());
            else return currentCompression;
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
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to resize dataset on read-only file [{}]", filePath.string());
            h5pp::hdf5::resizeDataset(info, newDimensions, mode_override);
        }

        DsetInfo
            resizeDataset(std::string_view dsetPath, const DimsType &newDimensions, std::optional<h5pp::ResizePolicy> mode = std::nullopt) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to resize dataset on read-only file [{}]", filePath.string());
            Options options;
            options.linkPath     = dsetPath;
            options.resizePolicy = mode;
            auto info            = h5pp::scan::inferDsetInfo(openFileHandle(), dsetPath, options, plists);
            if(not info.dsetExists.value()) throw h5pp::runtime_error("Failed to resize dataset [{}]: dataset does not exist", dsetPath);
            h5pp::hdf5::resizeDataset(info, newDimensions, mode);
            return info;
        }

        void createDataset(DsetInfo &info) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to create dataset on read-only file [{}]", filePath.string());
            h5pp::hdf5::createDataset(info, plists);
        }

        DsetInfo createDataset(const Options &options) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to create dataset on read-only file [{}]", filePath.string());
            options.assertWellDefined();
            if(not options.linkPath) throw h5pp::runtime_error("Error creating dataset: No dataset path specified");
            if(not options.dataDims)
                throw h5pp::runtime_error("Error creating dataset [{}]: Dimensions or size not specified", options.linkPath.value());
            if(not options.h5Type)
                throw h5pp::runtime_error("Error creating dataset [{}]: HDF5 type not specified", options.linkPath.value());
            auto dsetInfo = h5pp::scan::makeDsetInfo(openFileHandle(), options, plists);
            h5pp::File::createDataset(dsetInfo);
            return dsetInfo;
        }

        DsetInfo createDataset(std::string_view         dsetPath,
                               const hid::h5t          &h5Type,
                               H5D_layout_t             h5Layout,
                               const DimsType          &dsetDims,
                               const OptDimsType       &dsetChunkDims = std::nullopt,
                               const OptDimsType       &dsetMaxDims   = std::nullopt,
                               const std::optional<int> compression   = std::nullopt) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to create dataset on read-only file [{}]", filePath.string());
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dsetDims;
            options.dsetChunkDims = dsetChunkDims;
            options.dsetMaxDims   = dsetMaxDims;
            options.h5Type        = H5Tcopy(h5Type);
            options.h5Layout      = h5Layout;
            options.compression   = getCompressionLevel(compression);
            return createDataset(options);
        }

        template<typename DataType>
        DsetInfo createDataset(const DataType &data, const Options &options) {
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to create dataset on read-only file [{}]", filePath.string());
            auto dsetInfo = h5pp::scan::inferDsetInfo(openFileHandle(), data, options, plists);
            h5pp::File::createDataset(dsetInfo);
            return dsetInfo;
        }

        template<typename DataType>
        DsetInfo createDataset(const DataType             &data,
                               std::string_view            dsetPath,
                               const OptDimsType          &dataDims      = std::nullopt,
                               std::optional<H5D_layout_t> h5Layout      = std::nullopt,
                               const OptDimsType          &dsetChunkDims = std::nullopt,
                               const OptDimsType          &dsetMaxDims   = std::nullopt,
                               const std::optional<int>    compression   = std::nullopt) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to create dataset on read-only file [{}]", filePath.string());
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetChunkDims = dsetChunkDims;
            options.dsetMaxDims   = dsetMaxDims;
            options.h5Type        = h5pp::type::getH5Type<DataType>();
            options.h5Layout      = h5Layout;
            options.compression   = getCompressionLevel(compression);
            // If dsetdims is a nullopt we can infer its dimensions from the given dataset
            return createDataset(data, options);
        }

        template<typename DataType>
        void writeDataset(const DataType &data, DsetInfo &dsetInfo, const Options &options = Options()) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            // Fill missing metadata in given dset
            if(dsetInfo.hasLocId()) h5pp::scan::readDsetInfo(dsetInfo, dsetInfo.getLocId(), options, plists);
            else h5pp::scan::readDsetInfo(dsetInfo, openFileHandle(), options, plists);
            if(not dsetInfo.dsetExists or not dsetInfo.dsetExists.value()) createDataset(dsetInfo);
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            // Resize dataset to fit the given data (or a selection therein)
            h5pp::hdf5::resizeDataset(dsetInfo, dataInfo);
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists);
        }

        template<typename DataType>
        void writeDataset(const DataType &data, DataInfo &dataInfo, DsetInfo &dsetInfo, const Options &options = Options()) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            // Fill missing metadata in dsetInfo
            if(dsetInfo.hasLocId()) h5pp::scan::readDsetInfo(dsetInfo, dsetInfo.getLocId(), options, plists);
            else h5pp::scan::readDsetInfo(dsetInfo, openFileHandle(), options, plists);
            // Fill missing metadata in dataInfo
            h5pp::scan::scanDataInfo(dataInfo, data, options);
            h5pp::hdf5::createDataset(dsetInfo, plists);
            // Resize dataset to fit the given data (or a selection therein)
            h5pp::hdf5::resizeDataset(dsetInfo, dataInfo);
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists);
        }

        template<typename DataType>
        DsetInfo writeDataset(const DataType &data, const Options &options) {
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
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
            const OptDimsType &         dsetChunkDims  = std::nullopt, /*!< (On create) Chunking dimensions. Only valid for H5D_CHUNKED datasets */
            const OptDimsType &         dsetMaxDims    = std::nullopt, /*!< (On create) Maximum dimensions. Only valid for H5D_CHUNKED datasets */
            std::optional<hid::h5t>     h5Type         = std::nullopt, /*!< (On create) Type of dataset. Override automatic type detection. */
            std::optional<ResizePolicy> resizePolicy   = std::nullopt, /*!< Type of resizing if needed. Choose GROW, FIT,OFF */
            const std::optional<int> compression    = std::nullopt) /*!< (On create) Compression level 0-9, 0 = off, 9 is gives best compression and is slowest */
        /* clang-format on */
        {
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetChunkDims = dsetChunkDims;
            options.dsetMaxDims   = dsetMaxDims;
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
            const OptDimsType &         dsetChunkDims = std::nullopt, /*!< (On create) Chunking dimensions. Only valid for H5D_CHUNKED datasets */
            const OptDimsType &         dsetMaxDims   = std::nullopt, /*!< (On create) Maximum dimensions. Only valid for H5D_CHUNKED datasets */
            std::optional<ResizePolicy> resizePolicy  = std::nullopt, /*!< Type of resizing if needed. Choose GROW, FIT, OFF */
            const std::optional<int> compression   = std::nullopt  /*!< (On create) Compression level 0-9, 0 = off, 9 is gives best compression and is slowest */
            /* clang-format on */
        ) {
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetChunkDims = dsetChunkDims;
            options.dsetMaxDims   = dsetMaxDims;
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
            const OptDimsType &         dsetChunkDims = std::nullopt, /*!< (On create) Chunking dimensions. Only valid for H5D_CHUNKED datasets */
            const OptDimsType &         dsetMaxDims   = std::nullopt, /*!< (On create) Maximum dimensions. Only valid for H5D_CHUNKED datasets */
            std::optional<hid::h5t>     h5Type        = std::nullopt, /*!< (On create) Type of dataset. Override automatic type detection. */
            std::optional<ResizePolicy> resizePolicy  = std::nullopt, /*!< Type of resizing if needed. Choose GROW, FIT, OFF */
            const std::optional<int> compression   = std::nullopt  /*!< (On create) Compression level 0-9, 0 = off, 9 is gives best compression and is slowest */
            /* clang-format on */
        ) {
            Options options;
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetChunkDims = dsetChunkDims;
            options.dsetMaxDims   = dsetMaxDims;
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
        DsetInfo writeDataset_chunked(const DataType          &data,
                                      std::string_view         dsetPath,
                                      const OptDimsType       &dataDims      = std::nullopt,
                                      const OptDimsType       &dsetChunkDims = std::nullopt,
                                      const OptDimsType       &dsetMaxDims   = std::nullopt,
                                      std::optional<hid::h5t>  h5Type        = std::nullopt,
                                      const std::optional<int> compression   = std::nullopt) {
            Options options; // Get optional iterable should have three different return states, nullopt, empty or nonempty, ´,
            options.linkPath      = dsetPath;
            options.dataDims      = dataDims;
            options.dsetChunkDims = dsetChunkDims;
            options.dsetMaxDims   = dsetMaxDims;
            options.h5Layout      = H5D_CHUNKED;
            options.h5Type        = std::move(h5Type);
            options.compression   = getCompressionLevel(compression);
            return writeDataset(data, options);
        }

        template<typename DataType>
        DsetInfo writeDataset_compressed(const DataType &data, std::string_view dsetPath, const std::optional<int> compression = 3) {
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
                throw h5pp::runtime_error("Could not write hyperslab: dataset [{}] does not exist", dsetPath);
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            // Resize dataset to fit the given data (or a selection therein)
            h5pp::hdf5::resizeDataset(dsetInfo, dataInfo);
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists);
            return dsetInfo;
        }

        template<typename DataType>
        void readDataset(DataType &data, DataInfo &dataInfo, const DsetInfo &dsetInfo) const {
            static_assert(not std::is_const_v<DataType>);
            h5pp::hdf5::resizeData(data, dataInfo, dsetInfo);
            h5pp::hdf5::readDataset(data, dataInfo, dsetInfo, plists);
        }

        template<typename DataType>
        [[nodiscard]] DataType readDataset(DataInfo &dataInfo, const DsetInfo &dsetInfo) const {
            static_assert(not std::is_const_v<DataType>);
            if constexpr(type::sfinae::is_specialization_v<DataType, std::optional>) {
                dsetInfo.assertReadReady();
                if(dsetInfo.dsetExists.value() or linkExists(dsetInfo.dsetPath.value()))
                    return readDataset<typename DataType::value_type>(dataInfo, dsetInfo);
                return std::nullopt;
            }
            DataType data;
            readDataset(data, dataInfo, dsetInfo);
            return data;
        }

        template<typename DataType>
        void readDataset(DataType &data, const DsetInfo &dsetInfo, const Options &options = Options()) const {
            static_assert(not std::is_const_v<DataType>);
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            readDataset(data, dataInfo, dsetInfo);
        }

        template<typename DataType>
        [[nodiscard]] DataType readDataset(const DsetInfo &dsetInfo, const Options &options = Options()) const {
            static_assert(not std::is_const_v<DataType>);
            if constexpr(type::sfinae::is_specialization_v<DataType, std::optional>) {
                dsetInfo.assertReadReady();
                if(dsetInfo.dsetExists.value() or linkExists(dsetInfo.dsetPath.value()))
                    return readDataset<typename DataType::value_type>(dsetInfo, options);
                return std::nullopt;
            }
            DataType data;
            readDataset(data, dsetInfo, options);
            return data;
        }

        template<typename DataType>
        [[nodiscard]] DataType readDataset(const DsetInfo &dsetInfo, const DimsType &dataDims) const {
            static_assert(not std::is_const_v<DataType>);
            if constexpr(type::sfinae::is_specialization_v<DataType, std::optional>) {
                dsetInfo.assertReadReady();
                if(dsetInfo.dsetExists.value() or linkExists(dsetInfo.dsetPath.value()))
                    return readDataset<typename DataType::value_type>(dsetInfo, dataDims);
                return std::nullopt;
            }
            DataType data;
            Options  options;
            options.dataDims = dataDims;
            readDataset(data, dsetInfo, options);
            return data;
        }

        template<typename DataType>
        void readDataset(DataType &data, const Options &options) const {
            static_assert(not std::is_const_v<DataType>);
            options.assertWellDefined();
            // Generate the metadata for the dataset on file
            auto dsetInfo = h5pp::scan::readDsetInfo(openFileHandle(), options, plists);
            if(dsetInfo.dsetExists and not dsetInfo.dsetExists.value())
                throw h5pp::runtime_error("Cannot read dataset [{}]: It does not exist", options.linkPath.value());
            // Generate the metadata for given data
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            readDataset(data, dataInfo, dsetInfo);
        }
        template<typename DataType>
        [[nodiscard]] DataType readDataset(std::string_view dsetPath, const Options &options) const {
            static_assert(not std::is_const_v<DataType>);
            if constexpr(type::sfinae::is_specialization_v<DataType, std::optional>) {
                if(linkExists(dsetPath)) return readDataset<typename DataType::value_type>(dsetPath, options);
                return std::nullopt;
            }

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
            static_assert(not std::is_const_v<DataType>);
            Options options;
            options.linkPath = dsetPath;
            options.dataDims = dataDims;
            options.h5Type   = std::move(h5Type);
            readDataset(data, options);
        }

        template<typename DataType>
        [[nodiscard]] DataType readDataset(std::string_view        dsetPath,
                                           const OptDimsType      &dataDims = std::nullopt,
                                           std::optional<hid::h5t> h5Type   = std::nullopt) const {
            static_assert(not std::is_const_v<DataType>);
            if constexpr(type::sfinae::is_specialization_v<DataType, std::optional>) {
                if(linkExists(dsetPath)) return readDataset<typename DataType::value_type>(dsetPath, dataDims, h5Type);
                return std::nullopt;
            }

            DataType data;
            readDataset(data, dsetPath, dataDims, std::move(h5Type));
            return data;
        }

        template<typename DataType>
        void appendToDataset(const DataType &data, const DataInfo &dataInfo, DsetInfo &dsetInfo, size_t axis) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            h5pp::hdf5::extendDataset(dsetInfo, dataInfo, axis);
            h5pp::hdf5::writeDataset(data, dataInfo, dsetInfo, plists);
        }

        template<typename DataType>
        void appendToDataset(const DataType &data, DsetInfo &dsetInfo, size_t axis, const OptDimsType &dataDims = std::nullopt) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            Options options;
            options.dataDims = dataDims;
            auto dataInfo    = h5pp::scan::scanDataInfo(data, options);
            appendToDataset(data, dataInfo, dsetInfo, axis);
        }

        template<typename DataType>
        DsetInfo appendToDataset(const DataType &data, size_t axis, const Options &options = Options()) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            options.assertWellDefined();
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            auto dsetInfo = h5pp::scan::readDsetInfo(openFileHandle(), options, plists);
            appendToDataset(data, dataInfo, dsetInfo, axis);
            return dsetInfo;
        }

        template<typename DataType>
        DsetInfo appendToDataset(const DataType &data, std::string_view dsetPath, size_t axis, const OptDimsType &dataDims = std::nullopt) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            Options options;
            options.linkPath = dsetPath;
            options.dataDims = dataDims;
            return appendToDataset(data, axis, options);
        }

        template<typename DataType>
        void readHyperslab(DataType &data, std::string_view dsetPath, const Hyperslab &hyperslab) const {
            static_assert(not std::is_const_v<DataType>);
            Options options;
            options.linkPath = dsetPath;
            options.dsetSlab = hyperslab;
            readDataset(data, options);
        }

        template<typename DataType>
        [[nodiscard]] DataType readHyperslab(std::string_view dsetPath, const Hyperslab &hyperslab) const {
            static_assert(not std::is_const_v<DataType>);
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
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to create attribute on read-only file [{}]", filePath.string());
            if(attrInfo.hasLocId()) h5pp::scan::inferAttrInfo(attrInfo, attrInfo.getLocId(), options, plists);
            else h5pp::scan::inferAttrInfo(attrInfo, openFileHandle(), options, plists);
            h5pp::hdf5::createAttribute(attrInfo);
        }

        template<typename DataType>
        AttrInfo createAttribute(const DataType &data, AttrInfo &attrInfo, const Options &options = Options()) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to create attribute on read-only file [{}]", filePath.string());
            if(attrInfo.hasLocId()) h5pp::scan::inferAttrInfo(attrInfo, attrInfo.getLocId(), data, options, plists);
            else h5pp::scan::inferAttrInfo(attrInfo, openFileHandle(), data, options, plists);

            h5pp::hdf5::createAttribute(attrInfo);
            return attrInfo;
        }

        template<typename DataType>
        AttrInfo createAttribute(const DataType &data, const Options &options) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to create attribute on read-only file [{}]", filePath.string());
            auto attrInfo = h5pp::scan::inferAttrInfo(openFileHandle(), data, options, plists);
            h5pp::hdf5::createAttribute(attrInfo);
            return attrInfo;
        }

        template<typename DataType>
        AttrInfo createAttribute(const DataType &data, const DimsType &dataDims, std::string_view linkPath, std::string_view attrName) {
            Options options;
            options.linkPath = linkPath;
            options.attrName = attrName;
            options.dataDims = dataDims;
            return createAttribute(data, options);
        }

        template<typename DataType>
        void writeAttribute(const DataType &data, DataInfo &dataInfo, AttrInfo &attrInfo, const Options &options = Options()) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            if(attrInfo.hasLocId()) h5pp::scan::inferAttrInfo(attrInfo, attrInfo.getLocId(), data, options, plists);
            else h5pp::scan::inferAttrInfo(attrInfo, openFileHandle(), data, options, plists);
            h5pp::scan::scanDataInfo(dataInfo, data, options);
            h5pp::hdf5::writeAttribute(data, dataInfo, attrInfo);
        }

        template<typename DataType>
        AttrInfo writeAttribute(const DataType &data, const Options &options) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            options.assertWellDefined();
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            auto attrInfo = createAttribute(data, options);
            h5pp::hdf5::writeAttribute(data, dataInfo, attrInfo);
            return attrInfo;
        }

        template<typename DataType>
        AttrInfo writeAttribute(const DataType         &data,
                                std::string_view        linkPath,
                                std::string_view        attrName,
                                const OptDimsType      &dataDims = std::nullopt,
                                std::optional<hid::h5t> h5Type   = std::nullopt) {
            Options options;
            options.linkPath = linkPath;
            options.attrName = attrName;
            options.dataDims = dataDims;
            options.h5Type   = std::move(h5Type);
            return writeAttribute(data, options);
        }

        template<typename DataType>
        void readAttribute(DataType &data, const h5pp::AttrInfo &attrInfo, const Options &options = Options()) const {
            static_assert(not std::is_const_v<DataType>);
            if(attrInfo.linkExists.has_value() and not attrInfo.linkExists.value()) {
                throw h5pp::runtime_error("Could not read attribute [{}] in link [{}]: "
                                          "Link does not exist. "
                                          "NOTE: h5pp v1.10 and above requires the 'linkPath' argument before 'attrName'.",
                                          attrInfo.attrName.value(),
                                          attrInfo.linkPath.value());
            }

            if(attrInfo.attrExists.has_value() and not attrInfo.attrExists.value()) {
                throw h5pp::runtime_error("Could not read attribute [{}] in link [{}]: "
                                          "Attribute does not exist. "
                                          "NOTE: h5pp v1.10 and above requires the 'linkPath' argument before 'attrName'.",
                                          attrInfo.attrName.value(),
                                          attrInfo.linkPath.value());
            }
            auto dataInfo = h5pp::scan::scanDataInfo(data, options);
            h5pp::hdf5::resizeData(data, dataInfo, attrInfo);
            h5pp::hdf5::readAttribute(data, dataInfo, attrInfo, plists);
        }

        template<typename DataType>
        void readAttribute(DataType &data, const Options &options) const {
            static_assert(not std::is_const_v<DataType>);
            options.assertWellDefined();
            auto attrInfo = h5pp::scan::readAttrInfo(openFileHandle(), options, plists);
            readAttribute(data, attrInfo, options);
        }

        template<typename DataType>
        void readAttribute(DataType               &data,
                           std::string_view        linkPath,
                           std::string_view        attrName,
                           const OptDimsType      &dataDims = std::nullopt,
                           std::optional<hid::h5t> h5Type   = std::nullopt) const {
            static_assert(not std::is_const_v<DataType>);
            Options options;
            options.linkPath = linkPath;
            options.attrName = attrName;
            options.dataDims = dataDims;
            options.h5Type   = std::move(h5Type);
            readAttribute(data, options);
        }

        template<typename DataType>
        [[nodiscard]] DataType readAttribute(const Options &options) const {
            static_assert(not std::is_const_v<DataType>);
            if constexpr(type::sfinae::is_specialization_v<DataType, std::optional>) {
                options.assertWellDefined();
                if(attributeExists(options.linkPath.value(), options.attrName.value()))
                    return readAttribute<typename DataType::value_type>(options);
                return std::nullopt;
            }
            DataType data;
            readAttribute(data, options);
            return data;
        }

        template<typename DataType>
        [[nodiscard]] DataType readAttribute(std::string_view        linkPath,
                                             std::string_view        attrName,
                                             const OptDimsType      &dataDims = std::nullopt,
                                             std::optional<hid::h5t> h5Type   = std::nullopt) const {
            static_assert(not std::is_const_v<DataType>);
            if constexpr(type::sfinae::is_specialization_v<DataType, std::optional>) {
                if(attributeExists(linkPath, attrName))
                    return readAttribute<typename DataType::value_type>(linkPath, attrName, dataDims, h5Type);
                return std::nullopt;
            }
            DataType data;
            readAttribute(data, linkPath, attrName, dataDims, h5Type);
            return data;
        }

        [[nodiscard]] std::vector<std::string> getAttributeNames(std::string_view linkPath) const {
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
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            if(info.hasLocId()) h5pp::scan::inferTableInfo(info, info.getLocId(), options, plists);
            else h5pp::scan::inferTableInfo(info, openFileHandle(), options, plists);
            h5pp::hdf5::createTable(info, plists);
        }

        TableInfo createTable(const hid::h5t    &h5Type,
                              std::string_view   tablePath,
                              std::string_view   tableTitle,
                              const OptDimsType &chunkDims   = std::nullopt,
                              std::optional<int> compression = std::nullopt

        ) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            Options options;
            options.linkPath      = h5pp::util::safe_str(tablePath);
            options.h5Type        = h5Type;
            options.dsetChunkDims = chunkDims;
            options.compression   = getCompressionLevel(compression);
            auto info             = h5pp::scan::makeTableInfo(openFileHandle(), options, tableTitle, plists);
            h5pp::hdf5::createTable(info, plists);
            return info;
        }

        template<typename DataType>
        TableInfo writeTableRecords(const DataType        &data,
                                    std::string_view       tablePath,
                                    hsize_t                offset = 0,
                                    std::optional<hsize_t> extent = std::nullopt) {
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            auto info        = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            h5pp::hdf5::writeTableRecords(data, info, offset, extent);
            return info;
        }

        template<typename DataType>
        TableInfo appendTableRecords(const DataType &data, std::string_view tablePath, std::optional<hsize_t> extent = std::nullopt) {
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            auto info        = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            info.assertWriteReady(); // Check to avoid bad access on numRecords below, in case of error.
            h5pp::hdf5::writeTableRecords(data, info, info.numRecords.value(), extent);
            return info;
        }

        template<typename DataType>
        TableInfo appendTableRecords(const DataType &data, TableInfo &info, std::optional<hsize_t> extent = std::nullopt) {
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            info.assertWriteReady(); // Check to avoid bad access on numRecords below, in case of error.
            h5pp::hdf5::writeTableRecords(data, info, info.numRecords.value(), extent);
            return info;
        }

        void appendTableRecords(const h5pp::TableInfo &srcInfo, hsize_t offset, hsize_t extent, h5pp::TableInfo &tgtInfo) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            tgtInfo.assertWriteReady();
            h5pp::hdf5::copyTableRecords(srcInfo, offset, extent, tgtInfo, tgtInfo.numRecords.value(), plists);
        }

        void appendTableRecords(const h5pp::TableInfo &srcInfo,
                                h5pp::TableInfo       &tgtInfo,
                                TableSelection         srcSelection = TableSelection::ALL) {
            srcInfo.assertReadReady();
            auto [offset, extent] = util::parseTableSelection(srcSelection, srcInfo.numRecords.value());
            appendTableRecords(srcInfo, offset, extent, tgtInfo);
        }

        TableInfo appendTableRecords(const h5pp::TableInfo &srcInfo,
                                     std::string_view       tgtTablePath,
                                     TableSelection         srcSelection = TableSelection::ALL,
                                     Options                options      = Options()) {
            if(not options.linkPath) options.linkPath = tgtTablePath;
            auto tgtInfo = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            if(not tgtInfo.tableExists or not tgtInfo.tableExists.value()) {
                srcInfo.assertCreateReady();
                if(not options.h5Type) options.h5Type = srcInfo.h5Type;
                h5pp::scan::makeTableInfo(tgtInfo, openFileHandle(), options, srcInfo.tableTitle.value(), plists);
                createTable(tgtInfo, options);
            }
            appendTableRecords(srcInfo, tgtInfo, srcSelection);
            return tgtInfo;
        }

        template<typename h5x_src>
        TableInfo appendTableRecords(const h5x_src   &srcLocation,
                                     std::string_view srcTablePath,
                                     std::string_view tgtTablePath,
                                     TableSelection   srcSelection = TableSelection::ALL,
                                     Options          options      = Options()) {
            static_assert(type::sfinae::is_h5pp_loc_id<h5x_src>);
            options.linkPath = h5pp::util::safe_str(srcTablePath);
            auto srcInfo     = h5pp::scan::readTableInfo(srcLocation, options, plists);
            return appendTableRecords(srcInfo, tgtTablePath, srcSelection, options);
        }

        void copyTableRecords(const h5pp::TableInfo &srcInfo,
                              hsize_t                srcOffset,
                              hsize_t                srcExtent,
                              h5pp::TableInfo       &tgtInfo,
                              hsize_t                tgtOffset) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            if(not srcInfo.numRecords) throw h5pp::runtime_error("Source TableInfo has undefined field [numRecords]");
            srcExtent = std::min(srcInfo.numRecords.value() - srcOffset, srcExtent);
            h5pp::hdf5::copyTableRecords(srcInfo, srcOffset, srcExtent, tgtInfo, tgtOffset, plists);
        }

        void copyTableRecords(const h5pp::TableInfo &srcInfo, h5pp::TableInfo &tgtInfo, TableSelection tableSelection, hsize_t tgtOffset) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to write on read-only file [{}]", filePath.string());
            srcInfo.assertReadReady();
            auto [offset, extent] = util::parseTableSelection(tableSelection, srcInfo.numRecords.value());
            h5pp::hdf5::copyTableRecords(srcInfo, offset, extent, tgtInfo, tgtOffset, plists);
        }

        TableInfo copyTableRecords(const h5pp::TableInfo &srcInfo,
                                   std::string_view       tgtTablePath,
                                   hsize_t                tgtOffset,
                                   TableSelection         srcSelection = TableSelection::ALL,
                                   Options                options      = Options()) {
            options.linkPath = h5pp::util::safe_str(tgtTablePath);
            auto tgtInfo     = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            if(not tgtInfo.tableExists or not tgtInfo.tableExists.value()) {
                srcInfo.assertCreateReady();
                if(not options.h5Type) options.h5Type = srcInfo.h5Type;
                h5pp::scan::makeTableInfo(tgtInfo, openFileHandle(), options, srcInfo.tableTitle.value(), plists);
                createTable(tgtInfo, options);
            }
            copyTableRecords(srcInfo, tgtInfo, srcSelection, tgtOffset);
            return tgtInfo;
        }

        TableInfo copyTableRecords(const h5pp::TableInfo &srcInfo,
                                   hsize_t                srcOffset,
                                   hsize_t                srcExtent,
                                   std::string_view       tgtTablePath,
                                   hsize_t                tgtOffset,
                                   Options                options = Options()) {
            options.linkPath = h5pp::util::safe_str(tgtTablePath);
            auto tgtInfo     = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            if(not tgtInfo.tableExists or not tgtInfo.tableExists.value()) {
                srcInfo.assertCreateReady();
                if(not options.h5Type) options.h5Type = srcInfo.h5Type;
                h5pp::scan::makeTableInfo(tgtInfo, openFileHandle(), options, srcInfo.tableTitle.value(), plists);
                createTable(tgtInfo, options);
            }
            h5pp::hdf5::copyTableRecords(srcInfo, srcOffset, srcExtent, tgtInfo, tgtOffset, plists);
            return tgtInfo;
        }

        template<typename h5x_src>
        TableInfo copyTableRecords(const h5x_src   &srcLocation,
                                   std::string_view srcTablePath,
                                   std::string_view tgtTablePath,
                                   hsize_t          tgtOffset,
                                   TableSelection   srcSelection = TableSelection::ALL,
                                   Options          options      = Options()) {
            static_assert(type::sfinae::is_h5pp_loc_id<h5x_src>);
            options.linkPath = h5pp::util::safe_str(srcTablePath);
            auto srcInfo     = h5pp::scan::readTableInfo(srcLocation, options, plists);
            return copyTableRecords(srcInfo, tgtTablePath, tgtOffset, srcSelection, options);
        }

        template<typename h5x_src>
        TableInfo copyTableRecords(const h5x_src   &srcLocation,
                                   std::string_view srcTablePath,
                                   hsize_t          srcOffset,
                                   hsize_t          srcExtent,
                                   std::string_view tgtTablePath,
                                   hsize_t          tgtOffset,
                                   Options          options = Options()) {
            static_assert(type::sfinae::is_h5pp_loc_id<h5x_src>);
            options.linkPath = h5pp::util::safe_str(srcTablePath);
            auto srcInfo     = h5pp::scan::readTableInfo(srcLocation, options, plists);
            return copyTableRecords(srcInfo, srcOffset, srcExtent, tgtTablePath, tgtOffset, options);
        }

        TableInfo copyTableRecords(std::string_view srcTablePath,
                                   hsize_t          srcOffset,
                                   hsize_t          srcExtent,
                                   std::string_view tgtTablePath,
                                   hsize_t          tgtOffset,
                                   const Options   &options = Options()) {
            Options src_options;
            src_options.linkPath = h5pp::util::safe_str(srcTablePath);
            auto srcInfo         = h5pp::scan::readTableInfo(openFileHandle(), src_options, plists);
            return copyTableRecords(srcInfo, srcOffset, srcExtent, tgtTablePath, tgtOffset, options);
        }

        template<typename DataType>
        TableInfo readTableRecords(DataType              &data,
                                   std::string_view       tablePath,
                                   std::optional<hsize_t> offset = std::nullopt,
                                   std::optional<hsize_t> extent = std::nullopt) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            auto info        = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            h5pp::hdf5::readTableRecords(data, info, offset, extent, plists);
            return info;
        }

        template<typename DataType>
        TableInfo readTableRecords(DataType &data, std::string_view tablePath, TableSelection tableSelection) const {
            Options options;
            options.linkPath      = h5pp::util::safe_str(tablePath);
            auto info             = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            auto [offset, extent] = util::parseTableSelection(data, tableSelection, info.numRecords, info.recordBytes);
            h5pp::hdf5::readTableRecords(data, info, offset, extent, plists);
            return info;
        }

        template<typename DataType>
        [[nodiscard]] DataType readTableRecords(std::string_view       tablePath,
                                                std::optional<hsize_t> offset = std::nullopt,
                                                std::optional<hsize_t> extent = std::nullopt) const {
            if constexpr(type::sfinae::is_specialization_v<DataType, std::optional>) {
                if(linkExists(tablePath)) return readTableRecords<typename DataType::value_type>(tablePath, offset, extent);
                return std::nullopt;
            }

            DataType data;
            auto     info = readTableRecords(data, tablePath, offset, extent);
            if(info.reclaimInfo) reclaimStack.emplace_back(info.reclaimInfo.value());
            return data;
        }

        template<typename DataType>
        [[nodiscard]] DataType readTableRecords(std::string_view tablePath, h5pp::TableSelection tableSelection) const {
            if constexpr(type::sfinae::is_specialization_v<DataType, std::optional>) {
                if(linkExists(tablePath)) return readTableRecords<typename DataType::value_type>(tablePath, tableSelection);
                return std::nullopt;
            }
            DataType data;
            auto     info = readTableRecords(data, tablePath, tableSelection);
            if(info.reclaimInfo) reclaimStack.emplace_back(info.reclaimInfo.value());
            return data;
        }

        template<typename DataType>
        void readTableField(DataType              &data,
                            const TableInfo       &info,
                            const NamesOrIndices  &fields,
                            std::optional<hsize_t> offset = std::nullopt,
                            std::optional<hsize_t> extent = std::nullopt) const {
            if(fields.has_indices()) h5pp::hdf5::readTableField(data, info, fields.get_indices(), offset, extent, plists);
            else if(fields.has_names()) h5pp::hdf5::readTableField(data, info, fields.get_names(), offset, extent, plists);
            else throw h5pp::runtime_error("No field names or indices have been specified");
        }

        template<typename DataType>
        TableInfo readTableField(DataType              &data,
                                 std::string_view       tablePath,
                                 const NamesOrIndices  &fields,
                                 std::optional<hsize_t> offset = std::nullopt,
                                 std::optional<hsize_t> extent = std::nullopt) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            auto info        = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            readTableField(data, info, fields, offset, extent);
            return info;
        }

        template<typename DataType>
        [[nodiscard]] DataType readTableField(std::string_view       tablePath,
                                              const NamesOrIndices  &fields,
                                              std::optional<hsize_t> offset = std::nullopt,
                                              std::optional<hsize_t> extent = std::nullopt) const {
            if constexpr(type::sfinae::is_specialization_v<DataType, std::optional>) {
                if(fieldExists(tablePath, fields)) return readTableField<typename DataType::value_type>(tablePath, fields, offset, extent);
                else return std::nullopt;
            }
            DataType data;
            auto     info = readTableField(data, tablePath, fields, offset, extent);
            if(info.reclaimInfo) reclaimStack.emplace_back(info.reclaimInfo.value());
            return data;
        }

        template<typename DataType>
        void readTableField(DataType &data, std::string_view tablePath, const NamesOrIndices &fields, TableSelection tableSelection) const {
            static_assert(not std::is_const_v<DataType>);
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            Options options;
            options.linkPath = h5pp::util::safe_str(tablePath);
            auto info        = h5pp::scan::readTableInfo(openFileHandle(), options, plists);
            info.assertReadReady();
            hsize_t offset, extent;
            if(fields.has_indices()) {
                std::tie(offset, extent) = util::parseTableSelection(data, tableSelection, fields.get_indices(), info);
                readTableField(data, info, fields.get_indices(), offset, extent);
            } else if(fields.has_names()) {
                std::tie(offset, extent) = util::parseTableSelection(data, tableSelection, fields.get_names(), info);
                readTableField(data, info, fields.get_names(), offset, extent);
            } else {
                throw h5pp::runtime_error("No field names or indices have been specified");
            }
        }
        template<typename DataType>
        void readTableField(DataType &data, std::string_view tablePath, const NamesOrIndices &fields) const {
            static_assert(not std::is_const_v<DataType>);
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            static_assert(type::sfinae::has_resize_v<DataType>);
            readTableField(data, tablePath, fields, h5pp::TableSelection::ALL);
        }

        template<typename DataType>
        [[nodiscard]] DataType
            readTableField(std::string_view tablePath, const NamesOrIndices &fields, TableSelection tableSelection) const {
            static_assert(not std::is_const_v<DataType>);
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            if constexpr(type::sfinae::is_specialization_v<DataType, std::optional>) {
                if(fieldExists(tablePath, fields)) return readTableField<typename DataType::value_type>(tablePath, fields, tableSelection);
                else return std::nullopt;
            }

            DataType data;
            readTableField(data, tablePath, fields, tableSelection);
            return data;
        }
        template<typename DataType>
        [[nodiscard]] DataType readTableField(std::string_view tablePath, const NamesOrIndices &fields) const {
            static_assert(not std::is_const_v<DataType>);
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            static_assert(type::sfinae::has_resize_v<DataType>);
            return readTableField<DataType>(tablePath, fields, h5pp::TableSelection::ALL);
        }

        template<typename DataType>
        [[nodiscard]] DataType readTableField(const TableInfo &info, const hid::h5t &fieldId, hsize_t offset, hsize_t extent) const {
            static_assert(not std::is_const_v<DataType>);
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            DataType data;
            h5pp::hdf5::readTableField(data, info, fieldId, offset, extent, plists);
            return data;
        }

        template<typename DataType>
        [[nodiscard]] DataType readTableField(const TableInfo &info, const hid::h5t &fieldId, TableSelection tableSelection) const {
            static_assert(not std::is_const_v<DataType>);
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            info.assertReadReady();
            DataType data;
            auto [offset, extent] = util::parseTableSelection(data, tableSelection, fieldId, info);
            h5pp::hdf5::readTableField(data, info, fieldId, offset, extent, plists);
            return data;
        }
        template<typename DataType>
        [[nodiscard]] DataType readTableField(const TableInfo &info, const hid::h5t &fieldId) const {
            static_assert(not std::is_const_v<DataType>);
            static_assert(not type::sfinae::is_h5pp_id<DataType>);
            static_assert(type::sfinae::has_resize_v<DataType>);
            return readTableField<DataType>(info, fieldId, TableSelection::ALL);
        }

        [[nodiscard]] TableFieldInfo getTableFieldInfo(std::string_view tablePath) const {
            return h5pp::scan::getTableFieldInfo(openFileHandle(), tablePath, std::nullopt, std::nullopt, plists.dsetAccess);
        }

        /*
         *
         *
         * Functions for creating hard/soft/external links
         *
         *
         */

        void deleteLink(std::string_view linkPath) { h5pp::hdf5::deleteLink(openFileHandle(), linkPath, plists.linkAccess); }
        void deleteAttribute(std::string_view linkPath, std::string_view attrName) {
            h5pp::hdf5::deleteAttribute(openFileHandle(), linkPath, attrName, plists.linkAccess);
        }

        void createSoftLink(std::string_view targetLinkPath, std::string_view softLinkPath) {
            // It's important that targetLinkPath is a full path
            std::string targetLinkFullPath =
                targetLinkPath.front() == '/' ? std::string(targetLinkPath) : h5pp::format("/{}", targetLinkPath);
            h5pp::hdf5::createSoftLink(targetLinkFullPath, openFileHandle(), softLinkPath, plists);
        }

        void createHardLink(std::string_view targetLinkPath, std::string_view hardLinkPath) {
            std::string targetLinkFullPath =
                targetLinkPath.front() == '/' ? std::string(targetLinkPath) : h5pp::format("/{}", targetLinkPath);
            h5pp::hdf5::createHardLink(openFileHandle(), targetLinkFullPath, openFileHandle(), hardLinkPath, plists);
        }

        void createExternalLink(std::string_view targetFilePath, /*!< Path to an external hdf5 file with the desired link. If relative, it
                                                                    is relative to the current file */
                                std::string_view targetLinkPath, /*!< Full path to link within the external file */
                                std::string_view softLinkPath    /*!< Full path to the new soft link created within this file  */
        ) {
            // The given targetFilePath is written as-is into the TARGETFILE property of the new external link.
            // Therefore, it is important that it is written either as:
            //      1: a path relative to the current file, and not relative to the current process, or
            //      2: a full path
#if __cplusplus > 201703L
            if(fs::path(targetFilePath).is_relative()) {
                auto prox = fs::proximate(targetFilePath, filePath);
                if(prox != targetFilePath) {
                    h5pp::logger::log->debug("External link [{}] is not relative to the current file [{}]."
                                             "This can cause a dangling soft link");
                }
            }
#endif

            h5pp::hdf5::createExternalLink(targetFilePath, targetLinkPath, openFileHandle(), softLinkPath, plists);
        }

        /*
         *
         *
         * Functions for querying
         *
         *
         */

        [[nodiscard]] int getDatasetRank(std::string_view datasetPath) const {
            auto filehdl = openFileHandle();
            auto dataset = h5pp::hdf5::openLink<hid::h5d>(filehdl, datasetPath, std::nullopt, plists.dsetAccess);
            return h5pp::hdf5::getRank(dataset);
        }

        [[nodiscard]] std::vector<hsize_t> getDatasetDimensions(std::string_view datasetPath) const {
            auto filehdl = openFileHandle();
            auto dataset = h5pp::hdf5::openLink<hid::h5d>(filehdl, datasetPath, std::nullopt, plists.dsetAccess);
            return h5pp::hdf5::getDimensions(dataset);
        }
        [[nodiscard]] std::optional<std::vector<hsize_t>> getDatasetMaxDimensions(std::string_view datasetPath) const {
            auto filehdl = openFileHandle();
            auto dataset = h5pp::hdf5::openLink<hid::h5d>(filehdl, datasetPath, std::nullopt, plists.dsetAccess);
            return h5pp::hdf5::getMaxDimensions(dataset);
        }

        [[nodiscard]] std::optional<std::vector<hsize_t>> getDatasetChunkDimensions(std::string_view datasetPath) const {
            auto filehdl = openFileHandle();
            auto dataset = h5pp::hdf5::openLink<hid::h5d>(filehdl, datasetPath, std::nullopt, plists.dsetAccess);
            return h5pp::hdf5::getChunkDimensions(dataset);
        }

        [[nodiscard]] bool linkExists(std::string_view linkPath) const {
            return h5pp::hdf5::checkIfLinkExists(openFileHandle(), linkPath, plists.linkAccess);
        }

        [[nodiscard]] bool attributeExists(std::string_view linkPath, std::string_view attrName) const {
            return h5pp::hdf5::checkIfAttrExists(openFileHandle(), linkPath, attrName, std::nullopt, plists.linkAccess);
        }
        template<typename h5x>
        [[nodiscard]] bool attributeExists(const h5x &link, std::string_view attrName) const {
            return h5pp::hdf5::checkIfAttrExists(link, attrName, plists.linkAccess);
        }
        [[nodiscard]] bool fieldExists(std::string_view tablePath, const NamesOrIndices &fields) const {
            if(fields.has_indices()) return hdf5::checkIfTableFieldsExists(openFileHandle(), tablePath, fields.get_indices(), plists);
            else if(fields.has_names()) return hdf5::checkIfTableFieldsExists(openFileHandle(), tablePath, fields.get_names(), plists);
            return false;
        }

        [[nodiscard]] std::vector<std::string> findLinks(std::string_view searchKey      = "",
                                                         std::string_view searchRoot     = "/",
                                                         long             maxHits        = -1,
                                                         long             maxDepth       = -1,
                                                         bool             followSymlinks = false) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_UNKNOWN>(openFileHandle(),
                                                           searchKey,
                                                           searchRoot,
                                                           maxHits,
                                                           maxDepth,
                                                           followSymlinks,
                                                           plists);
        }

        [[nodiscard]] std::vector<std::string> findDatasets(std::string_view searchKey      = "",
                                                            std::string_view searchRoot     = "/",
                                                            long             maxHits        = -1,
                                                            long             maxDepth       = -1,
                                                            bool             followSymlinks = false) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_DATASET>(openFileHandle(),
                                                           searchKey,
                                                           searchRoot,
                                                           maxHits,
                                                           maxDepth,
                                                           followSymlinks,
                                                           plists);
        }

        [[nodiscard]] std::vector<std::string> findGroups(std::string_view searchKey      = "",
                                                          std::string_view searchRoot     = "/",
                                                          long             maxHits        = -1,
                                                          long             maxDepth       = -1,
                                                          bool             followSymlinks = false) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_GROUP>(openFileHandle(),
                                                         searchKey,
                                                         searchRoot,
                                                         maxHits,
                                                         maxDepth,
                                                         followSymlinks,
                                                         plists);
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
            return h5pp::hdf5::getTypeInfo(openFileHandle(), dsetPath, std::nullopt, plists.dsetAccess);
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

        [[nodiscard]] LinkInfo getLinkInfo(std::string_view linkPath) const {
            Options options;
            options.linkPath = h5pp::util::safe_str(linkPath);
            return h5pp::scan::readLinkInfo(openFileHandle(), options, plists);
        }

        template<typename InfoType>
        [[nodiscard]] InfoType getInfo(std::string_view linkPath) const {
            if constexpr(std::is_same_v<InfoType, DsetInfo>) {
                return getDatasetInfo(linkPath);
            } else if constexpr(std::is_same_v<InfoType, TableInfo>) {
                return getTableInfo(linkPath);
            } else if constexpr(std::is_same_v<InfoType, TableFieldInfo>) {
                return getTableFieldInfo(linkPath);
            } else if constexpr(std::is_same_v<InfoType, TypeInfo>) {
                return getTypeInfoDataset(linkPath);
            } else if constexpr(std::is_same_v<InfoType, LinkInfo>) {
                return getLinkInfo(linkPath);
            } else {
                static_assert(type::sfinae::invalid_type_v<InfoType>,
                              "Template function 'h5pp::File::getInfo<InfoType>(std::string_view linkPath)' "
                              "requires template type 'InfoType' to be one of "
                              "[h5pp::DsetInfo], [h5pp::TableInfo], [h5pp::TableFieldInfo], [h5pp::TypeInfo] or [h5pp::LinkInfo]");
            }
        }

        template<typename InfoType>
        [[nodiscard]] InfoType getInfo(std::string_view linkPath, std::string_view attrName) const {
            if constexpr(std::is_same_v<InfoType, AttrInfo>) {
                return getAttributeInfo(linkPath, attrName);
            } else if constexpr(std::is_same_v<InfoType, TypeInfo>) {
                return getTypeInfoAttribute(linkPath, attrName);
            } else {
                static_assert(type::sfinae::invalid_type_v<InfoType>,
                              "Template function 'h5pp::File::getInfo<InfoType>(std::string_view linkPath, std::string_view attrName)' "
                              "requires template type 'InfoType' to be either "
                              "[h5pp::AttrInfo] or [h5pp::TypeInfo]");
            }
        }

        [[nodiscard]] bool fileIsValid() const { return h5pp::hdf5::fileIsValid(filePath); }
    };
}
