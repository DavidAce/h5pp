#pragma once

#include "h5ppConvert.h"
#include <string>
#include <utility>

namespace h5pp::inline v2 {
    class File;

    template<typename FileType>
    requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    class BasicAttribute {
        private:
        hid::h5f      fileHandle_;
        PropertyLists plists_;
        FileAccess    fileAccess_;
        std::string   linkPath_;
        std::string   attrName_;

        void requireWritable_(std::string_view operation) const {
            if(fileAccess_ == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to {} on read-only attribute [{}] on [{}]", operation, attrName_, linkPath_);
        }

        public:
        BasicAttribute(FileType &file, std::string_view linkPath, std::string_view attrName)
            : BasicAttribute(file.openFileHandle(), file.plists, file.getFileAccess(), linkPath, attrName) {}

        private:
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicDataset;
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicGroup;
        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicLink;

        BasicAttribute(const hid::h5f      &fileHandle,
                       const PropertyLists &plists,
                       FileAccess           fileAccess,
                       std::string_view     linkPath,
                       std::string_view     attrName)
            : fileHandle_(fileHandle), plists_(plists), fileAccess_(fileAccess), linkPath_(linkPath), attrName_(attrName) {}

        public:
        [[nodiscard]] bool exists() const {
            return h5pp::hdf5::checkIfAttrExists(fileHandle_, linkPath_, attrName_, std::nullopt, plists_.linkAccess);
        }
        [[nodiscard]] AttrInfo getInfo() const {
            Options options;
            options.linkPath = h5pp::util::safe_str(linkPath_);
            options.attrName = h5pp::util::safe_str(attrName_);
            return h5pp::scan::readAttrInfo(fileHandle_, options, plists_);
        }
        [[nodiscard]] TypeInfo getTypeInfo() const {
            return h5pp::hdf5::getTypeInfo(fileHandle_, linkPath_, attrName_, std::nullopt, std::nullopt, plists_.linkAccess);
        }

        template<typename DataType>
        requires traits::is_mutable_file_v<FileType>
        void write(const DataType &data, const AttributeWriteOptions &options = AttributeWriteOptions()) {
            requireWritable_("write");
            auto ioOptions = detail::makeOptions(linkPath_, attrName_, options);
            ioOptions.assertWellDefined();
            auto dataInfo   = h5pp::scan::scanDataInfo(data, ioOptions);
            auto fileHandle = fileHandle_;
            auto attrInfo   = h5pp::scan::inferAttrInfo(fileHandle, data, ioOptions, plists_);
            h5pp::hdf5::createAttribute(attrInfo);
            h5pp::hdf5::writeAttribute(data, dataInfo, attrInfo);
        }

        template<typename DataType>
        [[nodiscard]] DataType read(const AttributeReadOptions &options = AttributeReadOptions()) const {
            if constexpr(h5pp::traits::is_specialization_v<DataType, std::optional>) {
                if(not exists()) return std::nullopt;
                return read<typename DataType::value_type>(options);
            }
            DataType data;
            readInto(data, options);
            return data;
        }

        template<typename DataType>
        void readInto(DataType &data, const AttributeReadOptions &options = AttributeReadOptions()) const {
            static_assert(not std::is_const_v<DataType>);
            auto ioOptions = detail::makeOptions(linkPath_, attrName_, options);
            ioOptions.assertWellDefined();
            auto fileHandle = fileHandle_;
            auto attrInfo   = h5pp::scan::readAttrInfo(fileHandle, ioOptions, plists_);
            auto dataInfo   = h5pp::scan::scanDataInfo(data, ioOptions);
            h5pp::hdf5::resizeData(data, dataInfo, attrInfo);
            h5pp::hdf5::readAttribute(data, dataInfo, attrInfo, plists_);
        }
    };

    using Attribute      = BasicAttribute<h5pp::v2::File>;
    using ConstAttribute = BasicAttribute<const h5pp::v2::File>;
}
