#pragma once

#include "h5ppLink.h"
#include <memory>
#include <string>

namespace h5pp::inline v2 {
    class File;

    /**
     * @brief Group handle.
     *
     * Obtain a Group with write methods from non-const File::group() or Group::group().
     * ConstGroup provides read-only methods. Calling group() does not create a group
     * on file; use create() when an empty group is needed.
     *
     * Child paths are relative to this group, even if they start with a slash.
     * Each group object keeps the file open and remains usable after the File
     * object is destroyed. Release these objects before File::moveFileTo().
     *
     * @code{.cpp}
     * auto group = file.group("measurements");
     * group.create();
     * group.dataset("velocity").write(std::vector<double>{1.0, 2.0});
     * group.attribute("description").write("Measured velocities");
     * @endcode
     *
     * @tparam FileType File for Group, or const File for ConstGroup.
     */
    template<typename FileType>
    requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    class BasicGroup {
        private:
        hid::h5f                                           fileHandle_;
        PropertyLists                                      plists_;
        FileAccess                                         fileAccess_;
        int                                                compression_;
        std::shared_ptr<std::vector<ReclaimInfo::Reclaim>> reclaimStack_;
        fs::path                                           filePath_;
        std::string                                        groupPath_;

        public:
        /**
         * @brief Refer to a group without creating it.
         * @param file File containing the group.
         * @param groupPath Group path relative to the file root; "/" refers to the root group.
         */
        BasicGroup(FileType &file, std::string_view groupPath)
            : BasicGroup(file.openFileHandle(),
                         file.plists,
                         file.getFileAccess(),
                         file.getCompressionLevel(),
                         file.reclaimStack,
                         file.getFilePath(),
                         groupPath) {}

        private:
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicLink;

        BasicGroup(const hid::h5f                                           &fileHandle,
                   const PropertyLists                                      &plists,
                   FileAccess                                                fileAccess,
                   int                                                       compression,
                   const std::shared_ptr<std::vector<ReclaimInfo::Reclaim>> &reclaimStack,
                   const fs::path                                           &filePath,
                   std::string_view                                          groupPath)
            : fileHandle_(fileHandle), plists_(plists), fileAccess_(fileAccess), compression_(compression), reclaimStack_(reclaimStack),
              filePath_(filePath), groupPath_(detail::normalizeLinkPath(groupPath)) {}

        public:
        //! Object name without its parent path.
        [[nodiscard]] std::string getName() const { return groupPath_ == "/" ? "/" : groupPath_.substr(groupPath_.find_last_of('/') + 1); }
        //! Parent group path; "/" for objects in the file root.
        [[nodiscard]] std::string getParentPath() const {
            auto parent = util::getParentPath(groupPath_);
            return parent.empty() ? "/" : std::string(parent);
        }
        //! Owning file path.
        [[nodiscard]] std::string getFilePath() const { return hdf5::getFilePath(fileHandle_); }
        //! Owning filename.
        [[nodiscard]] std::string getFileName() const { return fs::path(getFilePath()).filename().string(); }
        //! File access policy.
        [[nodiscard]] FileAccess  getFileAccess() const { return fileAccess_; }

        //! Return the group path relative to the file root, or "/" for the root group.
        [[nodiscard]] const std::string &getPath() const { return groupPath_; }
        //! Check whether a link exists at the group path.
        [[nodiscard]] bool     exists() const { return h5pp::hdf5::checkIfLinkExists(fileHandle_, groupPath_, plists_.linkAccess); }
        //! Read link metadata at the group path.
        [[nodiscard]] LinkInfo getInfo() const {
            Options options;
            options.linkPath = h5pp::util::safe_str(groupPath_);
            return h5pp::scan::readLinkInfo(fileHandle_, options, plists_);
        }
        //! Read datatype information for attributes on this group.
        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes() const {
            return h5pp::hdf5::getTypeInfo_allAttributes(fileHandle_, groupPath_, std::nullopt, plists_.linkAccess);
        }
        //! List attribute names on this group.
        [[nodiscard]] std::vector<std::string> getAttributeNames() const {
            return h5pp::hdf5::getAttributeNames(fileHandle_, groupPath_, std::nullopt, plists_.linkAccess);
        }
        //! Check whether a child link exists.
        [[nodiscard]] bool linkExists(std::string_view childPath) const { return link(childPath).exists(); }
        //! Check whether an attribute exists on the specified child object.
        [[nodiscard]] bool attributeExists(std::string_view childPath, std::string_view attrName) const {
            return link(childPath).attribute(attrName).exists();
        }
        //! Read metadata for a child link.
        [[nodiscard]] LinkInfo getLinkInfo(std::string_view childPath) const { return link(childPath).getInfo(); }
        //! Read metadata for a dataset relative to this group.
        [[nodiscard]] DsetInfo getDatasetInfo(std::string_view dsetPath) const { return dataset(dsetPath).getInfo(); }
        //! Read metadata for an attribute on a child object.
        [[nodiscard]] AttrInfo getAttributeInfo(std::string_view childPath, std::string_view attrName) const {
            return link(childPath).attribute(attrName).getInfo();
        }
        //! Read metadata for a table relative to this group.
        [[nodiscard]] TableInfo      getTableInfo(std::string_view tablePath) const { return table(tablePath).getInfo(); }
        //! Read field names, datatypes and offsets for a table relative to this group.
        [[nodiscard]] TableFieldInfo getTableFieldInfo(std::string_view tablePath) const { return table(tablePath).getFieldInfo(); }
        //! Read datatype information for a dataset relative to this group.
        [[nodiscard]] TypeInfo       getTypeInfoDataset(std::string_view dsetPath) const { return dataset(dsetPath).getTypeInfo(); }
        //! Read datatype information for an attribute on a child object.
        [[nodiscard]] TypeInfo       getTypeInfoAttribute(std::string_view childPath, std::string_view attrName) const {
            return link(childPath).attribute(attrName).getTypeInfo();
        }
        //! Read datatype information for attributes on a child object.
        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes(std::string_view childPath) const {
            return link(childPath).getTypeInfoAttributes();
        }
        //! List attribute names on a child object.
        [[nodiscard]] std::vector<std::string> getAttributeNames(std::string_view childPath) const {
            return link(childPath).getAttributeNames();
        }
        /**
         * @brief Find attribute names on this group containing a substring.
         * @param searchKey Substring to match. An empty string returns all names.
         */
        [[nodiscard]] std::vector<std::string> findAttributes(std::string_view searchKey = "") const {
            std::vector<std::string> matches;
            for(const auto &attrName : getAttributeNames())
                if(searchKey.empty() or attrName.find(searchKey) != std::string::npos) matches.emplace_back(attrName);
            return matches;
        }
        //! Refer to an attribute on this group without creating it.
        BasicAttribute<FileType> attribute(std::string_view attrName) const {
            return BasicAttribute<FileType>(fileHandle_, plists_, fileAccess_, groupPath_, attrName);
        }

        //! Refer to a child group without creating it.
        BasicGroup group(std::string_view childPath) const {
            return BasicGroup(fileHandle_,
                              plists_,
                              fileAccess_,
                              compression_,
                              reclaimStack_,
                              filePath_,
                              detail::joinLinkPath(groupPath_, childPath));
        }

        //! Refer to a child link without creating it.
        BasicLink<FileType> link(std::string_view childPath) const {
            return BasicLink<FileType>(fileHandle_,
                                       plists_,
                                       fileAccess_,
                                       compression_,
                                       reclaimStack_,
                                       filePath_,
                                       detail::joinLinkPath(groupPath_, childPath));
        }

        //! Refer to a dataset relative to this group without creating it.
        BasicDataset<FileType> dataset(std::string_view dsetPath) const {
            return BasicDataset<FileType>(fileHandle_, plists_, fileAccess_, compression_, detail::joinLinkPath(groupPath_, dsetPath));
        }

        //! Refer to a table relative to this group without creating it.
        BasicTable<FileType> table(std::string_view tablePath) const {
            return BasicTable<FileType>(fileHandle_,
                                        plists_,
                                        fileAccess_,
                                        compression_,
                                        reclaimStack_,
                                        detail::joinLinkPath(groupPath_, tablePath));
        }

        [[nodiscard]] std::vector<std::string>
            /**
             * @brief Find links below this group.
             * @param searchKey Substring to match in each link name. An empty string matches all names.
             * @param maxHits Maximum number of matches; -1 means unlimited.
             * @param maxDepth Maximum search depth; 0 searches immediate children, -1 means unlimited.
             * @param followSymlinks Follow soft and external links when searching.
             * @return Paths relative to this group.
             */
            findLinks(std::string_view searchKey = "", long maxHits = -1, long maxDepth = -1, bool followSymlinks = false) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_UNKNOWN>(fileHandle_, searchKey, groupPath_, maxHits, maxDepth, followSymlinks, plists_);
        }

