#pragma once

#include "h5ppLink.h"
#include <memory>
#include <string>

namespace h5pp::inline v2 {
    class File;

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
        [[nodiscard]] const std::string &getPath() const { return groupPath_; }
        [[nodiscard]] bool     exists() const { return h5pp::hdf5::checkIfLinkExists(fileHandle_, groupPath_, plists_.linkAccess); }
        [[nodiscard]] LinkInfo getInfo() const {
            Options options;
            options.linkPath = h5pp::util::safe_str(groupPath_);
            return h5pp::scan::readLinkInfo(fileHandle_, options, plists_);
        }
        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes() const {
            return h5pp::hdf5::getTypeInfo_allAttributes(fileHandle_, groupPath_, std::nullopt, plists_.linkAccess);
        }
        [[nodiscard]] std::vector<std::string> getAttributeNames() const {
            return h5pp::hdf5::getAttributeNames(fileHandle_, groupPath_, std::nullopt, plists_.linkAccess);
        }
        [[nodiscard]] bool linkExists(std::string_view childPath) const { return link(childPath).exists(); }
        [[nodiscard]] bool attributeExists(std::string_view childPath, std::string_view attrName) const {
            return link(childPath).attribute(attrName).exists();
        }
        [[nodiscard]] LinkInfo getLinkInfo(std::string_view childPath) const { return link(childPath).getInfo(); }
        [[nodiscard]] DsetInfo getDatasetInfo(std::string_view dsetPath) const { return dataset(dsetPath).getInfo(); }
        [[nodiscard]] AttrInfo getAttributeInfo(std::string_view childPath, std::string_view attrName) const {
            return link(childPath).attribute(attrName).getInfo();
        }
        [[nodiscard]] TableInfo      getTableInfo(std::string_view tablePath) const { return table(tablePath).getInfo(); }
        [[nodiscard]] TableFieldInfo getTableFieldInfo(std::string_view tablePath) const { return table(tablePath).getFieldInfo(); }
        [[nodiscard]] TypeInfo       getTypeInfoDataset(std::string_view dsetPath) const { return dataset(dsetPath).getTypeInfo(); }
        [[nodiscard]] TypeInfo       getTypeInfoAttribute(std::string_view childPath, std::string_view attrName) const {
            return link(childPath).attribute(attrName).getTypeInfo();
        }
        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes(std::string_view childPath) const {
            return link(childPath).getTypeInfoAttributes();
        }
        [[nodiscard]] std::vector<std::string> getAttributeNames(std::string_view childPath) const {
            return link(childPath).getAttributeNames();
        }
        [[nodiscard]] std::vector<std::string> findAttributes(std::string_view searchKey = "") const {
            std::vector<std::string> matches;
            for(const auto &attrName : getAttributeNames())
                if(searchKey.empty() or attrName.find(searchKey) != std::string::npos) matches.emplace_back(attrName);
            return matches;
        }
        [[nodiscard]] BasicAttribute<FileType> attribute(std::string_view attrName) const {
            return BasicAttribute<FileType>(fileHandle_, plists_, fileAccess_, groupPath_, attrName);
        }

        [[nodiscard]] BasicGroup group(std::string_view childPath) const {
            return BasicGroup(fileHandle_,
                              plists_,
                              fileAccess_,
                              compression_,
                              reclaimStack_,
                              filePath_,
                              detail::joinLinkPath(groupPath_, childPath));
        }

        [[nodiscard]] BasicLink<FileType> link(std::string_view childPath) const {
            return BasicLink<FileType>(fileHandle_,
                                       plists_,
                                       fileAccess_,
                                       compression_,
                                       reclaimStack_,
                                       filePath_,
                                       detail::joinLinkPath(groupPath_, childPath));
        }

        [[nodiscard]] BasicDataset<FileType> dataset(std::string_view dsetPath) const {
            return BasicDataset<FileType>(fileHandle_, plists_, fileAccess_, compression_, detail::joinLinkPath(groupPath_, dsetPath));
        }

        [[nodiscard]] BasicTable<FileType> table(std::string_view tablePath) const {
            return BasicTable<FileType>(fileHandle_, plists_, reclaimStack_, detail::joinLinkPath(groupPath_, tablePath));
        }

        [[nodiscard]] std::vector<std::string>
            findLinks(std::string_view searchKey = "", long maxHits = -1, long maxDepth = -1, bool followSymlinks = false) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_UNKNOWN>(fileHandle_, searchKey, groupPath_, maxHits, maxDepth, followSymlinks, plists_);
        }

        [[nodiscard]] std::vector<std::string>
            findDatasets(std::string_view searchKey = "", long maxHits = -1, long maxDepth = -1, bool followSymlinks = false) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_DATASET>(fileHandle_, searchKey, groupPath_, maxHits, maxDepth, followSymlinks, plists_);
        }

        [[nodiscard]] std::vector<std::string>
            findGroups(std::string_view searchKey = "", long maxHits = -1, long maxDepth = -1, bool followSymlinks = false) const {
            return h5pp::hdf5::findLinks<H5O_TYPE_GROUP>(fileHandle_, searchKey, groupPath_, maxHits, maxDepth, followSymlinks, plists_);
        }

        void create() requires traits::is_mutable_file_v<FileType>
        {
            h5pp::hdf5::createGroup(fileHandle_, groupPath_, std::nullopt, plists_);
        }

        void createGroup(std::string_view childPath) requires traits::is_mutable_file_v<FileType>
        {
            group(childPath).create();
        }

        void deleteLink(std::string_view childPath) requires traits::is_mutable_file_v<FileType>
        {
            link(childPath).deleteLink();
        }

        void createSoftLink(std::string_view targetChildPath, std::string_view softChildPath) requires traits::is_mutable_file_v<FileType>
        {
            link(targetChildPath).createSoftLink(detail::joinLinkPath(groupPath_, softChildPath));
        }

        void createExternalLink(std::string_view targetFilePath, std::string_view targetLinkPath, std::string_view softChildPath)
            requires traits::is_mutable_file_v<FileType>
        {
            link(softChildPath).createExternalLink(targetFilePath, targetLinkPath);
        }

        template<typename Fields>
        [[nodiscard]] bool fieldExists(std::string_view tablePath, const Fields &fields) const {
            return table(tablePath).fieldExists(fields);
        }
    };

    template<typename FileType>
    requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    [[nodiscard]] inline BasicGroup<FileType> BasicLink<FileType>::asGroup() const {
        return BasicGroup<FileType>(fileHandle_, plists_, fileAccess_, compression_, reclaimStack_, filePath_, linkPath_);
    }

    using Group      = BasicGroup<h5pp::v2::File>;
    using ConstGroup = BasicGroup<const h5pp::v2::File>;
}
