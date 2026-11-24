#pragma once

#include "h5ppAttribute.h"
#include "h5ppDataset.h"
#include "h5ppTable.h"
#include <memory>
#include <string>

namespace h5pp::inline v2 {
    class File;

    template<typename FileType>
    requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    class BasicGroup;

    /**
     * @brief Link handle.
     *
     * A link names an object in an HDF5 file. Obtain a Link with write methods from a
     * non-const File or Group. ConstLink provides inspection and copying to other files.
     * Calling link() does not create a link or check the target object's type.
     *
     * Each link object keeps the file open and remains usable after the File object
     * is destroyed. Release these objects before File::moveFileTo(). Paths supplied
     * to copy, move and link-creation methods are relative to the file root unless
     * an HDF5 location identifier is supplied.
     *
     * @code{.cpp}
     * auto link = file.link("measurements/velocity");
     * if(link.exists()) {
     *     auto dataset = link.asDataset();
     *     auto values = dataset.read<std::vector<double>>();
     * }
     * @endcode
     *
     * @tparam FileType File for Link, or const File for ConstLink.
     */
    template<typename FileType>
    requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    class BasicLink {
        private:
        hid::h5f                                           fileHandle_;
        PropertyLists                                      plists_;
        FileAccess                                         fileAccess_;
        int                                                compression_;
        std::shared_ptr<std::vector<ReclaimInfo::Reclaim>> reclaimStack_;
        fs::path                                           filePath_;
        std::string                                        linkPath_;

        void requireWritable_(std::string_view operation) const {
            if(fileAccess_ == FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to {} on read-only link [{}]", operation, linkPath_);
        }

        public:
        /**
         * @brief Refer to a link without creating it.
         * @param file File containing the link.
         * @param linkPath Link path relative to the file root.
         */
        BasicLink(FileType &file, std::string_view linkPath)
            : BasicLink(file.openFileHandle(),
                        file.plists,
                        file.getFileAccess(),
                        file.getCompressionLevel(),
                        file.reclaimStack,
                        file.getFilePath(),
                        linkPath) {}

        private:
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicGroup;

        BasicLink(const hid::h5f                                           &fileHandle,
                  const PropertyLists                                      &plists,
                  FileAccess                                                fileAccess,
                  int                                                       compression,
                  const std::shared_ptr<std::vector<ReclaimInfo::Reclaim>> &reclaimStack,
                  const fs::path                                           &filePath,
                  std::string_view                                          linkPath)
            : fileHandle_(fileHandle), plists_(plists), fileAccess_(fileAccess), compression_(compression), reclaimStack_(reclaimStack),
              filePath_(filePath), linkPath_(linkPath) {}

        public:
        [[nodiscard]] std::string getName() const {
            return linkPath_ == "/" ? "/" : linkPath_.substr(linkPath_.find_last_of('/') + 1);
        } /*!< Object name without its parent path. */
        [[nodiscard]] std::string getParentPath() const {
            auto parent = util::getParentPath(linkPath_);
            return parent.empty() ? "/" : std::string(parent);
        } /*!< Parent group path; "/" for objects in the file root. */
        [[nodiscard]] std::string getFilePath() const { return hdf5::getFilePath(fileHandle_); }              /*!< Owning file path. */
        [[nodiscard]] std::string getFileName() const { return fs::path(getFilePath()).filename().string(); } /*!< Owning filename. */
        [[nodiscard]] FileAccess  getFileAccess() const { return fileAccess_; }                               /*!< File access policy. */

        [[nodiscard]] const std::string &getPath() const { return linkPath_; } /*!< Return the stored link path. */
        [[nodiscard]] bool               exists() const {
            return h5pp::hdf5::checkIfLinkExists(fileHandle_, linkPath_, plists_.linkAccess);
        } /*!< Check whether the link exists. */
        [[nodiscard]] LinkInfo getInfo() const {
            Options options;
            options.linkPath = h5pp::util::safe_str(linkPath_);
            return h5pp::scan::readLinkInfo(fileHandle_, options, plists_);
        } /*!< Read link metadata, including the target object type. */
        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes() const {
            return h5pp::hdf5::getTypeInfo_allAttributes(fileHandle_, linkPath_, std::nullopt, plists_.linkAccess);
        } /*!< Read datatype information for attributes on the linked object. */
        [[nodiscard]] std::vector<std::string> getAttributeNames() const {
            return h5pp::hdf5::getAttributeNames(fileHandle_, linkPath_, std::nullopt, plists_.linkAccess);
        } /*!< List attribute names on the linked object. */
        /**
         * @brief Find attribute names on the linked object containing a substring.
         * @param searchKey Substring to match. An empty string returns all names.
         */
        [[nodiscard]] std::vector<std::string> findAttributes(std::string_view searchKey = "") const {
            std::vector<std::string> matches;
            for(const auto &attrName : getAttributeNames())
                if(searchKey.empty() or attrName.find(searchKey) != std::string::npos) matches.emplace_back(attrName);
            return matches;
        }
        BasicAttribute<FileType> attribute(std::string_view attrName) const {
            return BasicAttribute<FileType>(fileHandle_, plists_, fileAccess_, linkPath_, attrName);
        } /*!< Refer to an attribute on the linked object without creating it. */

        BasicDataset<FileType> asDataset() const {
            return BasicDataset<FileType>(fileHandle_, plists_, fileAccess_, compression_, linkPath_);
        } /*!< Refer to this path as a dataset. Its type is checked when a dataset operation is performed. */
        BasicTable<FileType> asTable() const {
            return BasicTable<FileType>(fileHandle_, plists_, fileAccess_, reclaimStack_, linkPath_);
        } /*!< Refer to this path as a table. Its type is checked when a table operation is performed. */
        BasicGroup<FileType> asGroup() const; /*!< Refer to this path as a group without creating it or checking its type. */

        void deleteLink() requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("modify link");
            h5pp::hdf5::deleteLink(fileHandle_, linkPath_, plists_.linkAccess);
        } /*!< Delete this link. The object is retained while other hard links or open identifiers refer to it. */

        /**
         * @brief Copy the linked object to another file, leaving the source unchanged.
         * @param targetFilePath Destination file path.
         * @param targetLinkPath Destination object path relative to that file's root.
         * @param perm Destination file open/create policy.
         */
        void copyToFile(const h5pp::fs::path &targetFilePath,
                        std::string_view      targetLinkPath,
                        const FileAccess     &perm = FileAccess::READWRITE) const {
            h5pp::hdf5::copyLink(filePath_.string(), linkPath_, targetFilePath, targetLinkPath, perm, plists_);
        }

        /**
         * @brief Copy an object from another file to this link path.
         * @param sourceFilePath Source file path.
         * @param sourceLinkPath Source object path relative to that file's root.
         */
        void copyFromFile(const h5pp::fs::path &sourceFilePath, std::string_view sourceLinkPath)
            requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("modify link");
            h5pp::hdf5::copyLink(sourceFilePath, sourceLinkPath, filePath_.string(), linkPath_, h5pp::FileAccess::READWRITE, plists_);
        }

