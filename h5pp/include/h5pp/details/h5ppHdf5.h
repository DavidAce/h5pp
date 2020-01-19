#pragma once
#include "h5ppDatasetProperties.h"
#include "h5ppEigen.h"
#include "h5ppLogger.h"
#include "h5ppPropertyLists.h"
#include "h5ppTypeSfinae.h"
#include "h5ppUtils.h"
#include <hdf5.h>

namespace h5pp::hdf5 {

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

    inline std::vector<hsize_t> getDimensions(const hid::h5s &space) {
        int                  ndims = H5Sget_simple_extent_ndims(space);
        std::vector<hsize_t> dims(ndims);
        H5Sget_simple_extent_dims(space, dims.data(), nullptr);
        return dims;
    }

    inline std::vector<hsize_t> getDimensions(const hid::h5d &dataset) {
        hid::h5s space = H5Dget_space(dataset);
        return getDimensions(space);
    }

    inline std::vector<hsize_t> getDimensions(const DatasetProperties &dsetProps) { return getDimensions(dsetProps.dataSet); }

    inline std::vector<hsize_t> getMaxDimensions(const hid::h5s &space) {
        int                  ndims = H5Sget_simple_extent_ndims(space);
        std::vector<hsize_t> maxdims(ndims);
        H5Sget_simple_extent_dims(space, nullptr, maxdims.data());
        return maxdims;
    }

    inline std::vector<hsize_t> getMaxDimensions(const hid::h5d &dataset) {
        hid::h5s space = H5Dget_space(dataset);
        return getMaxDimensions(space);
    }

    inline std::vector<hsize_t> getMaxDimensions(const DatasetProperties &dsetProps) {
        std::vector<hsize_t> maxdims(dsetProps.ndims.value());
        H5Sget_simple_extent_dims(dsetProps.dataSpace, nullptr, maxdims.data());
        return maxdims;
    }

    inline bool checkIfLinkExists(const hid::h5f &file, std::string_view linkName, std::optional<bool> linkExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        if(linkExists) return linkExists.value();
        h5pp::logger::log->trace("Checking if link exists: [{}]", linkName);
        for(const auto &subPath : pathCumulativeSplit(linkName, "/")) {
            int exists = H5Lexists(file, std::string(subPath).c_str(), plists.link_access);
            if(exists == 0) {
                h5pp::logger::log->trace("Checking if link exists: [{}] ... {}", linkName, false);
                return false;
            }
            if(exists < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to check if link exists: [" + std::string(linkName) + "]");
            }
        }
        h5pp::logger::log->trace("Checking if link exists: [{}] ... {}", linkName, true);
        return true;
    }

    inline bool
        checkIfDatasetExists(const hid::h5f &file, std::string_view dsetName, std::optional<bool> dsetExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        if(dsetExists) return dsetExists.value();
        h5pp::logger::log->trace("Checking if dataset exists: [{}]", dsetName);
        for(const auto &subPath : pathCumulativeSplit(dsetName, "/")) {
            int exists = H5Lexists(file, std::string(subPath).c_str(), plists.link_access);
            if(exists == 0) {
                h5pp::logger::log->trace("Checking if dataset exists: [{}] ... false", dsetName);
                return false;
            }
            if(exists < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to check if dataset exists: [" + std::string(dsetName) + "]");
            }
        }
        hid::h5o   object     = H5Oopen(file, std::string(dsetName).c_str(), plists.link_access);
        H5I_type_t objectType = H5Iget_type(object);
        if(objectType != H5I_DATASET) {
            h5pp::logger::log->trace("Checking if dataset exists: [{}] ... false", dsetName);
            return false;
        } else {
            h5pp::logger::log->trace("Checking if dataset exists: [{}] ... true", dsetName);
            return true;
        }
    }

