#pragma once

#include "h5ppGroup.h"
#include "h5ppInitListType.h"
#include "h5ppScan.h"
#include "h5ppVersion.h"
#include <memory>

namespace h5pp::inline v2 {
    /**
     * @brief HDF5 file.
     *
     * Construct a File with a filesystem path and a FileAccess policy. The default
     * policy, RENAME, creates a new file and chooses another name if the path exists.
     * Use READWRITE to open or create a file, or READONLY to inspect an existing file.
     *
     * Use dataset(), group(), attribute(), table() and link() to access objects in the
     * file. Const File objects return objects with read-only methods. A non-const
     * File opened with READONLY access also rejects writes at runtime.
     *
     * @code{.cpp}
     * h5pp::File file("measurements.h5", h5pp::FileAccess::READWRITE);
     * file.dataset("velocity").write(std::vector<double>{1.0, 2.0});
     * file.dataset("velocity").attribute("units").write("m/s");
     * auto values = file.dataset("velocity").read<std::vector<double>>();
     * @endcode
     */
    class File {
        private:
        fs::path                        filePath;                              /*!< Full path to the file */
        h5pp::FileAccess                fileAccess = h5pp::FileAccess::RENAME; /*!< File open/create policy. */
        mutable std::optional<hid::h5f> fileHandle = std::nullopt;             /*!< Keeps a file handle alive in batch operations */
        mutable std::weak_ptr<hid::h5f> scopedFileHandle;                      /*!< A handle owned by keep-open tokens, when present. */
        mutable LogLevel                logLevel                        = LogLevel::info; /*!< Log verbosity from 0 [trace] to 6 [off] */
        bool                            logTimestamp                    = false;          /*!< Add a time stamp to console log output */
        hid::h5e                        error_stack                     = H5E_DEFAULT;    /*!< Reference to the error stack used by HDF5 */
        int                             currentCompression              = -1; /*!< Compression level (-1 is off, 0 is none, 9 is max) */
        // File copies and their objects share allocation records for explicit reclamation.
        std::shared_ptr<std::vector<ReclaimInfo::Reclaim>> reclaimStack = std::make_shared<std::vector<ReclaimInfo::Reclaim>>();

        void init() {
            h5pp::logger::setLogger("h5pp|init", logLevel, logTimestamp);
            h5pp::logger::log->debug("Initializing file object for [{}]", filePath.string());

            error_stack                          = H5Eget_current_stack();
            herr_t turnOffAutomaticErrorPrinting = H5Eset_auto2(error_stack, nullptr, nullptr);
            if(turnOffAutomaticErrorPrinting < 0) throw h5pp::runtime_error("Failed to turn off H5E error printing");

            filePath = h5pp::hdf5::createFile(filePath, fileAccess, plists);
        }

        public:
        /**
         * @brief Open the file or return a retained HDF5 file handle.
         *
         * The returned identifier owns a reference and remains valid independently of
         * this File object. Release it before moveFileTo().
         */
        [[nodiscard]] hid::h5f openFileHandle() const {
            h5pp::logger::setLogger("h5pp|" + filePath.filename().string(), logLevel, logTimestamp);
            if(fileHandle.has_value()) return fileHandle.value();
            if(auto scopedHandle = scopedFileHandle.lock()) return *scopedHandle;
            if(fileAccess == h5pp::FileAccess::READONLY) {
                h5pp::logger::log->trace("Opening file [{}] with READONLY access", filePath.string());
                hid_t fid = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, plists.fileAccess);
                if(fid < 0) throw h5pp::runtime_error("Failed to open file with read-only access [{}]", filePath.string());
                return fid;
            }

            h5pp::logger::log->trace("Opening file [{}] with READWRITE access", filePath.string());
            hid_t fid = H5Fopen(filePath.string().c_str(), H5F_ACC_RDWR, plists.fileAccess);
            if(fid < 0) throw h5pp::runtime_error("Failed to open file with read-write access [{}]", filePath.string());
            return fid;
        }

        private:
        template<typename FileType>
        requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
        friend class BasicDataset;
        template<typename FileType>
        requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
        friend class BasicAttribute;
        template<typename FileType>
        requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
        friend class BasicTable;
        template<typename FileType>
        requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
        friend class BasicGroup;
        template<typename FileType>
        requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
        friend class BasicLink;

        public:
        /**
         * @brief HDF5 property lists used by file operations and copied into newly obtained objects.
         * Changes do not update previously obtained Dataset, Attribute, Table, Group or Link objects.
         */
        PropertyLists plists = PropertyLists();

        /**
         * @brief Keep a shared file handle open for the lifetime of this token.
         * Tokens are movable, not copyable. Nested tokens share the retained handle.
         * @see keepFileOpen()
         */
        struct KeepFileOpenToken {
            std::shared_ptr<hid::h5f> pin_;

            //! Retain the file handle, sharing it with existing tokens when possible.
            explicit KeepFileOpenToken(const File &file) : pin_(file.scopedFileHandle.lock()) {
                if(not pin_) {
                    pin_                  = std::make_shared<hid::h5f>(file.openFileHandle());
                    file.scopedFileHandle = pin_;
                }
            }

