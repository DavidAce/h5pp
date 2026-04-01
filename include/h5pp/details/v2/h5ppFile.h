#pragma once

#include "../h5ppFile.h"
#include "h5ppGroup.h"

namespace h5pp::v2 {
    class File {
        private:
        fs::path                                  filePath;                                      /*!< Full path to the file */
        h5pp::FileAccess                          fileAccess         = h5pp::FileAccess::RENAME; /*!< File open/create policy. */
        mutable std::optional<hid::h5f>           fileHandle         = std::nullopt; /*!< Keeps a file handle alive in batch operations */
        mutable bool                              keepFileOpenPinned = false; /*!< Tracks whether the cached handle should survive scoped tokens */
        mutable LogLevel                          logLevel           = LogLevel::info; /*!< Log verbosity from 0 [trace] to 6 [off] */
        bool                                      logTimestamp       = false; /*!< Add a time stamp to console log output */
        hid::h5e                                  error_stack        = H5E_DEFAULT; /*!< Reference to the error stack used by HDF5 */
        int                                       currentCompression = -1; /*!< Compression level (-1 is off, 0 is none, 9 is max) */
        mutable std::vector<ReclaimInfo::Reclaim> reclaimStack; /*!< Stores alloc metadata from variable-length reads to free */

        void init() {
            h5pp::logger::setLogger("h5pp|init", logLevel, logTimestamp);
            h5pp::logger::log->debug("Initializing file object for [{}]", filePath.string());

            error_stack                          = H5Eget_current_stack();
            herr_t turnOffAutomaticErrorPrinting = H5Eset_auto2(error_stack, nullptr, nullptr);
            if(turnOffAutomaticErrorPrinting < 0) throw h5pp::runtime_error("Failed to turn off H5E error printing");

            filePath = h5pp::hdf5::createFile(filePath, fileAccess, plists);
        }

