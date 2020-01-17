#pragma once
#include "h5ppDatasetProperties.h"
#include "h5ppLogger.h"
#include "h5ppPropertyLists.h"
#include "h5ppTypeScan.h"
#include "h5ppUtils.h"
#include <hdf5.h>

namespace h5pp::Hdf5 {

    std::vector<std::string_view> pathCumulativeSplit(std::string_view strv, std::string_view delim) {
        std::vector<std::string_view> output;
        size_t                        current_position = 0;
        while(current_position < strv.size()) {
            const auto found_position = strv.find_first_of(delim, current_position);
            if(current_position != found_position) { output.emplace_back(strv.substr(0, found_position)); }
            if(found_position == std::string_view::npos) break;
            current_position = found_position + 1;
        }
        return output;
    }

    inline bool checkIfLinkExists(const Hid::h5f &file, std::string_view linkName, std::optional<bool> linkExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        if(linkExists) return linkExists.value();
        h5pp::Logger::log->trace("Checking if link exists: [{}]", linkName);
        for(const auto &subPath : pathCumulativeSplit(linkName, "/")) {
            int exists = H5Lexists(file, std::string(subPath).c_str(), plists.link_access);
            if(exists == 0) {
                h5pp::Logger::log->trace("Checking if link exists: [{}] ... {}", linkName, false);
                return false;
            }
            if(exists < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to check if link exists: [" + std::string(linkName) + "]");
            }
        }
        h5pp::Logger::log->trace("Checking if link exists: [{}] ... {}", linkName, true);
        return true;
    }

    [[nodiscard]] inline Hid::h5o
        openLink(const Hid::h5f &file, std::string_view linkName, std::optional<bool> linkExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        if(checkIfLinkExists(file, linkName, linkExists, plists)) {
            h5pp::Logger::log->trace("Opening link: [{}]", linkName);
            Hid::h5o linkObject = H5Oopen(file, std::string(linkName).c_str(), plists.link_access);
            if(linkObject < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to open existing link: [" + std::string(linkName) + "]");
            } else {
                return linkObject;
            }
        } else {
            throw std::runtime_error("Link does not exist: [" + std::string(linkName) + "]");
        }
    }

    [[nodiscard]] inline Hid::h5d
        openDataset(const Hid::h5f &file, std::string_view linkName, std::optional<bool> linkExists = std::nullopt, const Hid::h5p &dset_access = H5Pcreate(H5P_DATASET_ACCESS)) {
        if(checkIfLinkExists(file, linkName, linkExists)) {
            h5pp::Logger::log->trace("Opening link: [{}]", linkName);
            Hid::h5d dataSet = H5Dopen(file, std::string(linkName).c_str(), dset_access);
            if(dataSet < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to open existing link: [" + std::string(linkName) + "]");
            } else {
                return dataSet;
            }
        } else {
            throw std::runtime_error("Link does not exist: [" + std::string(linkName) + "]");
        }
    }
    //    template<typename h5x>
    //    [[nodiscard]] h5x openObject(const Hid::h5f &    file,
    //                                 std::string_view    objectName,
    //                                 std::optional<bool> objectExists = std::nullopt,
    //                                 const Hid::h5p &    dset_access  = H5Pcreate(H5P_DATASET_ACCESS)) {
    //        if(checkIfLinkExists(file, objectName, objectExists)) {
    //            h5pp::Logger::log->trace("Opening object: [{}]", objectName);
    //            h5x object;
    //            if constexpr(std::is_same_v<h5x, Hid::h5d>) object = H5Dopen(file, std::string(objectName).c_str(), dset_access);
    //            if constexpr(std::is_same_v<h5x, Hid::h5g>) object = H5Gopen(file, std::string(objectName).c_str(), dset_access);
    //            if constexpr(std::is_same_v<h5x, Hid::h5o>) object = H5Oopen(file, std::string(objectName).c_str(), dset_access);
    //
    //            if(object < 0) {
    //                H5Eprint(H5E_DEFAULT, stderr);
    //                throw std::runtime_error("Failed to open existing object: [" + std::string(objectName) + "]");
    //            } else {
    //                return object;
    //            }
    //        } else {
    //            throw std::runtime_error("Object does not exist: [" + std::string(objectName) + "]");
    //        }
    //    }