    [[nodiscard]] inline hid::h5o
        openLink(const hid::h5f &file, std::string_view linkName, std::optional<bool> linkExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        if(checkIfLinkExists(file, linkName, linkExists, plists)) {
            h5pp::logger::log->trace("Opening link: [{}]", linkName);
            hid::h5o linkObject = H5Oopen(file, std::string(linkName).c_str(), plists.link_access);
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

    template<typename h5x>
    [[nodiscard]] h5x openObject(const hid::h5f &file, std::string_view objectName, std::optional<bool> objectExists = std::nullopt, const hid::h5p &object_access = H5P_DEFAULT) {
        if(checkIfLinkExists(file, objectName, objectExists)) {
            h5pp::logger::log->trace("Opening object: [{}]", objectName);
            h5x object;
            if constexpr(std::is_same_v<h5x, hid::h5d>) object = H5Dopen(file, std::string(objectName).c_str(), object_access);
            if constexpr(std::is_same_v<h5x, hid::h5g>) object = H5Gopen(file, std::string(objectName).c_str(), object_access);
            if constexpr(std::is_same_v<h5x, hid::h5o>) object = H5Oopen(file, std::string(objectName).c_str(), object_access);

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

    bool checkEqualTypesRecursive(const hid::h5t &type1, const hid::h5t &type2) {
        // If types are compound, check recursively that all members have equal types and names
        H5T_class_t dataClass1 = H5Tget_class(type1);
        H5T_class_t dataClass2 = H5Tget_class(type1);
        if(dataClass1 == H5T_COMPOUND and dataClass2 == H5T_COMPOUND) {
            int num_members1 = H5Tget_nmembers(type1);
            int num_members2 = H5Tget_nmembers(type2);
            if(num_members1 != num_members2) return false;
            for(int idx = 0; idx < num_members1; idx++) {
                hid::h5t         t1    = H5Tget_member_type(type1, idx);
                hid::h5t         t2    = H5Tget_member_type(type2, idx);
                char *           mem1  = H5Tget_member_name(type1, idx);
                char *           mem2  = H5Tget_member_name(type2, idx);
                std::string_view n1    = mem1;
                std::string_view n2    = mem2;
                bool             equal = n1 == n2;
                H5free_memory(mem1);
                H5free_memory(mem2);
                if(not equal) return false;
                if(not checkEqualTypesRecursive(t1, t2)) return false;
            }
            return true;
        } else if(dataClass1 == dataClass2) {
            if(dataClass1 == H5T_STRING)
                return true;
            else
                return type1 == type2;
        } else {
            return false;
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
                    h5pp::logger::log->trace("Given valid compression level {}", desiredCompressionLevel.value());
                    return desiredCompressionLevel.value();
                } else {
                    h5pp::logger::log->debug("Given compression level: {} is too high. Expected value 0 (min) to 9 (max). Returning 9");
                    return 9;
                }
            } else {
                return 0;
            }
        } else {
            h5pp::logger::log->debug("Compression is not available with this HDF5 library");
            return 0;
        }
    }

    inline std::vector<std::string>
        getAttributeNames(hid::h5f &file, std::string_view linkName, std::optional<bool> linkExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        hid::h5o                 link      = openLink(file, linkName, linkExists, plists);
        unsigned int             num_attrs = H5Aget_num_attrs(link);
        std::vector<std::string> attrNames;
        for(unsigned int i = 0; i < num_attrs; i++) {
            hid::h5a attr_id  = H5Aopen_idx(link, i);
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

    inline bool checkIfAttributeExists(const hid::h5f &     file,
                                       std::string_view     linkName,
                                       std::string_view     attrName,
                                       std::optional<bool>  linkExists = std::nullopt,
                                       std::optional<bool>  attrExists = std::nullopt,
                                       const PropertyLists &plists     = PropertyLists()) {
        if(linkExists and attrExists and linkExists.value() and attrExists.value()) return true;
        hid::h5o link = openLink(file, linkName, linkExists, plists);
        h5pp::logger::log->trace("Checking if attribute [{}] exitst in link [{}]", attrName, linkName);
        bool         exists    = false;
        unsigned int num_attrs = H5Aget_num_attrs(link);

        for(unsigned int i = 0; i < num_attrs; i++) {
            hid::h5a    attr_id  = H5Aopen_idx(link, i);
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
        h5pp::logger::log->trace("Checking if attribute [{}] exitst in link [{}] ... {}", attrName, linkName, exists);
        return exists;
    }

    inline void extendDataset(hid::h5f &           file,
                              std::string_view     datasetRelativeName,
                              const int            dim,
                              const int            extent,
                              std::optional<bool>  linkExists = std::nullopt,
                              const PropertyLists &plists     = PropertyLists()) {
        hid::h5o dataset = openLink(file, datasetRelativeName, linkExists, plists);
        h5pp::logger::log->trace("Extending dataset [ {} ] dimension [{}] to extent [{}]", datasetRelativeName, dim, extent);
        // Retrieve the current size of the memSpace (act as if you don't know its size and want to append)
        hid::h5a             dataSpace = H5Dget_space(dataset);
        const int            ndims     = H5Sget_simple_extent_ndims(dataSpace);
        std::vector<hsize_t> oldDims(ndims);
        std::vector<hsize_t> newDims(ndims);
        H5Sget_simple_extent_dims(dataSpace, oldDims.data(), nullptr);
        newDims = oldDims;
        newDims[dim] += extent;
        H5Dset_extent(dataset, newDims.data());
    }

    template<typename DataType>
    void extendDataset(hid::h5f &file, const DataType &data, std::string_view datasetRelativeName) {
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_core<DataType>::value) {
            extendDataset(file, datasetRelativeName, 0, data.rows());
            hid::h5o             dataSet   = openLink(file, datasetRelativeName);
            hid::h5s             fileSpace = H5Dget_space(dataSet);
            int                  ndims     = H5Sget_simple_extent_ndims(fileSpace);
            std::vector<hsize_t> dims(ndims);
            H5Sget_simple_extent_dims(fileSpace, dims.data(), nullptr);
            H5Sclose(fileSpace);
            if(dims[1] < (hsize_t) data.cols()) extendDataset(file, datasetRelativeName, 1, data.cols());
        } else
#endif
        {
            extendDataset(file, datasetRelativeName, 0, h5pp::utils::getSize(data));
        }
    }

    inline void createGroup(hid::h5f &file, std::string_view groupRelativeName, std::optional<bool> linkExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        // Check if group exists already
        linkExists = checkIfLinkExists(file, groupRelativeName, linkExists, plists);
        if(linkExists.value()) {
            h5pp::logger::log->trace("Group already exists: {}", groupRelativeName);
            return;
        } else {
            h5pp::logger::log->trace("Creating group link: {}", groupRelativeName);
            hid::h5g group = H5Gcreate(file, std::string(groupRelativeName).c_str(), plists.link_create, plists.group_create, plists.group_access);
        }
    }

    inline void writeSymbolicLink(hid::h5f &file, std::string_view src_path, std::string_view tgt_path, const PropertyLists &plists = PropertyLists()) {
        if(checkIfLinkExists(file, src_path, std::nullopt, plists)) {
            h5pp::logger::log->trace("Creating symbolik link: [{}] --> [{}]", src_path, tgt_path);
            herr_t retval = H5Lcreate_soft(std::string(src_path).c_str(), file, std::string(tgt_path).c_str(), plists.link_create, plists.link_access);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                h5pp::logger::log->error("Failed to write symbolic link: {}", src_path);
                throw std::runtime_error("Failed to write symbolic link:  " + std::string(src_path));
            }
        } else {
            throw std::runtime_error("Trying to write soft link to non-existing path: " + std::string(src_path));
        }
    }

    inline void setDatasetCreationPropertyLayout(const DatasetProperties &dsetProps) {
        h5pp::logger::log->trace("Setting layout in dataset creation property list");
        herr_t err = H5Pset_layout(dsetProps.plist_dset_create, dsetProps.layout.value());
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Could not set layout");
        }
        if(dsetProps.layout.value() == H5D_CHUNKED) {
            h5pp::logger::log->trace("Setting chunk dimensions in dataset creation property list");
            err = H5Pset_chunk(dsetProps.plist_dset_create, dsetProps.ndims.value(), dsetProps.chunkDims.value().data());
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Could not set chunk dimensions");
            }
        }
    }

    inline void setDatasetCreationPropertyCompression(const DatasetProperties &dsetProps) {
        h5pp::logger::log->trace("Setting compression in dataset creation property list");
        // We assume that compression level is nonzero only if compression is actually available.
        // We do not check it here, but H5Pset_deflate will return an error if zlib is not enabled.
        if(dsetProps.compressionLevel and dsetProps.compressionLevel.value() == 0 and dsetProps.compressionLevel.value() < 10) {
            if(dsetProps.layout.value() == H5D_CHUNKED) {
                herr_t err = H5Pset_deflate(dsetProps.plist_dset_create, dsetProps.compressionLevel.value());
                if(err < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    h5pp::logger::log->error("Failed to set compression level. Check that your HDF5 version has zlib enabled.");
                    throw std::runtime_error("Failed to set compression level. Check that your HDF5 version has zlib enabled.");
                } else {
                    h5pp::logger::log->trace("Compression set to level {}", dsetProps.compressionLevel.value());
                }
            } else {
                h5pp::logger::log->trace("Compression ignored: Layout is not H5D_CHUNKED");
            }
        } else if(dsetProps.compressionLevel) {
            h5pp::logger::log->warn("Invalid compression level", dsetProps.compressionLevel.value());
        }
    }

    inline void setDatasetExtent(const DatasetProperties &dsetProps) {
        h5pp::logger::log->trace("Setting extent on dataset");
        if(not dsetProps.dataSet) throw std::runtime_error("Extent can only be set on a valid dataset id. Got: " + std::to_string(dsetProps.dataSet));
        // Setting extent works only on chunked datasets
        if(dsetProps.layout.value() == H5D_CHUNKED) {
            h5pp::logger::log->trace("Setting dataset extent: {}", dsetProps.dims.value());
            herr_t err = H5Dset_extent(dsetProps.dataSet, dsetProps.dims.value().data());
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Could not set extent");
            }
        } else {
            h5pp::logger::log->trace("Extent ignored: Layout not H5D_CHUNKED");
        }
    }