        /**
         * @brief Copy the linked object to another path in the same file.
         * @param targetLinkPath Destination path relative to the file root.
         * @param perm Retained for compatibility; ignored for copies within this open file.
         */
        void copyTo(std::string_view targetLinkPath, const FileAccess &perm = FileAccess::READWRITE) const
            requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("modify link");
            static_cast<void>(perm);
            h5pp::hdf5::copyLink(fileHandle_, linkPath_, fileHandle_, targetLinkPath, plists_);
        }

        /**
         * @brief Copy the linked object to an open HDF5 location.
         * @param targetLocationId Destination file or group identifier.
         * @param targetLinkPath Destination path relative to targetLocationId.
         */
        template<typename h5x_tgt>
        void copyToLocation(const h5x_tgt &targetLocationId, std::string_view targetLinkPath) const {
            h5pp::hdf5::copyLink(fileHandle_, linkPath_, targetLocationId, targetLinkPath, plists_);
        }

        /**
         * @brief Copy an object from an open HDF5 location to this link path.
         * @param sourceLocationId Source file or group identifier.
         * @param sourceLinkPath Source path relative to sourceLocationId.
         */
        template<typename h5x_src>
        void copyFromLocation(const h5x_src &sourceLocationId, std::string_view sourceLinkPath) requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("modify link");
            h5pp::hdf5::copyLink(sourceLocationId, sourceLinkPath, fileHandle_, linkPath_, plists_);
        }

        /**
         * @brief Move the linked object to another file, removing the source link after copying.
         * @param targetFilePath Destination file path.
         * @param targetLinkPath Destination object path relative to that file's root.
         * @param perm Destination file open/create policy.
         */
        void moveToFile(const h5pp::fs::path &targetFilePath,
                        std::string_view      targetLinkPath,
                        const FileAccess     &perm = FileAccess::READWRITE) requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("modify link");
            h5pp::hdf5::moveLink(filePath_.string(), linkPath_, targetFilePath, targetLinkPath, perm, plists_);
        }

        /**
         * @brief Move an object from another file to this link path.
         * @param sourceFilePath Source file path.
         * @param sourceLinkPath Source object path relative to that file's root.
         */
        void moveFromFile(const h5pp::fs::path &sourceFilePath, std::string_view sourceLinkPath)
            requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("modify link");
            h5pp::hdf5::moveLink(sourceFilePath, sourceLinkPath, filePath_.string(), linkPath_, h5pp::FileAccess::READWRITE, plists_);
        }

        /**
         * @brief Move this link to another path in the same file.
         *
         * This object retains its original path; obtain a new Link for the destination.
         *
         * @param targetLinkPath Destination path relative to the file root.
         * @param perm Retained for compatibility; ignored for moves within this open file.
         */
        void moveTo(std::string_view targetLinkPath, const FileAccess &perm = FileAccess::READWRITE)
            requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("modify link");
            static_cast<void>(perm);
            h5pp::hdf5::moveLink(fileHandle_, linkPath_, fileHandle_, targetLinkPath, LocationMode::DETECT, plists_);
        }

        /**
         * @brief Move the linked object to an open HDF5 location.
         * @param targetLocationId Destination file or group identifier.
         * @param targetLinkPath Destination path relative to targetLocationId.
         * @param locMode Whether the locations belong to the same file. DETECT determines this automatically.
         */
        template<typename h5x_tgt>
        void moveToLocation(const h5x_tgt &targetLocationId, std::string_view targetLinkPath, LocationMode locMode = LocationMode::DETECT)
            requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("modify link");
            h5pp::hdf5::moveLink(fileHandle_, linkPath_, targetLocationId, targetLinkPath, locMode, plists_);
        }

        /**
         * @brief Move an object from an open HDF5 location to this link path.
         * @param sourceLocationId Source file or group identifier.
         * @param sourceLinkPath Source path relative to sourceLocationId.
         * @param locMode Whether the locations belong to the same file. DETECT determines this automatically.
         */
        template<typename h5x_src>
        void moveFromLocation(const h5x_src &sourceLocationId, std::string_view sourceLinkPath, LocationMode locMode = LocationMode::DETECT)
            requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("modify link");
            h5pp::hdf5::moveLink(sourceLocationId, sourceLinkPath, fileHandle_, linkPath_, locMode, plists_);
        }

        /**
         * @brief Create a soft link pointing to this object's absolute path.
         * @param softLinkPath New soft-link path relative to the file root.
         */
        BasicLink createSoftLink(std::string_view softLinkPath) requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("modify link");
            std::string targetLinkFullPath = linkPath_.front() == '/' ? linkPath_ : h5pp::format("/{}", linkPath_);
            h5pp::hdf5::createSoftLink(targetLinkFullPath, fileHandle_, softLinkPath, plists_);
            auto copy      = *this;
            copy.linkPath_ = softLinkPath;
            return copy;
        }

        /**
         * @brief Create an external link at this path to an object in another file.
         * @param targetFilePath Target file path, stored in the link. Relative paths are resolved when HDF5 opens it.
         * @param targetLinkPath Object path in the target file.
         */
        BasicLink createExternalLink(std::string_view targetFilePath, std::string_view targetLinkPath)
            requires traits::is_mutable_file_v<FileType>
        {
            requireWritable_("modify link");
#if __cplusplus > 201703L
            if(fs::path(targetFilePath).is_relative()) {
                auto prox = fs::proximate(targetFilePath, filePath_.string());
                if(prox != targetFilePath) {
                    h5pp::logger::log->debug(
                        "External link target [{}] is not relative to current file [{}]. This may create a dangling external link",
                        targetFilePath,
                        filePath_.string());
                }
            }
#endif
            h5pp::hdf5::createExternalLink(targetFilePath, targetLinkPath, fileHandle_, linkPath_, plists_);
            return *this;
        }
    };

    using Link      = BasicLink<h5pp::v2::File>;       /*!< Link operations obtained from a non-const File. */
    using ConstLink = BasicLink<const h5pp::v2::File>; /*!< Link operations obtained from a const File. */
}