        [[nodiscard]] hid::h5f openFileHandle() const {
            h5pp::logger::setLogger("h5pp|" + filePath.filename().string(), logLevel, logTimestamp);
            if(fileHandle) return fileHandle.value();
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
        PropertyLists plists = PropertyLists();

        struct KeepFileOpenToken {
            const File *file_         = nullptr;
            hid::h5f    pin_;
            bool        createdCache_ = false;

            void release_() noexcept {
                (void)pin_.release();
                if(createdCache_ and file_ and file_->fileHandle and not file_->keepFileOpenPinned and file_->fileHandle->refcount() == 1)
                    file_->fileHandle = std::nullopt;
                file_         = nullptr;
                createdCache_ = false;
            }

            explicit KeepFileOpenToken(const File &file) : file_(&file) {
                if(not file_->fileHandle) {
                    file_->fileHandle = file_->openFileHandle();
                    createdCache_     = true;
                }
                pin_ = file_->fileHandle.value();
            }

            KeepFileOpenToken(const KeepFileOpenToken &)            = delete;
            KeepFileOpenToken &operator=(const KeepFileOpenToken &) = delete;

            KeepFileOpenToken(KeepFileOpenToken &&other) noexcept
                : file_(std::exchange(other.file_, nullptr)),
                  pin_(std::move(other.pin_)),
                  createdCache_(std::exchange(other.createdCache_, false)) {}

            KeepFileOpenToken &operator=(KeepFileOpenToken &&other) noexcept {
                if(this == &other) return *this;
                release_();
                file_         = std::exchange(other.file_, nullptr);
                pin_          = std::move(other.pin_);
                createdCache_ = std::exchange(other.createdCache_, false);
                return *this;
            }

            ~KeepFileOpenToken() { release_(); }
        };
        using FileHandleToken = KeepFileOpenToken;

        class Advanced {
            private:
            File &file_;

            public:
            explicit Advanced(File &file) : file_(file) {}

            void setCloseDegree(H5F_close_degree_t degree) {
                if(file_.plists.fileAccess == H5P_DEFAULT) file_.plists.fileAccess = H5Fget_access_plist(file_.openFileHandle());
                H5Pset_fclose_degree(file_.plists.fileAccess, degree);
                if(file_.fileHandle) {
                    file_.fileHandle = std::nullopt;
                    file_.fileHandle = file_.openFileHandle();
                }
            }

            void setDriver_core(bool writeOnClose = false, size_t bytesPerMalloc = 10240000) {
                if(file_.plists.fileAccess == H5P_DEFAULT) file_.plists.fileAccess = H5Fget_access_plist(file_.openFileHandle());
                H5Pset_fapl_core(file_.plists.fileAccess, bytesPerMalloc, static_cast<hbool_t>(writeOnClose));
                if(file_.fileHandle) {
                    file_.fileHandle = std::nullopt;
                    file_.fileHandle = file_.openFileHandle();
                }
            }

            void setDriver_sec2() {
                if(file_.plists.fileAccess == H5P_DEFAULT) file_.plists.fileAccess = H5Fget_access_plist(file_.openFileHandle());
                H5Pset_fapl_sec2(file_.plists.fileAccess);
                if(file_.fileHandle) {
                    file_.fileHandle = std::nullopt;
                    file_.fileHandle = file_.openFileHandle();
                }
            }

            void setDriver_stdio() {
                if(file_.plists.fileAccess == H5P_DEFAULT) file_.plists.fileAccess = H5Fget_access_plist(file_.openFileHandle());
                H5Pset_fapl_stdio(file_.plists.fileAccess);
                if(file_.fileHandle) {
                    file_.fileHandle = std::nullopt;
                    file_.fileHandle = file_.openFileHandle();
                }
            }

#ifdef H5PP_USE_MPI
            void setDriver_mpio(MPI_Comm comm, MPI_Info info) {
                file_.plists.fileAccess = H5Fget_access_plist(file_.openFileHandle());
                H5Pset_fapl_mpio(file_.plists.fileAccess, comm, info);
                if(file_.fileHandle) {
                    file_.fileHandle = std::nullopt;
                    file_.fileHandle = file_.openFileHandle();
                }
            }
#endif

            [[nodiscard]] hid::h5f openFileHandle() const { return file_.openFileHandle(); }
            [[nodiscard]] KeepFileOpenToken keepFileOpen() const { return file_.keepFileOpen(); }
            [[nodiscard]] FileHandleToken getFileHandleToken() const { return file_.getFileHandleToken(); }
            [[nodiscard]] File &getLowLevelFile() { return file_; }

            void vlenReclaim() const {
                for(auto &item : file_.reclaimStack) item.reclaim();
                file_.reclaimStack.clear();
            }

            void vlenDropReclaims() const {
                for(auto &item : file_.reclaimStack) item.drop();
                file_.reclaimStack.clear();
            }

            void vlenEnableReclaimsTracking() { file_.plists.vlenTrackReclaims = true; }
            void vlenDisableReclaimsTracking() { file_.plists.vlenTrackReclaims = false; }
        };

        File() = default;

        template<typename LogLevelType = LogLevel>
        explicit File(h5pp::fs::path       filePath_,
                      h5pp::FileAccess     fileAccess_   = h5pp::FileAccess::RENAME,
                      LogLevelType         logLevel_     = LogLevel::info,
                      bool                 logTimestamp_ = false,
                      const PropertyLists &plists_       = PropertyLists())
            : filePath(std::move(filePath_)),
              fileAccess(fileAccess_),
              logLevel(Num2Level(logLevel_)),
              logTimestamp(logTimestamp_),
              plists(plists_) {
            init();
        }

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

        [[nodiscard]] Dataset dataset(std::string_view dsetPath) { return Dataset(*this, dsetPath); }
        [[nodiscard]] ConstDataset dataset(std::string_view dsetPath) const { return ConstDataset(*this, dsetPath); }
        [[nodiscard]] Group group(std::string_view groupPath = "/") { return Group(*this, groupPath); }
        [[nodiscard]] ConstGroup group(std::string_view groupPath = "/") const { return ConstGroup(*this, groupPath); }
        [[nodiscard]] Link link(std::string_view linkPath) { return Link(*this, linkPath); }
        [[nodiscard]] ConstLink link(std::string_view linkPath) const { return ConstLink(*this, linkPath); }
        [[nodiscard]] Table table(std::string_view tablePath) { return Table(*this, tablePath); }
        [[nodiscard]] ConstTable table(std::string_view tablePath) const { return ConstTable(*this, tablePath); }
        [[nodiscard]] Attribute attribute(std::string_view linkPath, std::string_view attrName) {
            return Attribute(*this, linkPath, attrName);
        }
        [[nodiscard]] ConstAttribute attribute(std::string_view linkPath, std::string_view attrName) const {
            return ConstAttribute(*this, linkPath, attrName);
        }
        [[nodiscard]] Advanced advanced() { return Advanced(*this); }

        [[nodiscard]] KeepFileOpenToken keepFileOpen() const { return KeepFileOpenToken(*this); }
        [[nodiscard]] FileHandleToken getFileHandleToken() const { return keepFileOpen(); }

        [[nodiscard]] fs::path copyFileTo(const h5pp::fs::path &targetFilePath, const FileAccess &perm = FileAccess::COLLISION_FAIL) const {
            return h5pp::hdf5::copyFile(getFilePath(), targetFilePath, perm, plists);
        }

        [[nodiscard]] fs::path moveFileTo(const h5pp::fs::path &targetFilePath, const FileAccess &perm = FileAccess::COLLISION_FAIL) {
            auto newPath = h5pp::hdf5::moveFile(getFilePath(), targetFilePath, perm, plists);
            if(fs::exists(newPath)) filePath = newPath;
            return newPath;
        }

        void copyLinkToFile(std::string_view localLinkPath, const h5pp::fs::path &targetFilePath, std::string_view targetLinkPath, const FileAccess &perm = FileAccess::READWRITE) const {
            link(localLinkPath).copyToFile(targetFilePath, targetLinkPath, perm);
        }

        void copyLinkFromFile(std::string_view localLinkPath, const h5pp::fs::path &sourceFilePath, std::string_view sourceLinkPath) {
            link(localLinkPath).copyFromFile(sourceFilePath, sourceLinkPath);
        }

        template<typename h5x_tgt>
        void copyLinkToLocation(std::string_view localLinkPath, const h5x_tgt &targetLocationId, std::string_view targetLinkPath) const {
            link(localLinkPath).copyToLocation(targetLocationId, targetLinkPath);
        }

        template<typename h5x_src>
        void copyLinkFromLocation(std::string_view localLinkPath, const h5x_src &sourceLocationId, std::string_view sourceLinkPath) {
            link(localLinkPath).copyFromLocation(sourceLocationId, sourceLinkPath);
        }

        void moveLinkToFile(std::string_view localLinkPath, const h5pp::fs::path &targetFilePath, std::string_view targetLinkPath, const FileAccess &perm = FileAccess::READWRITE) {
            link(localLinkPath).moveToFile(targetFilePath, targetLinkPath, perm);
        }

        void moveLinkFromFile(std::string_view localLinkPath, const h5pp::fs::path &sourceFilePath, std::string_view sourceLinkPath) {
            link(localLinkPath).moveFromFile(sourceFilePath, sourceLinkPath);
        }

        template<typename h5x_tgt>
        void moveLinkToLocation(std::string_view localLinkPath, const h5x_tgt &targetLocationId, std::string_view targetLinkPath, LocationMode locMode = LocationMode::DETECT) {
            link(localLinkPath).moveToLocation(targetLocationId, targetLinkPath, locMode);
        }

        template<typename h5x_src>
        void moveLinkFromLocation(std::string_view localLinkPath, const h5x_src &sourceLocationId, std::string_view sourceLinkPath, LocationMode locMode = LocationMode::DETECT) {
            link(localLinkPath).moveFromLocation(sourceLocationId, sourceLinkPath, locMode);
        }

        void resizeDataset(DsetInfo &info, const DimsType &newDimensions, std::optional<h5pp::ResizePolicy> modeOverride = std::nullopt) {
            if(fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to resize dataset on read-only file [{}]", filePath.string());
            h5pp::hdf5::resizeDataset(info, newDimensions, modeOverride);
        }

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

        template<typename DataType>
        void writeDataset(const DataType &data, std::string_view dsetPath, const DatasetWriteOptions &options = DatasetWriteOptions()) {
            dataset(dsetPath).write(data, options);
        }

        [[deprecated("Use file.dataset(path).create(createOptions) instead")]] [[nodiscard]] DsetInfo
            createDataset(std::string_view dsetPath, const DatasetCreateOptions &createOptions) {
            return dataset(dsetPath).create(createOptions);
        }

        template<typename DataType>
        void writeDataset(const DataType &data,
                          std::string_view dsetPath,
                          const DatasetCreateOptions &createOptions,
                          const DatasetWriteOptions &writeOptions = DatasetWriteOptions()) {
            dataset(dsetPath).ensure(createOptions).write(data, writeOptions);
        }

        template<typename DataType>
        [[deprecated("Use file.dataset(path).append(data, axis, options) instead")]] [[nodiscard]] DsetInfo
            appendToDataset(const DataType &data, std::string_view dsetPath, size_t axis, const DatasetAppendOptions &options = DatasetAppendOptions()) {
            return dataset(dsetPath).append(data, axis, options);
        }

        template<typename DataType>
        void readDataset(DataType &data, std::string_view dsetPath, const DatasetReadOptions &options = DatasetReadOptions()) const {
            dataset(dsetPath).readInto(data, options);
        }

        template<typename DataType>
        [[nodiscard]] DataType readDataset(std::string_view dsetPath, const DatasetReadOptions &options = DatasetReadOptions()) const {
            return dataset(dsetPath).template read<DataType>(options);
        }

        template<typename DataType>
        void writeAttribute(std::string_view linkPath, std::string_view attrName, const DataType &data, const AttributeWriteOptions &options = AttributeWriteOptions()) {
            attribute(linkPath, attrName).write(data, options);
        }

        template<typename DataType>
        [[nodiscard]] DataType readAttribute(std::string_view linkPath, std::string_view attrName, const AttributeReadOptions &options = AttributeReadOptions()) const {
            return attribute(linkPath, attrName).template read<DataType>(options);
        }

        void flush() {
            H5Fflush(openFileHandle(), H5F_scope_t::H5F_SCOPE_GLOBAL);
            h5pp::logger::log->trace("Flushing caches");
            H5garbage_collect();
            H5Eprint(H5E_DEFAULT, stderr);
        }

        void createGroup(std::string_view groupPath) { group(groupPath).create(); }
        void deleteLink(std::string_view linkPath) { link(linkPath).deleteLink(); }

        void createSoftLink(std::string_view targetLinkPath, std::string_view softLinkPath) { link(targetLinkPath).createSoftLink(softLinkPath); }

        void createExternalLink(std::string_view targetFilePath, std::string_view targetLinkPath, std::string_view softLinkPath) {
            link(softLinkPath).createExternalLink(targetFilePath, targetLinkPath);
        }

        void setKeepFileOpened() const {
            keepFileOpenPinned = true;
            if(not fileHandle) fileHandle = openFileHandle();
        }

        void setKeepFileClosed() const {
            keepFileOpenPinned = false;
            fileHandle         = std::nullopt;
        }

        void setCompressionLevel(unsigned int compressionZeroToNine) {
            currentCompression = h5pp::hdf5::getValidCompressionLevel(compressionZeroToNine);
        }

        [[nodiscard]] int getCompressionLevel() const { return currentCompression; }
        [[nodiscard]] int getCompressionLevel(const std::optional<int> compression) const {
            if(compression) return h5pp::hdf5::getValidCompressionLevel(compression.value());
            return currentCompression;
        }

        [[nodiscard]] h5pp::FileAccess getFileAccess() const { return fileAccess; }
        [[nodiscard]] std::string getFileName() const { return filePath.filename().string(); }
        [[nodiscard]] std::string getFilePath() const { return filePath.string(); }
        [[nodiscard]] LogLevel getLogLevel() const { return logLevel; }
        [[nodiscard]] bool linkExists(std::string_view linkPath) const {
            return h5pp::hdf5::checkIfLinkExists(openFileHandle(), linkPath, plists.linkAccess);
        }
        [[nodiscard]] bool attributeExists(std::string_view linkPath, std::string_view attrName) const {
            return h5pp::hdf5::checkIfAttrExists(openFileHandle(), linkPath, attrName, std::nullopt, plists.linkAccess);
        }

        template<typename Fields>
        [[nodiscard]] bool fieldExists(std::string_view tablePath, const Fields &fields) const {
            return table(tablePath).fieldExists(fields);
        }

        [[nodiscard]] std::vector<std::string> findLinks(std::string_view searchKey = "", std::string_view searchRoot = "/", long maxHits = -1, long maxDepth = -1, bool followSymlinks = false) const {
            return group(searchRoot).findLinks(searchKey, maxHits, maxDepth, followSymlinks);
        }

        [[nodiscard]] std::vector<std::string> findDatasets(std::string_view searchKey = "", std::string_view searchRoot = "/", long maxHits = -1, long maxDepth = -1, bool followSymlinks = false) const {
            return group(searchRoot).findDatasets(searchKey, maxHits, maxDepth, followSymlinks);
        }

        [[nodiscard]] std::vector<std::string> findGroups(std::string_view searchKey = "", std::string_view searchRoot = "/", long maxHits = -1, long maxDepth = -1, bool followSymlinks = false) const {
            return group(searchRoot).findGroups(searchKey, maxHits, maxDepth, followSymlinks);
        }

        [[deprecated("Use file.link(path).getAttributeNames() instead")]] [[nodiscard]] std::vector<std::string>
            getAttributeNames(std::string_view linkPath) const {
            return link(linkPath).getAttributeNames();
        }

        [[nodiscard]] DsetInfo getDatasetInfo(std::string_view dsetPath) const {
            return dataset(dsetPath).getInfo();
        }

        [[nodiscard]] AttrInfo getAttributeInfo(std::string_view linkPath, std::string_view attrName) const {
            return attribute(linkPath, attrName).getInfo();
        }

        [[nodiscard]] TableInfo getTableInfo(std::string_view tablePath) const {
            return table(tablePath).getInfo();
        }

        [[nodiscard]] std::optional<std::vector<hsize_t>> getDatasetChunkDimensions(std::string_view datasetPath) const {
            return dataset(datasetPath).getChunkDimensions();
        }

        [[nodiscard]] TableFieldInfo getTableFieldInfo(std::string_view tablePath) const {
            return table(tablePath).getFieldInfo();
        }

        [[nodiscard]] TypeInfo getTypeInfoDataset(std::string_view dsetPath) const {
            return dataset(dsetPath).getTypeInfo();
        }

        [[nodiscard]] TypeInfo getTypeInfoAttribute(std::string_view linkPath, std::string_view attrName) const {
            return attribute(linkPath, attrName).getTypeInfo();
        }

        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes(std::string_view linkPath) const {
            return link(linkPath).getTypeInfoAttributes();
        }

        [[nodiscard]] LinkInfo getLinkInfo(std::string_view linkPath) const {
            return link(linkPath).getInfo();
        }

        [[deprecated("Use file.table(path).create(h5Type, title, options) instead")]] [[nodiscard]] TableInfo
            createTable(const hid::h5t &h5Type, std::string_view tablePath, std::string_view tableTitle, const TableCreateOptions &options = TableCreateOptions()) {
            return table(tablePath).create(h5Type, tableTitle, options).getInfo();
        }

        template<typename DataType>
        [[deprecated("Use file.table(path).appendRecords(data, extent) instead")]] [[nodiscard]] TableInfo
            appendTableRecords(const DataType &data, std::string_view tablePath, std::optional<hsize_t> extent = std::nullopt) {
            auto tgtTable = table(tablePath);
            tgtTable.appendRecords(data, extent);
            return tgtTable.getInfo();
        }

        template<typename h5x_src>
        [[deprecated("Use file.table(targetPath).ensure(options).appendRecordsFrom(srcLocation, srcTablePath, selection) instead")]]
        [[nodiscard]] TableInfo appendTableRecords(const h5x_src &srcLocation,
                                                   std::string_view srcTablePath,
                                                   std::string_view tgtTablePath,
                                                   TableSelection srcSelection = TableSelection::ALL,
                                                   const TableCreateOptions &options = TableCreateOptions()) {
            return table(tgtTablePath).ensure(options).appendRecordsFrom(srcLocation, srcTablePath, srcSelection);
        }

        template<typename DataType>
        [[deprecated("Use file.table(path).writeRecords(data, offset, extent) instead")]] [[nodiscard]] TableInfo
            writeTableRecords(const DataType &data, std::string_view tablePath, hsize_t offset = 0, std::optional<hsize_t> extent = std::nullopt) {
            auto tgtTable = table(tablePath);
            tgtTable.writeRecords(data, offset, extent);
            return tgtTable.getInfo();
        }

        template<typename DataType>
        [[nodiscard]] DataType readTableRecords(std::string_view tablePath, std::optional<hsize_t> offset = std::nullopt, std::optional<hsize_t> extent = std::nullopt) const {
            return table(tablePath).template readRecords<DataType>(offset, extent);
        }

        template<typename DataType>
        [[nodiscard]] DataType readTableRecords(std::string_view tablePath, TableSelection selection) const {
            return table(tablePath).template readRecords<DataType>(selection);
        }

        template<typename DataType, typename Fields>
        [[deprecated("Use file.table(path).readField<DataType>(fields, offset, extent) instead")]] [[nodiscard]] DataType
            readTableField(std::string_view tablePath, const Fields &fields, std::optional<hsize_t> offset = std::nullopt, std::optional<hsize_t> extent = std::nullopt) const {
            return table(tablePath).template readField<DataType>(fields, offset, extent);
        }

        template<typename DataType, typename Fields>
        [[deprecated("Use file.table(path).readField<DataType>(fields, selection) instead")]] [[nodiscard]] DataType
            readTableField(std::string_view tablePath, const Fields &fields, TableSelection selection) const {
            return table(tablePath).template readField<DataType>(fields, selection);
        }

        template<typename DataType, typename Fields>
        [[deprecated("Use file.table(path).readFieldInto(data, fields, selection) instead")]] void
            readTableField(DataType &data, std::string_view tablePath, const Fields &fields, TableSelection tableSelection) const {
            table(tablePath).readFieldInto(data, fields, tableSelection);
        }

        [[nodiscard]] bool fileIsValid() const { return h5pp::hdf5::fileIsValid(filePath); }
    };
}