            //! Disallow copying a token.
            KeepFileOpenToken(const KeepFileOpenToken &)                = delete;
            //! Disallow copying a token.
            KeepFileOpenToken &operator=(const KeepFileOpenToken &)     = delete;
            //! Transfer ownership of the retained handle.
            KeepFileOpenToken(KeepFileOpenToken &&) noexcept            = default;
            //! Transfer ownership of the retained handle.
            KeepFileOpenToken &operator=(KeepFileOpenToken &&) noexcept = default;
        };
        using FileHandleToken = KeepFileOpenToken; /*!< Alias for KeepFileOpenToken. */

        using Advanced = File; /*!< Alias for File, used by advanced(). */

        /**
         * @brief Set the HDF5 file close degree for subsequent opens.
         *
         * A cached handle owned by this File is reopened. Independently retained handles
         * and objects keep their existing access properties.
         *
         * @param degree HDF5 close behavior for outstanding object identifiers.
         */
        void setCloseDegree(H5F_close_degree_t degree) {
            if(plists.fileAccess == H5P_DEFAULT) plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fclose_degree(plists.fileAccess, degree);
            if(fileHandle.has_value()) {
                fileHandle = std::nullopt;
                fileHandle = openFileHandle();
            }
        }

        /**
         * @brief Select the HDF5 in-memory file driver for subsequent opens.
         * @param writeOnClose Write the in-memory file to disk when it is closed.
         * @param bytesPerMalloc Increment in bytes used to grow the in-memory file.
         */
        void setDriver_core(bool writeOnClose = false, size_t bytesPerMalloc = 10240000) {
            if(plists.fileAccess == H5P_DEFAULT) plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fapl_core(plists.fileAccess, bytesPerMalloc, static_cast<hbool_t>(writeOnClose));
            if(fileHandle.has_value()) {
                fileHandle = std::nullopt;
                fileHandle = openFileHandle();
            }
        }

        //! Select the HDF5 POSIX file driver for subsequent opens.
        void setDriver_sec2() {
            if(plists.fileAccess == H5P_DEFAULT) plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fapl_sec2(plists.fileAccess);
            if(fileHandle.has_value()) {
                fileHandle = std::nullopt;
                fileHandle = openFileHandle();
            }
        }

        //! Select the HDF5 standard C I/O file driver for subsequent opens.
        void setDriver_stdio() {
            if(plists.fileAccess == H5P_DEFAULT) plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fapl_stdio(plists.fileAccess);
            if(fileHandle.has_value()) {
                fileHandle = std::nullopt;
                fileHandle = openFileHandle();
            }
        }

#ifdef H5PP_USE_MPI
        /**
         * @brief Select the parallel HDF5 file driver for subsequent opens.
         * @param comm MPI communicator for file access.
         * @param info MPI I/O hints. Requires an MPI-enabled h5pp and HDF5 build.
         */
        void setDriver_mpio(MPI_Comm comm, MPI_Info info) {
            plists.fileAccess = H5Fget_access_plist(openFileHandle());
            H5Pset_fapl_mpio(plists.fileAccess, comm, info);
            if(fileHandle.has_value()) {
                fileHandle = std::nullopt;
                fileHandle = openFileHandle();
            }
        }
#endif

        //! Return this File for direct access to HDF5 handles and property lists.
        [[nodiscard]] File &getLowLevelFile() & { return *this; }
        //! Disallow obtaining a reference to a temporary File.
        File               &getLowLevelFile() && = delete;

        /**
         * @brief Free tracked HDF5 variable-length allocations and clear their records.
         *
         * Call only after the values referring to these allocations are no longer needed.
         * Copies of this File and its Table objects share the allocation records.
         * This does not free memory owned by std::string, vstr_t or vlen_t objects.
         */
        void vlenReclaim() const {
            if(not reclaimStack) return;
            for(auto &item : *reclaimStack) item.reclaim();
            reclaimStack->clear();
        }

        /**
         * @brief Discard tracked allocation records without freeing their memory.
         * Use when the caller has taken responsibility for reclaiming those allocations.
         */
        void vlenDropReclaims() const {
            if(not reclaimStack) return;
            for(auto &item : *reclaimStack) item.drop();
            reclaimStack->clear();
        }

        //! Enable tracking of HDF5 variable-length allocations for subsequently obtained objects.
        void vlenEnableReclaimsTracking() { plists.vlenTrackReclaims = true; }
        //! Disable tracking for subsequently obtained objects. Existing allocation records are retained.
        void vlenDisableReclaimsTracking() { plists.vlenTrackReclaims = false; }

        //! Construct an uninitialized File with no path. Assign an initialized File before file operations.
        File() = default;

        /**
         * @brief Open or create an HDF5 file according to a FileAccess policy.
         * @param filePath_ Filesystem path. Missing parent directories are created for new files.
         * @param fileAccess_ Open/create policy; RENAME avoids overwriting an existing file.
         * @param logLevel_ LogLevel value or integer from 0 (trace) to 6 (off).
         * @param logTimestamp_ Include timestamps in log output.
         * @param plists_ HDF5 property lists copied into this File.
         */
        template<typename LogLevelType = LogLevel>
        explicit File(h5pp::fs::path       filePath_,
                      h5pp::FileAccess     fileAccess_   = h5pp::FileAccess::RENAME,
                      LogLevelType         logLevel_     = LogLevel::info,
                      bool                 logTimestamp_ = false,
                      const PropertyLists &plists_       = PropertyLists())
            : filePath(std::move(filePath_)), fileAccess(fileAccess_), logLevel(Num2Level(logLevel_)), logTimestamp(logTimestamp_),
              plists(plists_) {
            init();
        }