    inline void setDataSpaceExtent(const DatasetProperties &dsetProps) {
        if(not dsetProps.dataSpace) throw std::runtime_error("Extent can only be set on a valid dataspace id");
        // Setting extent works only on chunked layouts
        if(dsetProps.layout.value() == H5D_CHUNKED) {
            h5pp::logger::log->trace("Setting dataspace extents: {}", dsetProps.dims.value());
            std::vector<hsize_t> maxDims(dsetProps.ndims.value());
            std::fill_n(maxDims.begin(), dsetProps.ndims.value(), H5S_UNLIMITED);
            herr_t err = H5Sset_extent_simple(dsetProps.dataSpace, dsetProps.ndims.value(), dsetProps.dims.value().data(), maxDims.data());
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                h5pp::logger::log->error("Could not set extent on dataspace");
                throw std::runtime_error("Could not set extent on dataspace");
            }
        } else {
            h5pp::logger::log->trace("Layout not H5D_CHUNKED: skip setting extent on dataspace");
        }
    }

    inline void createDataset(const hid::h5f &file, DatasetProperties &dsetProps, const PropertyLists &plists = PropertyLists()) {
        // Here we create, or register, the dataset id and set its properties before writing data to it.
        if(dsetProps.dsetExists and dsetProps.dsetExists.value()) return;
        if(dsetProps.dataSet) return;
        h5pp::logger::log->trace("Creating dataset: [{}]", dsetProps.dsetName.value());
        dsetProps.dataSet = H5Dcreate(
            file, dsetProps.dsetName.value().c_str(), dsetProps.dataType, dsetProps.dataSpace, plists.link_create, dsetProps.plist_dset_create, dsetProps.plist_dset_access);
    }

    inline void createAttribute(AttributeProperties &attrProps) {
        // Here we create, or register, the attribute id and set its properties before writing data to it.
        if(attrProps.attrExists and attrProps.attrExists.value()) return;
        if(attrProps.attributeId) return;
        h5pp::logger::log->trace("Creating attribute: [{}]", attrProps.attrName.value());
        attrProps.attributeId =
            H5Acreate(attrProps.linkObject, attrProps.attrName.value().c_str(), attrProps.dataType, attrProps.memSpace, attrProps.plist_attr_create, attrProps.plist_attr_access);
    }

    inline void selectHyperslab(const hid::h5s &fileSpace, const hid::h5s &memSpace) {
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

    inline std::vector<std::string> getContentsOfGroup(hid::h5f &file, std::string_view groupName) {
        h5pp::logger::log->trace("Getting contents of group: {}", groupName);
        std::vector<std::string> linkNames;
        try {
            herr_t err = H5Literate_by_name(file, std::string(groupName).c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, nullptr, fileInfo, &linkNames, H5P_DEFAULT);
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to iterate group: " + std::string(groupName));
            }
        } catch(std::exception &ex) { h5pp::logger::log->debug("Failed to get contents: {}", ex.what()); }
        return linkNames;
    }

    template<typename DataType>
    void writeDataset(const DataType &data, const DatasetProperties &props, const PropertyLists &plists = PropertyLists()) {
        h5pp::logger::log->debug("Writing dataset: [{}] | size {} | bytes {} | ndims {} | dims {} | type {}",
                                 props.dsetName.value(),
                                 props.size.value(),
                                 props.bytes.value(),
                                 props.ndims.value(),
                                 props.dims.value(),
                                 h5pp::type::sfinae::type_name<DataType>());
        h5pp::utils::assertBytesPerElemMatch<DataType>(props.dataType);
        if(props.dsetExists.value() and props.layout != H5D_CHUNKED) { h5pp::utils::assertBytesMatchTotal(data, props.dataSet); }
        herr_t retval = 0;

#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_colmajor<DataType>::value and not h5pp::type::sfinae::is_eigen_1d<DataType>::value) {
            h5pp::logger::log->debug("Converting data to row-major storage order");
            const auto tempRowm = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::writeDataset(tempRowm, props, plists);
        } else
