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

        public:
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
        [[nodiscard]] const std::string &getPath() const { return linkPath_; }
        [[nodiscard]] bool     exists() const { return h5pp::hdf5::checkIfLinkExists(fileHandle_, linkPath_, plists_.linkAccess); }
        [[nodiscard]] LinkInfo getInfo() const {
            Options options;
            options.linkPath = h5pp::util::safe_str(linkPath_);
            return h5pp::scan::readLinkInfo(fileHandle_, options, plists_);
        }
        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes() const {
            return h5pp::hdf5::getTypeInfo_allAttributes(fileHandle_, linkPath_, std::nullopt, plists_.linkAccess);
        }
        [[nodiscard]] std::vector<std::string> getAttributeNames() const {
            return h5pp::hdf5::getAttributeNames(fileHandle_, linkPath_, std::nullopt, plists_.linkAccess);
        }
        [[nodiscard]] std::vector<std::string> findAttributes(std::string_view searchKey = "") const {
            std::vector<std::string> matches;
            for(const auto &attrName : getAttributeNames()) {
                if(searchKey.empty() or attrName.find(searchKey) != std::string::npos) matches.emplace_back(attrName);
            }
            return matches;
        }
        [[nodiscard]] BasicAttribute<FileType> attribute(std::string_view attrName) const {
            return BasicAttribute<FileType>(fileHandle_, plists_, fileAccess_, linkPath_, attrName);
        }

        [[nodiscard]] BasicDataset<FileType> asDataset() const {
            return BasicDataset<FileType>(fileHandle_, plists_, fileAccess_, compression_, linkPath_);
        }
        [[nodiscard]] BasicTable<FileType> asTable() const { return BasicTable<FileType>(fileHandle_, plists_, reclaimStack_, linkPath_); }
        [[nodiscard]] BasicGroup<FileType>   asGroup() const;

        void deleteLink() requires detail::MutableFile<FileType>
        {
            h5pp::hdf5::deleteLink(fileHandle_, linkPath_, plists_.linkAccess);
        }

        void copyToFile(const h5pp::fs::path &targetFilePath,
                        std::string_view      targetLinkPath,
                        const FileAccess     &perm = FileAccess::READWRITE) const {
            h5pp::hdf5::copyLink(filePath_.string(), linkPath_, targetFilePath, targetLinkPath, perm, plists_);
        }

        void copyFromFile(const h5pp::fs::path &sourceFilePath, std::string_view sourceLinkPath) {
            h5pp::hdf5::copyLink(sourceFilePath, sourceLinkPath, filePath_.string(), linkPath_, h5pp::FileAccess::READWRITE, plists_);
        }

        void copyTo(std::string_view targetLinkPath, const FileAccess &perm = FileAccess::READWRITE) const {
            static_cast<void>(perm);
            h5pp::hdf5::copyLink(fileHandle_, linkPath_, fileHandle_, targetLinkPath, plists_);
        }

        template<typename h5x_tgt>
        void copyToLocation(const h5x_tgt &targetLocationId, std::string_view targetLinkPath) const {
            h5pp::hdf5::copyLink(fileHandle_, linkPath_, targetLocationId, targetLinkPath, plists_);
        }

        template<typename h5x_src>
        void copyFromLocation(const h5x_src &sourceLocationId, std::string_view sourceLinkPath) {
            h5pp::hdf5::copyLink(sourceLocationId, sourceLinkPath, fileHandle_, linkPath_, plists_);
        }

        void moveToFile(const h5pp::fs::path &targetFilePath,
                        std::string_view      targetLinkPath,
                        const FileAccess     &perm = FileAccess::READWRITE) requires detail::MutableFile<FileType>
        {
            h5pp::hdf5::moveLink(filePath_.string(), linkPath_, targetFilePath, targetLinkPath, perm, plists_);
        }

        void moveFromFile(const h5pp::fs::path &sourceFilePath, std::string_view sourceLinkPath) requires detail::MutableFile<FileType>
        {
            h5pp::hdf5::moveLink(sourceFilePath, sourceLinkPath, filePath_.string(), linkPath_, h5pp::FileAccess::READWRITE, plists_);
        }

        void moveTo(std::string_view targetLinkPath, const FileAccess &perm = FileAccess::READWRITE) requires detail::MutableFile<FileType>
        {
            static_cast<void>(perm);
            h5pp::hdf5::moveLink(fileHandle_, linkPath_, fileHandle_, targetLinkPath, LocationMode::DETECT, plists_);
        }

        template<typename h5x_tgt>
        void moveToLocation(const h5x_tgt &targetLocationId, std::string_view targetLinkPath, LocationMode locMode = LocationMode::DETECT)
            requires detail::MutableFile<FileType>
        {
            h5pp::hdf5::moveLink(fileHandle_, linkPath_, targetLocationId, targetLinkPath, locMode, plists_);
        }

        template<typename h5x_src>
        void moveFromLocation(const h5x_src &sourceLocationId, std::string_view sourceLinkPath, LocationMode locMode = LocationMode::DETECT)
            requires detail::MutableFile<FileType>
        {
            h5pp::hdf5::moveLink(sourceLocationId, sourceLinkPath, fileHandle_, linkPath_, locMode, plists_);
        }

        void createSoftLink(std::string_view softLinkPath) requires detail::MutableFile<FileType>
        {
            std::string targetLinkFullPath = linkPath_.front() == '/' ? linkPath_ : h5pp::format("/{}", linkPath_);
            h5pp::hdf5::createSoftLink(targetLinkFullPath, fileHandle_, softLinkPath, plists_);
        }

        void createExternalLink(std::string_view targetFilePath, std::string_view targetLinkPath) requires detail::MutableFile<FileType>
        {
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
        }
    };

    using Link      = BasicLink<h5pp::v2::File>;
    using ConstLink = BasicLink<const h5pp::v2::File>;
}