        /**
         * @brief Open or create a file using HDF5 access flags.
         * @param filePath_ Filesystem path.
         * @param H5F_ACC_FLAGS HDF5 flags converted to a FileAccess policy.
         * @param logLevel_ LogLevel value or integer from 0 (trace) to 6 (off).
         * @param logTimestamp_ Include timestamps in log output.
         * @param plists_ HDF5 property lists copied into this File.
         */
        template<typename LogLevelType = LogLevel>
        explicit File(h5pp::fs::path       filePath_,
                      unsigned int         H5F_ACC_FLAGS,
                      LogLevelType         logLevel_     = LogLevel::info,
                      bool                 logTimestamp_ = false,
                      const PropertyLists &plists_       = PropertyLists())
            : filePath(std::move(filePath_)), logLevel(Num2Level(logLevel_)), logTimestamp(logTimestamp_), plists(plists_) {
            fileAccess = h5pp::hdf5::convertFileAccessFlags(H5F_ACC_FLAGS);
            init();
        }

        /**
         * @brief Refer to a dataset without creating it.
         * @param dsetPath Path relative to the file root.
         * @return An object with write methods; writes require a writable file.
         */
        Dataset        dataset(std::string_view dsetPath) { return Dataset(*this, dsetPath); }
        /**
         * @brief Refer to a dataset without creating it.
         * @param dsetPath Path relative to the file root.
         * @return An object with read-only methods.
         */
        ConstDataset   dataset(std::string_view dsetPath) const { return ConstDataset(*this, dsetPath); }
        /**
         * @brief Refer to a group without creating it.
         * @param groupPath Path relative to the file root. The default "/" refers to the root group.
         * @return An object with write methods; writes require a writable file.
         */
        Group          group(std::string_view groupPath = "/") { return Group(*this, groupPath); }
        /**
         * @brief Refer to a group without creating it.
         * @param groupPath Path relative to the file root. The default "/" refers to the root group.
         * @return An object with read-only methods.
         */
        ConstGroup     group(std::string_view groupPath = "/") const { return ConstGroup(*this, groupPath); }
        /**
         * @brief Refer to a link without creating it.
         * @param linkPath Path relative to the file root.
         * @return An object with write methods; writes require a writable file.
         */
        Link           link(std::string_view linkPath) { return Link(*this, linkPath); }
        /**
         * @brief Refer to a link without creating it.
         * @param linkPath Path relative to the file root.
         * @return An object with read-only methods.
         */
        ConstLink      link(std::string_view linkPath) const { return ConstLink(*this, linkPath); }
        /**
         * @brief Refer to a table without creating it.
         * @param tablePath Path relative to the file root.
         * @return An object with write methods; writes require a writable file.
         */
        Table          table(std::string_view tablePath) { return Table(*this, tablePath); }
        /**
         * @brief Refer to a table without creating it.
         * @param tablePath Path relative to the file root.
         * @return An object with read-only methods.
         */
        ConstTable     table(std::string_view tablePath) const { return ConstTable(*this, tablePath); }
        //! Refer to a root-group attribute.
        Attribute      attribute(std::string_view attrName) { return attribute("/", attrName); }
        //! Read-only root-group attribute.
        ConstAttribute attribute(std::string_view attrName) const { return attribute("/", attrName); }

        /**
         * @brief Refer to an attribute without creating it.
         * @param linkPath Path relative to the file root.
         * @param attrName Attribute name on the specified object.
         * @return An object with write methods; writes require a writable file.
         */
        Attribute      attribute(std::string_view linkPath, std::string_view attrName) { return Attribute(*this, linkPath, attrName); }
        /**
         * @brief Refer to an attribute without creating it.
         * @param linkPath Path relative to the file root.
         * @param attrName Attribute name on the specified object.
         * @return An object with read-only methods.
         */
        ConstAttribute attribute(std::string_view linkPath, std::string_view attrName) const {
            return ConstAttribute(*this, linkPath, attrName);
        }
        //! Return this File. This accessor does not change access permissions.
        [[nodiscard]] Advanced &advanced() & { return *this; }
        //! Move this File into a returned value.
        [[nodiscard]] Advanced  advanced()  &&{ return std::move(*this); }

        /**
         * @brief Keep the file open until the returned token is destroyed.
         *
         * Operations reuse the retained handle. Tokens and objects obtained from the file
         * may extend its lifetime independently.
         *
         * @code{.cpp}
         * {
         *     auto token = file.keepFileOpen();
         *     file.dataset("x").write(1.0);
         *     file.dataset("y").write(2.0);
         * }
         * @endcode
         */
        [[nodiscard]] KeepFileOpenToken keepFileOpen() const { return KeepFileOpenToken(*this); }
        //! Return a keep-open token. Equivalent to keepFileOpen().
        [[nodiscard]] FileHandleToken   getFileHandleToken() const { return keepFileOpen(); }