#endif
            if constexpr(h5pp::type::sfinae::has_data<DataType>::value) {
            retval = H5Dwrite(props.dataSet, props.dataType, props.memSpace, props.fileSpace, plists.dset_xfer, data.data());
        } else if constexpr(std::is_pointer_v<DataType>) {
            retval = H5Dwrite(props.dataSet, props.dataType, props.memSpace, props.fileSpace, plists.dset_xfer, data);
        } else {
            retval = H5Dwrite(props.dataSet, props.dataType, props.memSpace, props.fileSpace, plists.dset_xfer, &data);
        }

        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to write number to file");
        }
    }

    template<typename DataType>
    void readDataset(DataType &data, const DatasetProperties &props, const PropertyLists &plists = PropertyLists()) {
        h5pp::logger::log->debug("Reading dataset: [{}] | size {} | bytes {} | ndims {} | dims {} | type {}",
                                 props.dsetName.value(),
                                 props.size.value(),
                                 props.bytes.value(),
                                 props.ndims.value(),
                                 props.dims.value(),
                                 type::sfinae::type_name<DataType>());
        h5pp::utils::assertBytesPerElemMatch<DataType>(props.dataType);
        herr_t retval = 0;
        // Resize/transpose the data container before writing

#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_dense<DataType>::value) {
            if constexpr(h5pp::type::sfinae::is_eigen_1d<DataType>::value) {
                data.resize(props.dims.value()[0]);
            } else if constexpr(h5pp::type::sfinae::is_eigen_rowmajor<DataType>::value) {
                // Data is RowMajor in HDF5, user gave a RowMajor container so no need to swap layout.
                data.resize(props.dims.value()[0], props.dims.value()[1]);
            } else if constexpr(h5pp::type::sfinae::is_eigen_colmajor<DataType>::value) {
                // Data is RowMajor in HDF5, user gave a ColMajor container we need to swap layout.
                h5pp::logger::log->debug("Transforming data to row major");
                Eigen::Matrix<typename DataType::Scalar, DataType::RowsAtCompileTime, DataType::RowsAtCompileTime, Eigen::RowMajor> matrixRowmajor;
                readDataset(matrixRowmajor, props, plists);
                data = matrixRowmajor;
                return;
            } else {
                throw std::runtime_error("Error detecting matrix type");
            }
        } else if constexpr(h5pp::type::sfinae::is_eigen_tensor<DataType>()) {
            if constexpr(DataType::Options == Eigen::RowMajor) {
                // Data is RowMajor in HDF5, user gave a RowMajor container so no need to swap layout.
                auto eigenDims = eigen::copy_dims<DataType::NumDimensions>(props.dims.value());
                data.resize(eigenDims);
            } else {
                // Data is RowMajor in HDF5, user gave a ColMajor container we need to swap layout.
                h5pp::logger::log->debug("Transforming data to row major");
                Eigen::Tensor<typename DataType::Scalar, DataType::NumIndices, Eigen::RowMajor> tensorRowmajor;
                readDataset(tensorRowmajor, props, plists);
                data = eigen::to_ColMajor(tensorRowmajor);
                return;
            }
        } else
