#pragma once

#include "h5ppConvert.h"
#include <string>
#include <utility>

namespace h5pp::v2 {
    class File;

    template<typename FileType>
        requires(std::is_same_v<std::remove_cv_t<FileType>, h5pp::v2::File>)
    class BasicAttribute {
        private:
        FileType   &file_;
        std::string linkPath_;
        std::string attrName_;

        void requireWritable_(std::string_view operation) const {
            if(file_.fileAccess == h5pp::FileAccess::READONLY)
                throw h5pp::runtime_error("Attempted to {} on read-only file [{}]", operation, file_.filePath.string());
        }

        public:
        BasicAttribute(FileType &file, std::string_view linkPath, std::string_view attrName)
            : file_(file), linkPath_(linkPath), attrName_(attrName) {}

        [[nodiscard]] bool exists() const {
            return h5pp::hdf5::checkIfAttrExists(file_.openFileHandle(), linkPath_, attrName_, std::nullopt, file_.plists.linkAccess);
        }
        [[nodiscard]] AttrInfo getInfo() const {
            Options options;
            options.linkPath = h5pp::util::safe_str(linkPath_);
            options.attrName = h5pp::util::safe_str(attrName_);
            return h5pp::scan::readAttrInfo(file_.openFileHandle(), options, file_.plists);
        }
        [[nodiscard]] TypeInfo getTypeInfo() const {
            return h5pp::hdf5::getTypeInfo(file_.openFileHandle(), linkPath_, attrName_, std::nullopt, std::nullopt, file_.plists.linkAccess);
        }

        template<typename DataType>
            requires detail::MutableFile<FileType>
        void write(const DataType &data, const AttributeWriteOptions &options = AttributeWriteOptions()) {
            requireWritable_("write");
            auto legacyOptions = detail::makeLegacyOptions(linkPath_, attrName_, options);
            legacyOptions.assertWellDefined();
            auto dataInfo   = h5pp::scan::scanDataInfo(data, legacyOptions);
            auto fileHandle = file_.openFileHandle();
            auto attrInfo   = h5pp::scan::inferAttrInfo(fileHandle, data, legacyOptions, file_.plists);
            h5pp::hdf5::createAttribute(attrInfo);
            h5pp::hdf5::writeAttribute(data, dataInfo, attrInfo);
        }

        template<typename DataType>
        [[nodiscard]] DataType read(const AttributeReadOptions &options = AttributeReadOptions()) const {
            if constexpr(h5pp::type::sfinae::is_specialization_v<DataType, std::optional>) {
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
            auto legacyOptions = detail::makeLegacyOptions(linkPath_, attrName_, options);
            legacyOptions.assertWellDefined();
            auto fileHandle = file_.openFileHandle();
            auto attrInfo   = h5pp::scan::readAttrInfo(fileHandle, legacyOptions, file_.plists);
            auto dataInfo   = h5pp::scan::scanDataInfo(data, legacyOptions);
            h5pp::hdf5::resizeData(data, dataInfo, attrInfo);
            h5pp::hdf5::readAttribute(data, dataInfo, attrInfo, file_.plists);
        }
    };

    using Attribute      = BasicAttribute<h5pp::v2::File>;
    using ConstAttribute = BasicAttribute<const h5pp::v2::File>;
}
