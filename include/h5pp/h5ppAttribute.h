#pragma once

#include "h5ppConvert.h"
#include <string>
#include <utility>

namespace h5pp::inline v2 {
    class File;

    /**
     * @brief Attribute handle.
     *
     * Obtain an Attribute with write methods from a non-const File, Dataset, Group or Link.
     * ConstAttribute provides read-only methods. The parent object must exist on file;
     * write() creates the attribute if absent.
     *
     * Each attribute object keeps the file open and remains usable after the File
     * object is destroyed. Release these objects before File::moveFileTo().
     *
     * @code{.cpp}
     * auto attribute = file.dataset("velocity").attribute("units");
     * attribute.write("m/s");
     * auto units = attribute.read<std::string>();
     * @endcode
     *
     * @tparam FileType File for Attribute, or const File for ConstAttribute.
     */
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
        /**
         * @brief Refer to an attribute without creating it.
         * @param file File containing the parent object.
         * @param linkPath Path to the parent dataset, group or named datatype, relative to the file root.
         * @param attrName Attribute name on that object.
         */
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

        template<typename OtherFileType>
        requires(std::is_same_v<std::remove_cv_t<OtherFileType>, h5pp::v2::File>)
        friend class BasicTable;

        BasicAttribute(const hid::h5f      &fileHandle,
                       const PropertyLists &plists,
                       FileAccess           fileAccess,
                       std::string_view     linkPath,
                       std::string_view     attrName)
            : fileHandle_(fileHandle), plists_(plists), fileAccess_(fileAccess), linkPath_(linkPath), attrName_(attrName) {}

        public:
        [[nodiscard]] const std::string &getLinkPath() const { return linkPath_; } /*!< Path of the object holding the attribute. */
        [[nodiscard]] const std::string &getName() const { return attrName_; }     /*!< Attribute name on its parent object. */
        [[nodiscard]] std::string        getFilePath() const { return hdf5::getFilePath(fileHandle_); }       /*!< Owning file path. */
        [[nodiscard]] std::string getFileName() const { return fs::path(getFilePath()).filename().string(); } /*!< Owning filename. */
        [[nodiscard]] FileAccess  getFileAccess() const { return fileAccess_; }                               /*!< File access policy. */

        [[nodiscard]] bool exists() const {
            return h5pp::hdf5::checkIfAttrExists(fileHandle_, linkPath_, attrName_, std::nullopt, plists_.linkAccess);
        } /*!< Check whether the attribute exists on its parent object. */
        [[nodiscard]] AttrInfo getInfo() const {
            Options options;
            options.linkPath = h5pp::util::safe_str(linkPath_);
            options.attrName = h5pp::util::safe_str(attrName_);
            return h5pp::scan::readAttrInfo(fileHandle_, options, plists_);
        } /*!< Read attribute metadata, including dimensions and datatype. */
        [[nodiscard]] TypeInfo getTypeInfo() const {
            return h5pp::hdf5::getTypeInfo(fileHandle_, linkPath_, attrName_, std::nullopt, std::nullopt, plists_.linkAccess);
        } /*!< Read datatype information for the attribute. */

        /**
         * @brief Write the attribute, creating it if absent.
         *
         * Dimensions and datatype are inferred from data unless overridden.
         * Existing attributes retain their dimensions and datatype.
         *
         * @param data Values to write.
         * @param options Override buffer dimensions or datatype. Specify dims for pointer data.
         * @return A copy of this object, for chained calls.
         */
        template<typename DataType>
        requires traits::is_mutable_file_v<FileType>
        BasicAttribute write(const DataType &data, const AttributeWriteOptions &options = AttributeWriteOptions()) {
            requireWritable_("write");
            auto ioOptions = detail::makeOptions(linkPath_, attrName_, options);
            ioOptions.assertWellDefined();
            auto dataInfo   = h5pp::scan::scanDataInfo(data, ioOptions);
            auto fileHandle = fileHandle_;
            auto attrInfo   = h5pp::scan::inferAttrInfo(fileHandle, data, ioOptions, plists_);
            h5pp::hdf5::createAttribute(attrInfo);
            h5pp::hdf5::writeAttribute(data, dataInfo, attrInfo);
            return *this;
        }

        /**
         * @brief Read the attribute into a returned value.
         *
         * With DataType = std::optional<T>, a missing attribute returns std::nullopt.
         * Other read errors are still reported as exceptions.
         *
         * @tparam DataType Type of the returned data.
         * @param options Override buffer dimensions or datatype.
         */
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

        /**
         * @brief Read the attribute into a supplied buffer.
         *
         * Resizable containers are resized to fit the attribute. Arrays and pointer
         * buffers must have enough space for the read.
         *
         * @param data Destination buffer.
         * @param options Override buffer dimensions or datatype. Specify dims for pointer data.
         */
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

    using Attribute      = BasicAttribute<h5pp::v2::File>;       /*!< Attribute operations obtained from a non-const File. */
    using ConstAttribute = BasicAttribute<const h5pp::v2::File>; /*!< Attribute operations obtained from a const File. */
}
