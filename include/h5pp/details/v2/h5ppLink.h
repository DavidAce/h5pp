#pragma once

#include "h5ppAttribute.h"
#include "h5ppDataset.h"
#include "h5ppTable.h"
#include <string>

namespace h5pp::v2 {
    class File;

    template<typename FileType>
        requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    class BasicGroup;

    template<typename FileType>
        requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    class BasicLink {
        private:
        FileType   &file_;
        std::string linkPath_;

        public:
        BasicLink(FileType &file, std::string_view linkPath) : file_(file), linkPath_(linkPath) {}

        [[nodiscard]] const std::string &getPath() const { return linkPath_; }
        [[nodiscard]] bool exists() const { return h5pp::hdf5::checkIfLinkExists(file_.openFileHandle(), linkPath_, file_.plists.linkAccess); }
        [[nodiscard]] LinkInfo getInfo() const {
            Options options;
            options.linkPath = h5pp::util::safe_str(linkPath_);
            return h5pp::scan::readLinkInfo(file_.openFileHandle(), options, file_.plists);
        }
        [[nodiscard]] std::vector<TypeInfo> getTypeInfoAttributes() const {
            return h5pp::hdf5::getTypeInfo_allAttributes(file_.openFileHandle(), linkPath_, std::nullopt, file_.plists.linkAccess);
        }
        [[nodiscard]] std::vector<std::string> getAttributeNames() const {
            return h5pp::hdf5::getAttributeNames(file_.openFileHandle(), linkPath_, std::nullopt, file_.plists.linkAccess);
        }
        [[nodiscard]] std::vector<std::string> findAttributes(std::string_view searchKey = "") const {
            std::vector<std::string> matches;
            for(const auto &attrName : getAttributeNames()) {
                if(searchKey.empty() or attrName.find(searchKey) != std::string::npos) matches.emplace_back(attrName);
            }
            return matches;
        }
        [[nodiscard]] BasicAttribute<FileType> attribute(std::string_view attrName) const {
            return BasicAttribute<FileType>(file_, linkPath_, attrName);
        }

        [[nodiscard]] BasicDataset<FileType> asDataset() const { return BasicDataset<FileType>(file_, linkPath_); }
        [[nodiscard]] BasicTable<FileType> asTable() const { return BasicTable<FileType>(file_, linkPath_); }
        [[nodiscard]] BasicGroup<FileType> asGroup() const;

        void deleteLink()
            requires detail::MutableFile<FileType>
        {
            h5pp::hdf5::deleteLink(file_.openFileHandle(), linkPath_, file_.plists.linkAccess);
        }

        void copyToFile(const h5pp::fs::path &targetFilePath,
                        std::string_view      targetLinkPath,
                        const FileAccess     &perm = FileAccess::READWRITE) const {
            h5pp::hdf5::copyLink(file_.getFilePath(), linkPath_, targetFilePath, targetLinkPath, perm, file_.plists);
        }

        void copyFromFile(const h5pp::fs::path &sourceFilePath, std::string_view sourceLinkPath) {
            h5pp::hdf5::copyLink(sourceFilePath, sourceLinkPath, file_.getFilePath(), linkPath_, h5pp::FileAccess::READWRITE, file_.plists);
        }

        void copyTo(std::string_view targetLinkPath, const FileAccess &perm = FileAccess::READWRITE) const {
            static_cast<void>(perm);
            h5pp::hdf5::copyLink(file_.openFileHandle(), linkPath_, file_.openFileHandle(), targetLinkPath, file_.plists);
        }

        template<typename h5x_tgt>
        void copyToLocation(const h5x_tgt &targetLocationId, std::string_view targetLinkPath) const {
            h5pp::hdf5::copyLink(file_.openFileHandle(), linkPath_, targetLocationId, targetLinkPath, file_.plists);
        }

        template<typename h5x_src>
        void copyFromLocation(const h5x_src &sourceLocationId, std::string_view sourceLinkPath) {
            h5pp::hdf5::copyLink(sourceLocationId, sourceLinkPath, file_.openFileHandle(), linkPath_, file_.plists);
        }

        void moveToFile(const h5pp::fs::path &targetFilePath,
                        std::string_view      targetLinkPath,
                        const FileAccess     &perm = FileAccess::READWRITE)
            requires detail::MutableFile<FileType>
        {
            h5pp::hdf5::moveLink(file_.getFilePath(), linkPath_, targetFilePath, targetLinkPath, perm, file_.plists);
        }

        void moveFromFile(const h5pp::fs::path &sourceFilePath, std::string_view sourceLinkPath)
            requires detail::MutableFile<FileType>
        {
            h5pp::hdf5::moveLink(sourceFilePath, sourceLinkPath, file_.getFilePath(), linkPath_, h5pp::FileAccess::READWRITE, file_.plists);
        }

        void moveTo(std::string_view targetLinkPath, const FileAccess &perm = FileAccess::READWRITE)
            requires detail::MutableFile<FileType>
        {
            static_cast<void>(perm);
            h5pp::hdf5::moveLink(file_.openFileHandle(), linkPath_, file_.openFileHandle(), targetLinkPath, LocationMode::DETECT, file_.plists);
        }

        template<typename h5x_tgt>
        void moveToLocation(const h5x_tgt &targetLocationId, std::string_view targetLinkPath, LocationMode locMode = LocationMode::DETECT)
            requires detail::MutableFile<FileType>
        {
            h5pp::hdf5::moveLink(file_.openFileHandle(), linkPath_, targetLocationId, targetLinkPath, locMode, file_.plists);
        }

        template<typename h5x_src>
        void moveFromLocation(const h5x_src &sourceLocationId, std::string_view sourceLinkPath, LocationMode locMode = LocationMode::DETECT)
            requires detail::MutableFile<FileType>
        {
            h5pp::hdf5::moveLink(sourceLocationId, sourceLinkPath, file_.openFileHandle(), linkPath_, locMode, file_.plists);
        }

        void createSoftLink(std::string_view softLinkPath)
            requires detail::MutableFile<FileType>
        {
            std::string targetLinkFullPath = linkPath_.front() == '/' ? linkPath_ : h5pp::format("/{}", linkPath_);
            h5pp::hdf5::createSoftLink(targetLinkFullPath, file_.openFileHandle(), softLinkPath, file_.plists);
        }

        void createExternalLink(std::string_view targetFilePath, std::string_view targetLinkPath)
            requires detail::MutableFile<FileType>
        {
#if __cplusplus > 201703L
            if(fs::path(targetFilePath).is_relative()) {
                auto prox = fs::proximate(targetFilePath, file_.getFilePath());
                if(prox != targetFilePath) {
                    h5pp::logger::log->debug(
                        "External link target [{}] is not relative to current file [{}]. This may create a dangling external link",
                        targetFilePath,
                        file_.getFilePath());
                }
            }
#endif
            h5pp::hdf5::createExternalLink(targetFilePath, targetLinkPath, file_.openFileHandle(), linkPath_, file_.plists);
        }
    };

    using Link      = BasicLink<h5pp::v2::File>;
    using ConstLink = BasicLink<const h5pp::v2::File>;
}