    template<typename h5x>
    [[nodiscard]] h5x openObject(const Hid::h5f &file, std::string_view objectName, const Hid::h5p &object_access = H5P_DEFAULT, std::optional<bool> objectExists = std::nullopt) {
        if(checkIfLinkExists(file, objectName, objectExists)) {
            h5pp::Logger::log->trace("Opening object: [{}]", objectName);
            h5x object;
            if constexpr(std::is_same_v<h5x, Hid::h5d>) object = H5Dopen(file, std::string(objectName).c_str(), object_access);
            if constexpr(std::is_same_v<h5x, Hid::h5g>) object = H5Gopen(file, std::string(objectName).c_str(), object_access);
            if constexpr(std::is_same_v<h5x, Hid::h5o>) object = H5Oopen(file, std::string(objectName).c_str(), object_access);

            if(object < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to open existing object: [" + std::string(objectName) + "]");
            } else {
                return object;
            }
        } else {
            throw std::runtime_error("Object does not exist: [" + std::string(objectName) + "]");
        }
    }

    inline bool checkIfCompressionIsAvailable() {
        /*
         * Check if zlib compression is available and can be used for both
         * compression and decompression. We do not throw errors because this
         * filter is an optional part of the hdf5 library.
         */
        htri_t zlib_avail = H5Zfilter_avail(H5Z_FILTER_DEFLATE);
        if(zlib_avail) {
            unsigned int filter_info;
            H5Zget_filter_info(H5Z_FILTER_DEFLATE, &filter_info);
            bool zlib_encode = (filter_info & H5Z_FILTER_CONFIG_ENCODE_ENABLED);
            bool zlib_decode = (filter_info & H5Z_FILTER_CONFIG_DECODE_ENABLED);
            return zlib_avail and zlib_encode and zlib_decode;
        } else {
            return false;
        }
    }

    unsigned int getValidCompressionLevel(std::optional<unsigned int> desiredCompressionLevel = std::nullopt) {
        if(checkIfCompressionIsAvailable()) {
            if(desiredCompressionLevel.has_value()) {
                if(desiredCompressionLevel.value() < 10) {
                    h5pp::Logger::log->trace("Given valid compression level {}", desiredCompressionLevel.value());
                    return desiredCompressionLevel.value();
                } else {
                    h5pp::Logger::log->debug("Given compression level: {} is too high. Expected value 0 (min) to 9 (max). Returning 9");
                    return 9;
                }
            } else {
                return 0;
            }
        } else {
            h5pp::Logger::log->debug("Compression is not available with this HDF5 library");
            return 0;
        }
    }

    inline std::vector<std::string>
        getAttributeNames(Hid::h5f &file, std::string_view linkName, std::optional<bool> linkExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        Hid::h5o                 link      = openLink(file, linkName, linkExists, plists);
        unsigned int             num_attrs = H5Aget_num_attrs(link);
        std::vector<std::string> attrNames;
        for(unsigned int i = 0; i < num_attrs; i++) {
            Hid::h5a attr_id  = H5Aopen_idx(link, i);
            hsize_t  buf_size = 0;
            buf_size          = H5Aget_name(attr_id, buf_size, nullptr);
            if(buf_size >= 0) {
                std::string buf;
                buf.resize(buf_size + 1);
                H5Aget_name(attr_id, buf_size + 1, buf.data());
                attrNames.emplace_back(buf);
            }
        }
        return attrNames;
    }