        /**
         * @brief Copy file contents to a destination without changing this File's path.
         *
         * Root attributes containing HDF5 object or region references are skipped,
         * with a critical log message identifying each omitted attribute.
         *
         * @param targetFilePath Destination filesystem path.
         * @param perm Destination collision policy; COLLISION_FAIL rejects an existing destination.
         * @return Actual destination path, which may differ when using RENAME.
         */
        [[nodiscard]] fs::path copyFileTo(const h5pp::fs::path &targetFilePath, const FileAccess &perm = FileAccess::COLLISION_FAIL) const {
            return h5pp::hdf5::copyFile(getFilePath(), targetFilePath, perm, plists);
        }

        /**
         * @brief Move file contents and update this File's path.
         *
         * Release Dataset, Attribute, Table, Group, Link, keep-open tokens and independent
         * HDF5 identifiers first. The move throws if outstanding identifiers keep the source
         * open, or if this File has READONLY access.
         *
         * Moving uses copy followed by removal. Root attributes containing HDF5 object
         * or region references are skipped with a critical log message and are lost
         * when the source file is removed.
         *
         * @param targetFilePath Destination filesystem path.
         * @param perm Destination collision policy; COLLISION_FAIL rejects an existing destination.
         * @return Actual destination path.
         */
        [[nodiscard]] fs::path moveFileTo(const h5pp::fs::path &targetFilePath, const FileAccess &perm = FileAccess::COLLISION_FAIL) {
            if(fileAccess == FileAccess::READONLY) throw h5pp::runtime_error("Cannot move read-only file [{}]", filePath.string());
            // Release only our own cache; independently held identifiers must prevent the move.
            if(fileHandle.has_value() and fileHandle->refcount() == 1 and H5Fget_obj_count(fileHandle.value(), H5F_OBJ_ALL) == 1)
                fileHandle.reset();
            auto newPath = h5pp::hdf5::moveFile(getFilePath(), targetFilePath, perm, plists);
            if(fs::exists(newPath)) filePath = newPath;
            return newPath;
        }

        /** @deprecated Use file.link(localLinkPath).copyToFile(targetFilePath, targetLinkPath, perm) instead. */
        [[deprecated("Use file.link(localLinkPath).copyToFile(targetFilePath, targetLinkPath, perm) instead")]]
        void copyLinkToFile(std::string_view      localLinkPath,
                            const h5pp::fs::path &targetFilePath,
                            std::string_view      targetLinkPath,
                            const FileAccess     &perm = FileAccess::READWRITE) const {
            link(localLinkPath).copyToFile(targetFilePath, targetLinkPath, perm);
        }

        /** @deprecated Use file.link(localLinkPath).copyFromFile(sourceFilePath, sourceLinkPath) instead. */
        [[deprecated("Use file.link(localLinkPath).copyFromFile(sourceFilePath, sourceLinkPath) instead")]]
        void copyLinkFromFile(std::string_view localLinkPath, const h5pp::fs::path &sourceFilePath, std::string_view sourceLinkPath) {
            link(localLinkPath).copyFromFile(sourceFilePath, sourceLinkPath);
        }

        /** @deprecated Use file.link(localLinkPath).copyToLocation(targetLocationId, targetLinkPath) instead. */
        template<typename h5x_tgt>
        [[deprecated("Use file.link(localLinkPath).copyToLocation(targetLocationId, targetLinkPath) instead")]]
        void copyLinkToLocation(std::string_view localLinkPath, const h5x_tgt &targetLocationId, std::string_view targetLinkPath) const {
            link(localLinkPath).copyToLocation(targetLocationId, targetLinkPath);
        }

        /** @deprecated Use file.link(localLinkPath).copyFromLocation(sourceLocationId, sourceLinkPath) instead. */
        template<typename h5x_src>
        [[deprecated("Use file.link(localLinkPath).copyFromLocation(sourceLocationId, sourceLinkPath) instead")]]
        void copyLinkFromLocation(std::string_view localLinkPath, const h5x_src &sourceLocationId, std::string_view sourceLinkPath) {
            link(localLinkPath).copyFromLocation(sourceLocationId, sourceLinkPath);
        }

        /** @deprecated Use file.link(localLinkPath).moveToFile(targetFilePath, targetLinkPath, perm) instead. */
        [[deprecated("Use file.link(localLinkPath).moveToFile(targetFilePath, targetLinkPath, perm) instead")]]
        void moveLinkToFile(std::string_view      localLinkPath,
                            const h5pp::fs::path &targetFilePath,
                            std::string_view      targetLinkPath,
                            const FileAccess     &perm = FileAccess::READWRITE) {
            link(localLinkPath).moveToFile(targetFilePath, targetLinkPath, perm);
        }

        /** @deprecated Use file.link(localLinkPath).moveFromFile(sourceFilePath, sourceLinkPath) instead. */
        [[deprecated("Use file.link(localLinkPath).moveFromFile(sourceFilePath, sourceLinkPath) instead")]]
        void moveLinkFromFile(std::string_view localLinkPath, const h5pp::fs::path &sourceFilePath, std::string_view sourceLinkPath) {
            link(localLinkPath).moveFromFile(sourceFilePath, sourceLinkPath);
        }