        [[nodiscard]] std::vector<std::string>
            /**
             * @brief Find datasets below this group.
             * @param searchKey Substring to match in each link name. An empty string matches all names.
             * @param maxHits Maximum number of matches; -1 means unlimited.
             * @param maxDepth Maximum search depth; 0 searches immediate children, -1 means unlimited.
             * @param followSymlinks Follow soft and external links when searching.
             * @return Paths relative to this group.
             */
            findDatasets(std::string_view searchKey = "", long maxHits = -1, long maxDepth = -1, bool followSymlinks = false) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_DATASET>(fileHandle_, searchKey, groupPath_, maxHits, maxDepth, followSymlinks, plists_);
        }

        [[nodiscard]] std::vector<std::string>
            /**
             * @brief Find groups below this group.
             * @param searchKey Substring to match in each link name. An empty string matches all names.
             * @param maxHits Maximum number of matches; -1 means unlimited.
             * @param maxDepth Maximum search depth; 0 searches immediate children, -1 means unlimited.
             * @param followSymlinks Follow soft and external links when searching.
             * @return Paths relative to this group.
             */
            findGroups(std::string_view searchKey = "", long maxHits = -1, long maxDepth = -1, bool followSymlinks = false) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_GROUP>(fileHandle_, searchKey, groupPath_, maxHits, maxDepth, followSymlinks, plists_);
        }

        //! Create this group and missing parent groups. An existing group is left unchanged.
        BasicGroup create() requires traits::is_mutable_file_v<FileType>
        {
            if(fileAccess_ == FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to create a group with read-only file access: [{}]", groupPath_);
            h5pp::hdf5::createGroup(fileHandle_, groupPath_, std::nullopt, plists_);
            return *this;
        }

        //! Create a child group and missing parent groups.
        BasicGroup createGroup(std::string_view childPath) requires traits::is_mutable_file_v<FileType>
        {
            return group(childPath).create();
        }

        //! Delete a child link. The object is retained while other hard links or open identifiers refer to it.
        void deleteLink(std::string_view childPath) requires traits::is_mutable_file_v<FileType>
        {
            link(childPath).deleteLink();
        }

        /**
         * @brief Create a soft link to an object relative to this group.
         * @param targetChildPath Target path relative to this group.
         * @param softChildPath New soft-link path relative to this group.
         */
        BasicLink<FileType> createSoftLink(std::string_view targetChildPath, std::string_view softChildPath)
            requires traits::is_mutable_file_v<FileType>
        {
            return link(targetChildPath).createSoftLink(detail::joinLinkPath(groupPath_, softChildPath));
        }

        /**
         * @brief Create a child link to an object in another file.
         * @param targetFilePath Target file path. Relative paths are resolved when HDF5 opens the link.
         * @param targetLinkPath Object path in the target file.
         * @param softChildPath New external-link path relative to this group.
         */
        BasicLink<FileType> createExternalLink(std::string_view targetFilePath,
                                               std::string_view targetLinkPath,
                                               std::string_view softChildPath) requires traits::is_mutable_file_v<FileType>
        {
            return link(softChildPath).createExternalLink(targetFilePath, targetLinkPath);
        }

        //! Check whether all requested field names or zero-based indices exist in a child table.
        template<typename Fields>
        [[nodiscard]] bool fieldExists(std::string_view tablePath, const Fields &fields) const {
            return table(tablePath).fieldExists(fields);
        }
    };

    template<typename FileType>
    requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    inline BasicGroup<FileType> BasicLink<FileType>::asGroup() const {
        return BasicGroup<FileType>(fileHandle_, plists_, fileAccess_, compression_, reclaimStack_, filePath_, linkPath_);
    }

    using Group      = BasicGroup<h5pp::v2::File>;       /*!< Group operations obtained from a non-const File. */
    using ConstGroup = BasicGroup<const h5pp::v2::File>; /*!< Group operations obtained from a const File. */
}