    inline bool checkIfAttributeExists(const Hid::h5f &     file,
                                       std::string_view     linkName,
                                       std::string_view     attrName,
                                       std::optional<bool>  linkExists = std::nullopt,
                                       std::optional<bool>  attrExists = std::nullopt,
                                       const PropertyLists &plists     = PropertyLists()) {
        if(linkExists and attrExists and linkExists.value() and attrExists.value()) return true;
        Hid::h5o link = openLink(file, linkName, linkExists, plists);
        h5pp::Logger::log->trace("Checking if attribute [{}] exitst in link [{}]", attrName, linkName);
        bool         exists    = false;
        unsigned int num_attrs = H5Aget_num_attrs(link);

        for(unsigned int i = 0; i < num_attrs; i++) {
            Hid::h5a    attr_id  = H5Aopen_idx(link, i);
            hsize_t     buf_size = 0;
            std::string buf;
            buf_size = H5Aget_name(attr_id, buf_size, nullptr);
            buf.resize(buf_size + 1);
            H5Aget_name(attr_id, buf_size + 1, buf.data());
            std::string attr_name(buf.data());
            H5Aclose(attr_id);
            if(attrName == attr_name) {
                exists = true;
                break;
            }
        }
        h5pp::Logger::log->trace("Checking if attribute [{}] exitst in link [{}] ... {}", attrName, linkName, exists);
        return exists;
    }

    inline void extendDataset(Hid::h5f &           file,
                              std::string_view     datasetRelativeName,
                              const int            dim,
                              const int            extent,
                              std::optional<bool>  linkExists = std::nullopt,
                              const PropertyLists &plists     = PropertyLists()) {
        Hid::h5o dataset = openLink(file, datasetRelativeName, linkExists, plists);
        h5pp::Logger::log->trace("Extending dataset [ {} ] dimension [{}] to extent [{}]", datasetRelativeName, dim, extent);
        // Retrieve the current size of the memSpace (act as if you don't know its size and want to append)
        Hid::h5a             dataSpace = H5Dget_space(dataset);
        const int            ndims     = H5Sget_simple_extent_ndims(dataSpace);
        std::vector<hsize_t> oldDims(ndims);
        std::vector<hsize_t> newDims(ndims);
        H5Sget_simple_extent_dims(dataSpace, oldDims.data(), nullptr);
        newDims = oldDims;
        newDims[dim] += extent;
        H5Dset_extent(dataset, newDims.data());
    }

    inline std::vector<hsize_t> getMaxDims(const DatasetProperties &dsetProps) {
        std::vector<hsize_t> maxDims(dsetProps.ndims.value());
        H5Sget_simple_extent_dims(dsetProps.dataSpace, nullptr, maxDims.data());
        return maxDims;
    }

    template<typename DataType>
    void extendDataset(Hid::h5f &file, const DataType &data, std::string_view datasetRelativeName) {
        namespace tc = h5pp::Type::Scan;
#ifdef H5PP_EIGEN3
        if constexpr(tc::is_eigen_core<DataType>::value) {
            extendDataset(file, datasetRelativeName, 0, data.rows());
            Hid::h5o             dataSet   = openLink(file, datasetRelativeName);
            Hid::h5s             fileSpace = H5Dget_space(dataSet);
            int                  ndims     = H5Sget_simple_extent_ndims(fileSpace);
            std::vector<hsize_t> dims(ndims);
            H5Sget_simple_extent_dims(fileSpace, dims.data(), nullptr);
            H5Sclose(fileSpace);
            closeLink(dataSet);
            if(dims[1] < (hsize_t) data.cols()) extendDataset(file, datasetRelativeName, 1, data.cols());
        } else
#endif
        {
            extendDataset(file, datasetRelativeName, 0, h5pp::Utils::getSize(data));
        }
    }