        /** @deprecated Use file.link(localLinkPath).moveToLocation(targetLocationId, targetLinkPath, locMode) instead. */
        template<typename h5x_tgt>
        [[deprecated("Use file.link(localLinkPath).moveToLocation(targetLocationId, targetLinkPath, locMode) instead")]]
        void moveLinkToLocation(std::string_view localLinkPath,
                                const h5x_tgt   &targetLocationId,
                                std::string_view targetLinkPath,
                                LocationMode     locMode = LocationMode::DETECT) {
            link(localLinkPath).moveToLocation(targetLocationId, targetLinkPath, locMode);
        }

        /** @deprecated Use file.link(localLinkPath).moveFromLocation(sourceLocationId, sourceLinkPath, locMode) instead. */
        template<typename h5x_src>
        [[deprecated("Use file.link(localLinkPath).moveFromLocation(sourceLocationId, sourceLinkPath, locMode) instead")]]
        void moveLinkFromLocation(std::string_view localLinkPath,
                                  const h5x_src   &sourceLocationId,
                                  std::string_view sourceLinkPath,
                                  LocationMode     locMode = LocationMode::DETECT) {
            link(localLinkPath).moveFromLocation(sourceLocationId, sourceLinkPath, locMode);
        }

        /**
         * @brief Resize a dataset described by open metadata.
         * @param info Dataset metadata, updated after resizing.
         * @param newDimensions Requested dimensions within the dataset's maximum dimensions.
         * @param modeOverride Override the metadata's resize policy.
         * @see BasicDataset::resize()
         */
        void resizeDataset(DsetInfo &info, const DimsType &newDimensions, std::optional<h5pp::ResizePolicy> modeOverride = std::nullopt) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to resize dataset on read-only file [{}]", filePath.string());
            h5pp::hdf5::resizeDataset(info, newDimensions, modeOverride);
        }

        /** @deprecated Use file.dataset(path).resize(newDimensions, resizePolicy) instead. */
        [[deprecated("Use file.dataset(path).resize(newDimensions, resizePolicy) instead")]] [[nodiscard]] DsetInfo
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

        // Widely used v1 API calls remain available to ease migration to the v2 API.
        /** @deprecated Use file.dataset(path).write(data, options) instead. */
        template<typename DataType>
        [[deprecated("Use file.dataset(path).write(data, options) instead")]]
        void writeDataset(const DataType &data, std::string_view dsetPath, const DatasetWriteOptions &options = DatasetWriteOptions()) {
            dataset(dsetPath).write(data, options);
        }

        /** @deprecated Use file.dataset(path).create(createOptions) instead. */
        [[deprecated("Use file.dataset(path).create(createOptions) instead")]] [[nodiscard]] DsetInfo
            createDataset(std::string_view dsetPath, const DatasetCreateOptions &createOptions) {
            return dataset(dsetPath).create(createOptions).getInfo();
        }

        /** @deprecated Use file.dataset(path).create(data, createOptions).write(data, writeOptions) instead. */
        template<typename DataType>
        [[deprecated("Use file.dataset(path).create(data, createOptions).write(data, writeOptions) instead")]]
        void writeDataset(const DataType             &data,
                          std::string_view            dsetPath,
                          const DatasetCreateOptions &createOptions,
                          const DatasetWriteOptions  &writeOptions = DatasetWriteOptions()) {
            auto creation = createOptions;
            if(writeOptions.dims.has_value()) creation.dims = writeOptions.dims;
            if(writeOptions.h5Type.has_value()) creation.h5Type = writeOptions.h5Type;
            auto options = writeOptions;
            if(not options.dims.has_value()) options.dims = creation.dims;
            dataset(dsetPath).create(data, creation).write(data, options);
        }

        /** @deprecated Use file.dataset(path).append(data, axis, options) instead. */
        template<typename DataType>
        [[deprecated("Use file.dataset(path).append(data, axis, options) instead")]] [[nodiscard]] DsetInfo
            appendToDataset(const DataType             &data,
                            std::string_view            dsetPath,
                            size_t                      axis,
                            const DatasetAppendOptions &options = DatasetAppendOptions()) {
            return dataset(dsetPath).append(data, axis, options).getInfo();
        }

        /** @deprecated Use file.dataset(path).readInto(data, options) instead. */
        template<typename DataType>
        [[deprecated("Use file.dataset(path).readInto(data, options) instead")]]
        void readDataset(DataType &data, std::string_view dsetPath, const DatasetReadOptions &options = DatasetReadOptions()) const {
            dataset(dsetPath).readInto(data, options);
        }

        /** @deprecated Use file.dataset(path).read<DataType>(options) instead. */
        template<typename DataType>
        [[deprecated("Use file.dataset(path).read<DataType>(options) instead")]] [[nodiscard]] DataType
            readDataset(std::string_view dsetPath, const DatasetReadOptions &options = DatasetReadOptions()) const {
            return dataset(dsetPath).template read<DataType>(options);
        }

        /** @deprecated Use file.attribute(linkPath, attrName).write(data, options) instead. */
        template<typename DataType>
        [[deprecated("Use file.attribute(linkPath, attrName).write(data, options) instead")]]
        void writeAttribute(const DataType              &data,
                            std::string_view             linkPath,
                            std::string_view             attrName,
                            const AttributeWriteOptions &options = AttributeWriteOptions()) {
            attribute(linkPath, attrName).write(data, options);
        }

        /** @deprecated Use file.attribute(linkPath, attrName).read<DataType>(options) instead. */
        template<typename DataType>
        [[deprecated("Use file.attribute(linkPath, attrName).read<DataType>(options) instead")]] [[nodiscard]] DataType
            readAttribute(std::string_view            linkPath,
                          std::string_view            attrName,
                          const AttributeReadOptions &options = AttributeReadOptions()) const {
            return attribute(linkPath, attrName).template read<DataType>(options);
        }

        /**
         * @brief Flush HDF5 file buffers and collect unused HDF5 memory.
         * Also prints the current HDF5 error stack to stderr.
         */
        void flush() {
            H5Fflush(openFileHandle(), H5F_scope_t::H5F_SCOPE_GLOBAL);
            h5pp::logger::log->trace("Flushing caches");
            H5garbage_collect();
            H5Eprint(H5E_DEFAULT, stderr);
        }

        /** @deprecated Use file.group(groupPath).create() instead. */
        [[deprecated("Use file.group(groupPath).create() instead")]]
        void createGroup(std::string_view groupPath) {
            group(groupPath).create();
        }
        /** @deprecated Use file.link(linkPath).deleteLink() instead. */
        [[deprecated("Use file.link(linkPath).deleteLink() instead")]]
        void deleteLink(std::string_view linkPath) {
            link(linkPath).deleteLink();
        }

        /** @deprecated Use file.link(targetLinkPath).createSoftLink(softLinkPath) instead. */
        [[deprecated("Use file.link(targetLinkPath).createSoftLink(softLinkPath) instead")]]
        void createSoftLink(std::string_view targetLinkPath, std::string_view softLinkPath) {
            link(targetLinkPath).createSoftLink(softLinkPath);
        }

        /** @deprecated Use file.link(softLinkPath).createExternalLink(targetFilePath, targetLinkPath) instead. */
        [[deprecated("Use file.link(softLinkPath).createExternalLink(targetFilePath, targetLinkPath) instead")]]
        void createExternalLink(std::string_view targetFilePath, std::string_view targetLinkPath, std::string_view softLinkPath) {
            link(softLinkPath).createExternalLink(targetFilePath, targetLinkPath);
        }

        /**
         * @brief Cache a file handle for reuse across operations.
         * The cache remains until setKeepFileClosed() or File destruction. For a scoped
         * lifetime, use keepFileOpen().
         */
        void setKeepFileOpened() const {
            if(not fileHandle.has_value()) fileHandle = openFileHandle();
        }

        /**
         * @brief Release this File's cached handle.
         * Handles retained by tokens and Dataset, Attribute, Table, Group or Link objects remain open.
         */
        void setKeepFileClosed() const { fileHandle = std::nullopt; }

        /**
         * @brief Set the default compression for subsequently obtained Dataset objects.
         * @param compressionZeroToNine Deflate level from 0 (no compression) to 9 (maximum).
         */
        void setCompressionLevel(unsigned int compressionZeroToNine) {
            currentCompression = h5pp::hdf5::getValidCompressionLevel(compressionZeroToNine);
        }

        //! Return the configured default compression level; -1 means disabled.
        [[nodiscard]] int getCompressionLevel() const { return currentCompression; }
        /**
         * @brief Resolve a compression override or return the configured default.
         * @param compression Optional level, validated and limited to the supported range.
         */
        [[nodiscard]] int getCompressionLevel(const std::optional<int> compression) const {
            if(compression.has_value()) return h5pp::hdf5::getValidCompressionLevel(compression.value());
            return currentCompression;
        }

        //! Return the file open/create policy.
        [[nodiscard]] h5pp::FileAccess getFileAccess() const { return fileAccess; }
        //! Return the filename without its parent directories.
        [[nodiscard]] std::string      getFileName() const { return filePath.filename().string(); }
        //! Return the filesystem path used by this File.
        [[nodiscard]] std::string      getFilePath() const { return filePath.string(); }
        //! Return this File's logging verbosity.
        [[nodiscard]] LogLevel         getLogLevel() const { return logLevel; }
        /** @deprecated Use file.link(linkPath).exists() instead. */
        [[deprecated("Use file.link(linkPath).exists() instead")]] [[nodiscard]] bool linkExists(std::string_view linkPath) const {
            return h5pp::hdf5::checkIfLinkExists(openFileHandle(), linkPath, plists.linkAccess);
        }
        /** @deprecated Use file.attribute(linkPath, attrName).exists() instead. */
        [[deprecated("Use file.attribute(linkPath, attrName).exists() instead")]] [[nodiscard]] bool
            attributeExists(std::string_view linkPath, std::string_view attrName) const {
            return h5pp::hdf5::checkIfAttrExists(openFileHandle(), linkPath, attrName, std::nullopt, plists.linkAccess);
        }

        /** @deprecated Use file.table(tablePath).fieldExists(fields) instead. */
        template<typename Fields>
        [[deprecated("Use file.table(tablePath).fieldExists(fields) instead")]] [[nodiscard]] bool fieldExists(std::string_view tablePath,
                                                                                                               const Fields &fields) const {
            return table(tablePath).fieldExists(fields);
        }

        /**
         * @brief Find links below a group in the file.
         * @param searchKey Substring to match in each link name. An empty string matches all names.
         * @param searchRoot Group path relative to the file root.
         * @param maxHits Maximum number of matches; -1 means unlimited.
         * @param maxDepth Maximum search depth; 0 searches immediate children, -1 means unlimited.
         * @param followSymlinks Follow soft and external links when searching.
         * @return Paths relative to searchRoot.
         */
        [[nodiscard]] std::vector<std::string> findLinks(std::string_view searchKey      = "",
                                                         std::string_view searchRoot     = "/",
                                                         long             maxHits        = -1,
                                                         long             maxDepth       = -1,
                                                         bool             followSymlinks = false) const {
            return group(searchRoot).findLinks(searchKey, maxHits, maxDepth, followSymlinks);
        }

        /**
         * @brief Find datasets below a group in the file.
         * @param searchKey Substring to match in each link name. An empty string matches all names.
         * @param searchRoot Group path relative to the file root.
         * @param maxHits Maximum number of matches; -1 means unlimited.
         * @param maxDepth Maximum search depth; 0 searches immediate children, -1 means unlimited.
         * @param followSymlinks Follow soft and external links when searching.
         * @return Paths relative to searchRoot.
         */
        [[nodiscard]] std::vector<std::string> findDatasets(std::string_view searchKey      = "",
                                                            std::string_view searchRoot     = "/",
                                                            long             maxHits        = -1,
                                                            long             maxDepth       = -1,
                                                            bool             followSymlinks = false) const {
            return group(searchRoot).findDatasets(searchKey, maxHits, maxDepth, followSymlinks);
        }

        /**
         * @brief Find groups below a group in the file.
         * @param searchKey Substring to match in each link name. An empty string matches all names.
         * @param searchRoot Group path relative to the file root.
         * @param maxHits Maximum number of matches; -1 means unlimited.
         * @param maxDepth Maximum search depth; 0 searches immediate children, -1 means unlimited.
         * @param followSymlinks Follow soft and external links when searching.
         * @return Paths relative to searchRoot.
         */
        [[nodiscard]] std::vector<std::string> findGroups(std::string_view searchKey      = "",
                                                          std::string_view searchRoot     = "/",
                                                          long             maxHits        = -1,
                                                          long             maxDepth       = -1,
                                                          bool             followSymlinks = false) const {
            return group(searchRoot).findGroups(searchKey, maxHits, maxDepth, followSymlinks);
        }

        /** @deprecated Use file.link(path).getAttributeNames() instead. */
        [[deprecated("Use file.link(path).getAttributeNames() instead")]] [[nodiscard]] std::vector<std::string>
            getAttributeNames(std::string_view linkPath) const {
            return link(linkPath).getAttributeNames();
        }

        /** @deprecated Use file.dataset(path).getInfo() instead. */
        [[deprecated("Use file.dataset(path).getInfo() instead")]] [[nodiscard]] DsetInfo getDatasetInfo(std::string_view dsetPath) const {
            return dataset(dsetPath).getInfo();
        }

        /** @deprecated Use file.attribute(linkPath, attrName).getInfo() instead. */
        [[deprecated("Use file.attribute(linkPath, attrName).getInfo() instead")]] [[nodiscard]] AttrInfo
            getAttributeInfo(std::string_view linkPath, std::string_view attrName) const {
            return attribute(linkPath, attrName).getInfo();
        }

        /** @deprecated Use file.table(path).getInfo() instead. */
        [[deprecated("Use file.table(path).getInfo() instead")]] [[nodiscard]] TableInfo getTableInfo(std::string_view tablePath) const {
            return table(tablePath).getInfo();
        }

        /** @deprecated Use file.dataset(path).getChunkDimensions() instead. */
        [[deprecated("Use file.dataset(path).getChunkDimensions() instead")]] [[nodiscard]] std::optional<std::vector<hsize_t>>
            getDatasetChunkDimensions(std::string_view datasetPath) const {
            return dataset(datasetPath).getChunkDimensions();
        }

        /** @deprecated Use file.table(path).getFieldInfo() instead. */
        [[deprecated("Use file.table(path).getFieldInfo() instead")]] [[nodiscard]] TableFieldInfo
            getTableFieldInfo(std::string_view tablePath) const {
            return table(tablePath).getFieldInfo();
        }

        /** @deprecated Use file.dataset(path).getTypeInfo() instead. */
        [[deprecated("Use file.dataset(path).getTypeInfo() instead")]] [[nodiscard]] TypeInfo
            getTypeInfoDataset(std::string_view dsetPath) const {
            return dataset(dsetPath).getTypeInfo();
        }

        /** @deprecated Use file.attribute(linkPath, attrName).getTypeInfo() instead. */
        [[deprecated("Use file.attribute(linkPath, attrName).getTypeInfo() instead")]] [[nodiscard]] TypeInfo
            getTypeInfoAttribute(std::string_view linkPath, std::string_view attrName) const {
            return attribute(linkPath, attrName).getTypeInfo();
        }

        /** @deprecated Use file.link(path).getTypeInfoAttributes() instead. */
        [[deprecated("Use file.link(path).getTypeInfoAttributes() instead")]] [[nodiscard]] std::vector<TypeInfo>
            getTypeInfoAttributes(std::string_view linkPath) const {
            return link(linkPath).getTypeInfoAttributes();
        }

        /** @deprecated Use file.link(path).getInfo() instead. */
        [[deprecated("Use file.link(path).getInfo() instead")]] [[nodiscard]] LinkInfo getLinkInfo(std::string_view linkPath) const {
            return link(linkPath).getInfo();
        }

        /** @deprecated Use file.table(path).create(h5Type, title, options) instead. */
        [[deprecated("Use file.table(path).create(h5Type, title, options) instead")]] [[nodiscard]] TableInfo
            createTable(const hid::h5t           &h5Type,
                        std::string_view          tablePath,
                        std::string_view          tableTitle,
                        const TableCreateOptions &options = TableCreateOptions()) {
            return table(tablePath).create(h5Type, tableTitle, options).getInfo();
        }

        /** @deprecated Use file.table(path).appendRecords(data, extent) instead. */
        template<typename DataType>
        [[deprecated("Use file.table(path).appendRecords(data, extent) instead")]] [[nodiscard]] TableInfo
            appendTableRecords(const DataType &data, std::string_view tablePath, std::optional<hsize_t> extent = std::nullopt) {
            auto tgtTable = table(tablePath);
            tgtTable.appendRecords(data, extent);
            return tgtTable.getInfo();
        }

        /** @deprecated Use file.table(targetPath).create(options).appendRecordsFrom(srcLocation, srcTablePath, selection) instead. */
        template<typename h5x_src>
        [[deprecated("Use file.table(targetPath).create(options).appendRecordsFrom(srcLocation, srcTablePath, selection) instead")]]
        [[nodiscard]] TableInfo
            appendTableRecords(const h5x_src            &srcLocation,
                               std::string_view          srcTablePath,
                               std::string_view          tgtTablePath,
                               TableSelection            srcSelection = TableSelection::ALL,
                               const TableCreateOptions &options      = TableCreateOptions()) {
            Options sourceOptions;
            sourceOptions.linkPath = h5pp::util::safe_str(srcTablePath);
            auto source            = h5pp::scan::readTableInfo(srcLocation, sourceOptions, plists);
            auto creation          = options;
            if(not creation.h5Type.has_value()) creation.h5Type = source.h5Type;
            if(not creation.title.has_value()) creation.title = source.tableTitle;
            return table(tgtTablePath).create(creation).appendRecordsFrom(source, srcSelection).getInfo();
        }

        /** @deprecated Use file.table(path).writeRecords(data, offset, extent) instead. */
        template<typename DataType>
        [[deprecated("Use file.table(path).writeRecords(data, offset, extent) instead")]] [[nodiscard]] TableInfo
            writeTableRecords(const DataType        &data,
                              std::string_view       tablePath,
                              hsize_t                offset = 0,
                              std::optional<hsize_t> extent = std::nullopt) {
            auto tgtTable = table(tablePath);
            tgtTable.writeRecords(data, offset, extent);
            return tgtTable.getInfo();
        }

        /** @deprecated Use file.table(path).readRecords<DataType>(offset, extent) instead. */
        template<typename DataType>
        [[deprecated("Use file.table(path).readRecords<DataType>(offset, extent) instead")]] [[nodiscard]] DataType
            readTableRecords(std::string_view       tablePath,
                             std::optional<hsize_t> offset = std::nullopt,
                             std::optional<hsize_t> extent = std::nullopt) const {
            return table(tablePath).template readRecords<DataType>(offset, extent);
        }

        /** @deprecated Use file.table(path).readRecords<DataType>(selection) instead. */
        template<typename DataType>
        [[deprecated("Use file.table(path).readRecords<DataType>(selection) instead")]] [[nodiscard]] DataType
            readTableRecords(std::string_view tablePath, TableSelection selection) const {
            return table(tablePath).template readRecords<DataType>(selection);
        }

        /** @deprecated Use file.table(path).readField<DataType>(fields, offset, extent) instead. */
        template<typename DataType, typename Fields>
        [[deprecated("Use file.table(path).readField<DataType>(fields, offset, extent) instead")]] [[nodiscard]] DataType
            readTableField(std::string_view       tablePath,
                           const Fields          &fields,
                           std::optional<hsize_t> offset = std::nullopt,
                           std::optional<hsize_t> extent = std::nullopt) const {
            return table(tablePath).template readField<DataType>(fields, offset, extent);
        }

        /** @deprecated Use file.table(path).readField<DataType>(fields, selection) instead. */
        template<typename DataType, typename Fields>
        [[deprecated("Use file.table(path).readField<DataType>(fields, selection) instead")]] [[nodiscard]] DataType
            readTableField(std::string_view tablePath, const Fields &fields, TableSelection selection) const {
            return table(tablePath).template readField<DataType>(fields, selection);
        }

        /** @deprecated Use file.table(path).readFieldInto(data, fields, selection) instead. */
        template<typename DataType, typename Fields>
        [[deprecated("Use file.table(path).readFieldInto(data, fields, selection) instead")]] void
            readTableField(DataType &data, std::string_view tablePath, const Fields &fields, TableSelection tableSelection) const {
            table(tablePath).readFieldInto(data, fields, tableSelection);
        }

        //! Check whether the path refers to a readable HDF5 file.
        [[nodiscard]] bool fileIsValid() const { return h5pp::hdf5::fileIsValid(filePath); }
    };
}