#endif // H5PP_EIGEN3

            if constexpr(h5pp::type::sfinae::is_std_vector<DataType>::value) {
            assert(props.ndims == 1 and "std::vector must be a one-dimensional datasets");
            data.resize(props.size.value());
        } else if constexpr((h5pp::type::sfinae::has_c_str<DataType>::value and h5pp::type::sfinae::has_data<DataType>::value) or std::is_same<std::string, DataType>::value) {
            assert(props.ndims == 1 and "std string needs to have 1 dimension");
            hsize_t stringsize = H5Dget_storage_size(props.dataSet);
            data.resize(stringsize);
        }

        h5pp::utils::assertBytesMatchTotal(data, props.dataSet);

        // Read the data into the container

        if constexpr(h5pp::type::sfinae::has_data<DataType>::value) {
            retval = H5Dread(props.dataSet, props.dataType, props.memSpace, props.dataSpace, plists.dset_xfer, data.data());
        } else if constexpr(std::is_arithmetic<DataType>::value) {
            retval = H5Dread(props.dataSet, props.dataType, props.memSpace, props.dataSpace, plists.dset_xfer, &data);
        } else {
            logger::log->error("Attempted to read dataset of unknown type. Name: [{}] | Type: [{}]", props.dsetName.value(), type::sfinae::type_name<DataType>());
            throw std::runtime_error("Attempted to read dataset of unknown type [" + props.dsetName.value() + "] | type [" + std::string(type::sfinae::type_name<DataType>()) +
                                     "]");
        }

        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            logger::log->error("Failed  to read dataset [{}] | type: [{}]", props.dsetName.value(), type::sfinae::type_name<DataType>());
            throw std::runtime_error("Failed to read dataset [" + props.dsetName.value() + "] | type [" + std::string(type::sfinae::type_name<DataType>()) + "]");
        }
    }

    template<typename DataType>
    void writeAttribute(const DataType &data, const AttributeProperties &props) {
        h5pp::logger::log->debug("Writing attribute: [{}] | link [{}] | size {} | bytes {} | ndims {} | dims {} | type {}",
                                 props.attrName.value(),
                                 props.linkName.value(),
                                 props.size.value(),
                                 props.bytes.value(),
                                 props.ndims.value(),
                                 props.dims.value(),
                                 type::sfinae::type_name<DataType>());
        h5pp::utils::assertBytesPerElemMatch<DataType>(props.dataType);
        if constexpr(h5pp::type::sfinae::has_c_str<DataType>::value) {
            herr_t retval = H5Awrite(props.attributeId, props.dataType, data.c_str());
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to write text attribute to link [ " + props.linkName.value() + " ]");
            }
        } else if constexpr(h5pp::type::sfinae::has_data<DataType>::value) {
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
    void readAttribute(DataType &data, const AttributeProperties &props) {
        if(not props.linkExists or not props.linkExists.value())
            throw std::runtime_error("Tried to read attribute [" + props.attrName.value() + "] on non-existing link: [" + props.linkName.value() + "]");
        if(not props.attrExists or not props.attrExists.value())
            throw std::runtime_error("Tried to read non-existing attribute [" + props.attrName.value() + "] on link: [" + props.linkName.value() + "]");

        h5pp::logger::log->debug("Reading attribute: [{}] | link {} | size {} | ndims {} | dims {} | type {}",
                                 props.attrName.value(),
                                 props.linkName.value(),
                                 props.size.value(),
                                 props.ndims.value(),
                                 props.dims.value(),
                                 type::sfinae::type_name<DataType>());
        h5pp::utils::assertBytesPerElemMatch<DataType>(props.dataType);

#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_core<DataType>::value) {
            if(data.IsRowMajor) {
                // Data is RowMajor in HDF5, user gave a RowMajor container so no need to swap layout.
                data.resize(props.dims.value()[0], props.dims.value()[1]);
                if(H5Aread(props.attributeId, props.dataType, data.data()) < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to read Eigen Matrix rowmajor dataset");
                }
            } else {
                // Data is RowMajor in HDF5, user gave a ColMajor container so we need to swap the layout.
                h5pp::logger::log->debug("Transforming data to row major");
                Eigen::Matrix<typename DataType::Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> matrixRowmajor;
                matrixRowmajor.resize(props.dims.value()[0], props.dims.value()[1]); // Data is transposed in HDF5!
                if(H5Aread(props.attributeId, props.dataType, matrixRowmajor.data()) < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to read Eigen Matrix colmajor dataset");
                }
                data = matrixRowmajor;
            }

        } else if constexpr(h5pp::type::sfinae::is_eigen_tensor<DataType>()) {
            auto eigenDims = eigen::copy_dims<DataType::NumDimensions>(props.dims.value());
            if constexpr(DataType::Options == Eigen::RowMajor) {
                // Data is RowMajor in HDF5, user gave a RowMajor container so no need to swap layout.
                data.resize(eigenDims);
                if(H5Aread(props.attributeId, props.dataType, data.data()) < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to read Eigen Tensor rowmajor dataset");
                }
            } else {
                // Data is RowMajor in HDF5, user gave a ColMajor container so we need to swap the layout.
                h5pp::logger::log->debug("Transforming data to row major");
                Eigen::Tensor<typename DataType::Scalar, DataType::NumIndices, Eigen::RowMajor> tensorRowmajor(eigenDims);
                if(H5Aread(props.attributeId, props.dataType, tensorRowmajor.data()) < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to read Eigen Tensor colmajor dataset");
                }
                data = eigen::to_ColMajor(tensorRowmajor);
            }
        } else
#endif
            if constexpr(h5pp::type::sfinae::is_std_vector<DataType>::value) {
            if(props.ndims.value() != 1) throw std::runtime_error("Vector cannot take datatypes with dimension: " + std::to_string(props.ndims.value()));
            data.resize(props.dims.value()[0]);
            if(H5Aread(props.attributeId, props.dataType, data.data()) < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to read std::vector dataset");
            }
        } else if constexpr(h5pp::type::sfinae::is_std_array<DataType>::value) {
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

        } else if constexpr(std::is_arithmetic<DataType>::value or h5pp::type::sfinae::is_std_complex<DataType>()) {
            if(H5Aread(props.attributeId, props.dataType, &data) < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to read arithmetic type dataset");
            }
        } else {
            throw std::runtime_error("Attempted to read dataset of unknown type");
        }
    }

}