    inline void createGroup(Hid::h5f &file, std::string_view groupRelativeName, std::optional<bool> linkExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        // Check if group exists already
        linkExists = checkIfLinkExists(file, groupRelativeName, linkExists, plists);
        if(linkExists.value()) {
            h5pp::Logger::log->trace("Group already exists: {}", groupRelativeName);
            return;
        } else {
            h5pp::Logger::log->trace("Creating group link: {}", groupRelativeName);
            Hid::h5g group = H5Gcreate(file, std::string(groupRelativeName).c_str(), plists.link_create, plists.group_create, plists.group_access);
        }
    }

    inline void writeSymbolicLink(Hid::h5f &file, std::string_view src_path, std::string_view tgt_path, const PropertyLists &plists = PropertyLists()) {
        if(checkIfLinkExists(file, src_path, std::nullopt, plists)) {
            h5pp::Logger::log->trace("Creating symbolik link: [{}] --> [{}]", src_path, tgt_path);
            herr_t retval = H5Lcreate_soft(std::string(src_path).c_str(), file, std::string(tgt_path).c_str(), plists.link_create, plists.link_access);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                h5pp::Logger::log->error("Failed to write symbolic link: {}", src_path);
                throw std::runtime_error("Failed to write symbolic link:  " + std::string(src_path));
            }
        } else {
            throw std::runtime_error("Trying to write soft link to non-existing path: " + std::string(src_path));
        }
    }

    inline void setDatasetCreationPropertyLayout(const DatasetProperties &dsetProps) {
        h5pp::Logger::log->trace("Setting layout in dataset creation property list");
        herr_t err = H5Pset_layout(dsetProps.plist_dset_create, dsetProps.layout.value());
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Could not set layout");
        }
        if(dsetProps.layout.value() == H5D_CHUNKED) {
            h5pp::Logger::log->trace("Setting chunk dimensions in dataset creation property list");
            err = H5Pset_chunk(dsetProps.plist_dset_create, dsetProps.ndims.value(), dsetProps.chunkDims.value().data());
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Could not set chunk dimensions");
            }
        }
    }

    inline void setDatasetCreationPropertyCompression(const DatasetProperties &dsetProps) {
        h5pp::Logger::log->trace("Setting compression in dataset creation property list");
        // We assume that compression level is nonzero only if compression is actually available.
        // We do not check it here, but H5Pset_deflate will return an error if zlib is not enabled.
        if(dsetProps.compressionLevel and dsetProps.compressionLevel.value() == 0 and dsetProps.compressionLevel.value() < 10) {
            if(dsetProps.layout.value() == H5D_CHUNKED) {
                herr_t err = H5Pset_deflate(dsetProps.plist_dset_create, dsetProps.compressionLevel.value());
                if(err < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    h5pp::Logger::log->error("Failed to set compression level. Check that your HDF5 version has zlib enabled.");
                    throw std::runtime_error("Failed to set compression level. Check that your HDF5 version has zlib enabled.");
                } else {
                    h5pp::Logger::log->trace("Compression set to level {}", dsetProps.compressionLevel.value());
                }
            } else {
                h5pp::Logger::log->trace("Compression ignored: Layout is not H5D_CHUNKED");
            }
        } else if(dsetProps.compressionLevel) {
            h5pp::Logger::log->warn("Invalid compression level", dsetProps.compressionLevel.value());
        }
    }

    inline void setDatasetExtent(const DatasetProperties &dsetProps) {
        h5pp::Logger::log->trace("Setting extent on dataset");
        if(not dsetProps.dataSet) throw std::runtime_error("Extent can only be set on a valid dataset id. Got: " + std::to_string(dsetProps.dataSet));
        // Setting extent works only on chunked datasets
        if(dsetProps.layout.value() == H5D_CHUNKED) {
            h5pp::Logger::log->trace("Setting dataset extent: {}", dsetProps.dims.value());
            herr_t err = H5Dset_extent(dsetProps.dataSet, dsetProps.dims.value().data());
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Could not set extent");
            }
        } else {
            h5pp::Logger::log->trace("Extent ignored: Layout not H5D_CHUNKED");
        }
    }

    inline void setDataSpaceExtent(const DatasetProperties &dsetProps) {
        if(not dsetProps.dataSpace) throw std::runtime_error("Extent can only be set on a valid dataspace id");
        // Setting extent works only on chunked layouts
        if(dsetProps.layout.value() == H5D_CHUNKED) {
            h5pp::Logger::log->trace("Setting dataspace extents: {}", dsetProps.dims.value());
            std::vector<hsize_t> maxDims(dsetProps.ndims.value());
            std::fill_n(maxDims.begin(), dsetProps.ndims.value(), H5S_UNLIMITED);
            herr_t err = H5Sset_extent_simple(dsetProps.dataSpace, dsetProps.ndims.value(), dsetProps.dims.value().data(), maxDims.data());
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                h5pp::Logger::log->error("Could not set extent on dataspace");
                throw std::runtime_error("Could not set extent on dataspace");
            }
        } else {
            h5pp::Logger::log->trace("Layout not H5D_CHUNKED: skip setting extent on dataspace");
        }
    }

    inline void createDataset(const Hid::h5f &file, DatasetProperties &dsetProps, const PropertyLists &plists = PropertyLists()) {
        // Here we create, or register, the dataset id and set its properties before writing data to it.
        if(dsetProps.dsetExists and dsetProps.dsetExists.value()) return;
        if(dsetProps.dataSet) return;
        h5pp::Logger::log->trace("Creating dataset: [{}]", dsetProps.dsetName.value());
        dsetProps.dataSet = H5Dcreate(
            file, dsetProps.dsetName.value().c_str(), dsetProps.dataType, dsetProps.dataSpace, plists.link_create, dsetProps.plist_dset_create, dsetProps.plist_dset_access);
    }

    inline void createAttribute(const Hid::h5f &file, AttributeProperties &attrProps) {
        // Here we create, or register, the attribute id and set its properties before writing data to it.
        if(attrProps.attrExists and attrProps.attrExists.value()) return;
        if(attrProps.attributeId) return;
        h5pp::Logger::log->trace("Creating attribute: [{}]", attrProps.attrName.value());
        attrProps.attributeId =
            H5Acreate(attrProps.linkObject, attrProps.attrName.value().c_str(), attrProps.dataType, attrProps.memSpace, attrProps.plist_attr_create, attrProps.plist_attr_access);
    }

    inline void selectHyperslab(const Hid::h5s &fileSpace, const Hid::h5s &memSpace) {
        const int            ndims = H5Sget_simple_extent_ndims(fileSpace);
        std::vector<hsize_t> memDims(ndims);
        std::vector<hsize_t> fileDims(ndims);
        std::vector<hsize_t> start(ndims);
        H5Sget_simple_extent_dims(memSpace, memDims.data(), nullptr);
        H5Sget_simple_extent_dims(fileSpace, fileDims.data(), nullptr);
        for(int i = 0; i < ndims; i++) start[i] = fileDims[i] - memDims[i];
        H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start.data(), nullptr, memDims.data(), nullptr);
    }

    inline herr_t fileInfo([[maybe_unused]] hid_t loc_id, const char *name, [[maybe_unused]] const H5L_info_t *linfo, void *opdata) {
        try {
            auto linkNames = reinterpret_cast<std::vector<std::string> *>(opdata);
            linkNames->push_back(name);
            return 0;
        } catch(...) { throw(std::logic_error("Not a group: " + std::string(name) + " loc_id: " + std::to_string(loc_id))); }
    }

    inline std::vector<std::string> getContentsOfGroup(Hid::h5f &file, std::string_view groupName) {
        h5pp::Logger::log->trace("Getting contents of group: {}", groupName);
        std::vector<std::string> linkNames;
        try {
            herr_t err = H5Literate_by_name(file, std::string(groupName).c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, nullptr, fileInfo, &linkNames, H5P_DEFAULT);
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to iterate group: " + std::string(groupName));
            }
        } catch(std::exception &ex) { h5pp::Logger::log->debug("Failed to get contents: {}", ex.what()); }
        return linkNames;
    }

    template<typename DataType>
    void writeDataset(const Hid::h5f &file, const DataType &data, const DatasetProperties &props, const PropertyLists &plists = PropertyLists()) {
        h5pp::Logger::log->debug("Writing dataset: [{}] | size {} | bytes {} | ndims {} | dims {} | type {}",
                                 props.dsetName.value(),
                                 props.size.value(),
                                 props.bytes.value(),
                                 props.ndims.value(),
                                 props.dims.value(),
                                 h5pp::Type::Scan::type_name<DataType>());

        h5pp::Utils::assertBytesPerElemMatch<DataType>(props.dataType);
        if(props.dsetExists.value()) {
            hsize_t dsetBytes = H5Dget_storage_size(props.dataSet);
            hsize_t dataBytes = props.bytes.value();
            if(dataBytes > dsetBytes and props.layout.value() != H5D_CHUNKED) {
                throw std::runtime_error("Overwriting non-chunked dataset with more bytes than allocated: Existing data = " + std::to_string(dsetBytes) +
                                         " bytes. Given data = " + std::to_string(dataBytes) + " bytes");
            }
        }

        if constexpr(h5pp::Type::Scan::hasMember_data<DataType>::value) {
            herr_t err = H5Dwrite(props.dataSet, props.dataType, props.memSpace, props.fileSpace, plists.dset_xfer, data.data());
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to write data using .data() method");
            }
        } else {
            herr_t retval = H5Dwrite(props.dataSet, props.dataType, props.memSpace, props.fileSpace, plists.dset_xfer, &data);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to write number to file");
            }
        }
    }

    template<typename DataType>
    void readDataset(const Hid::h5f &file, DataType &data, const DatasetProperties &props) {
        h5pp::Logger::log->debug("Reading dataset: [{}] | size {} | bytes {} | ndims {} | dims {} | type {}",
                                 props.dsetName.value(),
                                 props.size.value(),
                                 props.bytes.value(),
                                 props.ndims.value(),
                                 props.dims.value(),
                                 Type::Scan::type_name<DataType>());
        h5pp::Utils::assertBytesPerElemMatch<DataType>(props.dataType);
        herr_t retval;
#ifdef H5PP_EIGEN3
        if constexpr(tc::is_eigen_core<DataType>::value) {
            if(data.IsRowMajor) {
                // Data is RowMajor in HDF5, user gave a RowMajor container so no need to swap layout.
                data.resize(dims[0], dims[1]);
                retval = H5LTread_dataset(file, dsetName.c_str(), datatype, data.data());
                if(retval < 0) { throw std::runtime_error("Failed to read Eigen Matrix rowmajor dataset"); }
            } else {
                // Data is RowMajor in HDF5, user gave a ColMajor container we need to swap layout.
                Eigen::Matrix<typename DataType::Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> matrixRowmajor;
                matrixRowmajor.resize(dims[0], dims[1]); // Data is transposed in HDF5!
                retval = H5LTread_dataset(file, dsetName.c_str(), datatype, matrixRowmajor.data());
                if(retval < 0) { throw std::runtime_error("Failed to read Eigen Matrix colmajor dataset"); }
                data = matrixRowmajor;
            }
        } else if constexpr(tc::is_eigen_tensor<DataType>()) {
            auto eigenDims = Textra::copy_dims<DataType::NumDimensions>(dims);
            if constexpr(DataType::Options == Eigen::RowMajor) {
                // Data is RowMajor in HDF5, user gave a RowMajor container so no need to swap layout.
                data.resize(eigenDims);
                retval = H5LTread_dataset(file, dsetName.c_str(), datatype, data.data());
                if(retval < 0) { throw std::runtime_error("Failed to read Eigen Tensor rowmajor dataset"); }
            } else {
                // Data is RowMajor in HDF5, user gave a ColMajor container we need to swap layout.
                Eigen::Tensor<typename DataType::Scalar, DataType::NumIndices, Eigen::RowMajor> tensorRowmajor(eigenDims);
                retval = H5LTread_dataset(file, dsetName.c_str(), datatype, tensorRowmajor.data());
                if(retval < 0) { throw std::runtime_error("Failed to read Eigen Tensor colmajor dataset"); }
                data = Textra::to_ColMajor(tensorRowmajor);
            }
        } else
#endif // H5PP_EIGEN3

            if constexpr(h5pp::Type::Scan::is_std_vector<DataType>::value) {
            assert(props.ndims == 1 and "std::vector must be a one-dimensional datasets");
            data.resize(props.size.value());
            retval = H5LTread_dataset(file, props.dsetName.value().c_str(), props.dataType, data.data());
            if(retval < 0) { throw std::runtime_error("Failed to read std::vector dataset"); }
        } else if constexpr((h5pp::Type::Scan::hasMember_c_str<DataType>::value and h5pp::Type::Scan::hasMember_data<DataType>::value) or
                            std::is_same<std::string, DataType>::value) {
            assert(props.ndims <= 1 and "std string needs to have 1 dimension");
            hsize_t stringsize = H5Dget_storage_size(props.dataSet);
            data.resize(stringsize);
            retval = H5LTread_dataset(file, props.dsetName.value().c_str(), props.dataType, data.data());
            if(retval < 0) { throw std::runtime_error("Failed to read std::string dataset"); }
        } else if constexpr(h5pp::Type::Scan::hasMember_data<DataType>::value) {
            retval = H5LTread_dataset(file, props.dsetName.value().c_str(), props.dataType, data.data());
            if(retval < 0) { throw std::runtime_error("Failed to read into c-style array dataset"); }
        } else if constexpr(std::is_arithmetic<DataType>::value) {
            retval = H5LTread_dataset(file, props.dsetName.value().c_str(), props.dataType, &data);
            if(retval < 0) { throw std::runtime_error("Failed to read arithmetic type dataset"); }
        } else {
            Logger::log->error("Attempted to read dataset of unknown type. Name: [{}] | Type: [{}]", props.dsetName.value(), Type::Scan::type_name<DataType>());
            throw std::runtime_error("Attempted to read dataset of unknown type");
        }
    }

    template<typename DataType>
    void writeAttribute(const Hid::h5f &file, const DataType &data, const AttributeProperties &props) {
        h5pp::Logger::log->debug("Writing attribute: [{}] | link [{}] | size {} | bytes {} | ndims {} | dims {} | type {}",
                                 props.attrName.value(),
                                 props.linkName.value(),
                                 props.size.value(),
                                 props.bytes.value(),
                                 props.ndims.value(),
                                 props.dims.value(),
                                 Type::Scan::type_name<DataType>());
        h5pp::Utils::assertBytesPerElemMatch<DataType>(props.dataType);
        if constexpr(h5pp::Type::Scan::hasMember_c_str<DataType>::value) {
            herr_t retval = H5Awrite(props.attributeId, props.dataType, data.c_str());
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to write text attribute to link [ " + props.linkName.value() + " ]");
            }
        } else if constexpr(h5pp::Type::Scan::hasMember_data<DataType>::value) {
            herr_t retval = H5Awrite(props.attributeId, props.dataType, data.data());
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to write data attribute to link [ " + props.linkName.value() + " ]");
            }
        } else {
            herr_t retval = H5Awrite(props.attributeId, props.dataType, &data);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to write attribute to link [ " + props.linkName.value() + " ]");
            }
        }
    }

    template<typename DataType>
    void readAttribute(const Hid::h5f &file, DataType &data, const AttributeProperties &props) {
        if(not props.linkExists or not props.linkExists.value())
            throw std::runtime_error("Tried to read attribute [" + props.attrName.value() + "] on non-existing link: [" + props.linkName.value() + "]");
        if(not props.attrExists or not props.attrExists.value())
            throw std::runtime_error("Tried to read non-existing attribute [" + props.attrName.value() + "] on link: [" + props.linkName.value() + "]");

        h5pp::Logger::log->debug("Reading attribute: [{}] | link {} | size {} | ndims {} | dims {} | type {}",
                                 props.attrName.value(),
                                 props.linkName.value(),
                                 props.size.value(),
                                 props.ndims.value(),
                                 props.dims.value(),
                                 Type::Scan::type_name<DataType>());
        h5pp::Utils::assertBytesPerElemMatch<DataType>(props.dataType);

#ifdef H5PP_EIGEN3
        if constexpr(tc::is_eigen_core<DataType>::value) {
            if(data.IsRowMajor) {
                // Data is RowMajor in HDF5, user gave a RowMajor container so no need to swap layout.
                data.resize(dims[0], dims[1]);
                if(H5Aread(link_attribute, datatype, data.data()) < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to read Eigen Matrix rowmajor dataset");
                }
            } else {
                // Data is RowMajor in HDF5, user gave a ColMajor container so we need to swap the layout.
                Eigen::Matrix<typename DataType::Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> matrixRowmajor;
                matrixRowmajor.resize(dims[0], dims[1]); // Data is transposed in HDF5!
                if(H5Aread(link_attribute, datatype, matrixRowmajor.data()) < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to read Eigen Matrix colmajor dataset");
                }
                data = matrixRowmajor;
            }

        } else if constexpr(tc::is_eigen_tensor<DataType>()) {
            auto eigenDims = Textra::copy_dims<DataType::NumDimensions>(dims);
            if constexpr(DataType::Options == Eigen::RowMajor) {
                // Data is RowMajor in HDF5, user gave a RowMajor container so no need to swap layout.
                data.resize(eigenDims);
                if(H5Aread(link_attribute, datatype, data.data()) < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to read Eigen Tensor rowmajor dataset");
                }
            } else {
                // Data is RowMajor in HDF5, user gave a ColMajor container so we need to swap the layout.
                Eigen::Tensor<typename DataType::Scalar, DataType::NumIndices, Eigen::RowMajor> tensorRowmajor(eigenDims);
                if(H5Aread(link_attribute, datatype, tensorRowmajor.data()) < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to read Eigen Tensor colmajor dataset");
                }
                data = Textra::to_ColMajor(tensorRowmajor);
            }
        } else
#endif
            if constexpr(h5pp::Type::Scan::is_std_vector<DataType>::value) {
            if(props.ndims.value() != 1) throw std::runtime_error("Vector cannot take datatypes with dimension: " + std::to_string(props.ndims.value()));
            data.resize(props.dims.value()[0]);
            if(H5Aread(props.attributeId, props.dataType, data.data()) < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to read std::vector dataset");
            }
        } else if constexpr(h5pp::Type::Scan::is_std_array<DataType>::value) {
            if(props.ndims.value() != 1) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Array cannot take datatypes with dimension: " + std::to_string(props.ndims.value()));
            }
            if(H5Aread(props.attributeId, props.dataType, data.data()) < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to read std::vector dataset");
            }
        } else if constexpr(std::is_same<std::string, DataType>::value) {
            if(props.ndims.value() != 1) throw std::runtime_error("std::string expected ndims 1. Got: " + std::to_string(props.ndims.value()));
            hsize_t stringsize = H5Tget_size(props.dataType);
            data.resize(stringsize);
            if(H5Aread(props.attributeId, props.dataType, data.data()) < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to read std::string dataset");
            }

        } else if constexpr(std::is_arithmetic<DataType>::value or h5pp::Type::Scan::is_StdComplex<DataType>()) {
            if(H5Aread(props.attributeId, props.dataType, &data) < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to read arithmetic type dataset");
            }
        } else {
            throw std::runtime_error("Attempted to read dataset of unknown type");
        }
    }

}
