#pragma once
#include "h5ppEigen.h"
#include "h5ppFilesystem.h"
#include "h5ppHyperSlab.h"
#include "h5ppLogger.h"
#include "h5ppMeta.h"
#include "h5ppEnums.h"
#include "h5ppPropertyLists.h"
#include "h5ppTableProperties.h"
#include "h5ppTypeInfo.h"
#include "h5ppTypeSfinae.h"
#include "h5ppUtils.h"
#include <hdf5.h>
#include <map>
#include <typeindex>
/*!
 * \brief A collection of functions to create (or get information about) datasets and attributes in HDF5 files
 */
namespace h5pp::hdf5 {

    [[nodiscard]] inline std::vector<std::string_view> pathCumulativeSplit(std::string_view strv, std::string_view delim) {
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

    template<typename h5x, typename = std::enable_if_t<std::is_base_of_v<hid::hid_base<h5x>, h5x>>>
    [[nodiscard]] std::string getName(const h5x &object) {
        std::string buf;
        ssize_t     namesize = H5Iget_name(object, nullptr, 0);
        if(namesize > 0) {
            buf.resize((size_t) namesize + 1);
            H5Iget_name(object, buf.data(), namesize + 1);
        }
        return buf;
    }

    [[nodiscard]] inline int getRank(const hid::h5s &space) { return H5Sget_simple_extent_ndims(space); }

    [[nodiscard]] inline int getRank(const hid::h5d &dset) {
        hid::h5s space = H5Dget_space(dset);
        return getRank(space);
    }

    [[nodiscard]] inline int getRank(const hid::h5a &attr) {
        hid::h5s space = H5Aget_space(attr);
        return getRank(space);
    }

    [[nodiscard]] inline hsize_t getSize(const hid::h5s &space) { return (hsize_t) H5Sget_simple_extent_npoints(space); }

    [[nodiscard]] inline hsize_t getSize(const hid::h5d &dataset) {
        hid::h5s space = H5Dget_space(dataset);
        return getSize(space);
    }

    [[nodiscard]] inline hsize_t getSize(const hid::h5a &attribute) {
        hid::h5s space = H5Aget_space(attribute);
        return getSize(space);
    }

    [[nodiscard]] inline std::vector<hsize_t> getDimensions(const hid::h5s &space) {
        int                  ndims = H5Sget_simple_extent_ndims(space);
        std::vector<hsize_t> dims((size_t) ndims);
        H5Sget_simple_extent_dims(space, dims.data(), nullptr);
        return dims;
    }

    [[nodiscard]] inline std::vector<hsize_t> getDimensions(const hid::h5d &dataset) {
        hid::h5s space = H5Dget_space(dataset);
        return getDimensions(space);
    }

    [[nodiscard]] inline std::vector<hsize_t> getDimensions(const hid::h5a &attribute) {
        hid::h5s space = H5Aget_space(attribute);
        return getDimensions(space);
    }

    [[nodiscard]] inline std::vector<hsize_t> getMaxDimensions(const hid::h5s &space) {
        int                  ndims = H5Sget_simple_extent_ndims(space);
        std::vector<hsize_t> maxdims((size_t) ndims);
        H5Sget_simple_extent_dims(space, nullptr, maxdims.data());
        return maxdims;
    }

    [[nodiscard]] inline std::vector<hsize_t> getMaxDimensions(const hid::h5d &dset) {
        hid::h5s space = H5Dget_space(dset);
        return getMaxDimensions(space);
    }

    inline herr_t H5Dvlen_get_buf_size_safe(const hid::h5d &dset, const hid::h5t &type, const hid::h5s &space, hsize_t *vlen) {
        *vlen = 0;
        if(H5Tis_variable_str(type) <= 0) return -1;
        if(H5Sget_simple_extent_type(space) != H5S_SCALAR) {
            herr_t retval = H5Dvlen_get_buf_size(dset, type, space, vlen);
            if(retval >= 0) return retval;
        }
        if(H5Dget_storage_size(dset) <= 0) return 0;

        auto                      size = H5Sget_simple_extent_npoints(space);
        std::vector<const char *> vdata{(size_t) size}; // Allocate for pointers for "size" number of strings
        // HDF5 allocates space for each string
        herr_t retval = H5Dread(dset, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, vdata.data());
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            return 0;
        }
        // Sum up the number of bytes
        size_t max_len = h5pp::constants::maxSizeContiguous;
        for(auto elem : vdata) {
            if(elem == nullptr) continue;
            *vlen += (hsize_t) strnlen(elem, max_len) + 1; // Add null-terminator
        }
        H5Dvlen_reclaim(type, space, H5P_DEFAULT, vdata.data());
        return 1;
    }

    inline herr_t H5Avlen_get_buf_size_safe(const hid::h5a &attr, const hid::h5t &type, const hid::h5s &space, hsize_t *vlen) {
        *vlen = 0;
        if(H5Tis_variable_str(type) <= 0) return -1;
        if(H5Aget_storage_size(attr) <= 0) return 0;

        auto                      size = H5Sget_simple_extent_npoints(space);
        std::vector<const char *> vdata{(size_t) size}; // Allocate pointers for "size" number of strings
        // HDF5 allocates space for each string
        herr_t retval = H5Aread(attr, type, vdata.data());
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            return 0;
        }
        // Sum up the number of bytes
        size_t max_len = h5pp::constants::maxSizeCompact;
        for(auto elem : vdata) {
            if(elem == nullptr) continue;
            *vlen += (hsize_t) strnlen(elem, max_len) + 1; // Add null-terminator
        }
        H5Dvlen_reclaim(type, space, H5P_DEFAULT, vdata.data());
        return 1;
    }

    [[nodiscard]] inline size_t getBytesPerElem(const hid::h5t &h5_type) { return H5Tget_size(h5_type); }

    [[nodiscard]] inline size_t getBytesTotal(const hid::h5s &space, const hid::h5t &type) { return getBytesPerElem(type) * getSize(space); }

    [[nodiscard]] inline size_t getBytesTotal(const hid::h5d &dset) {
        hid::h5t type  = H5Dget_type(dset);
        hid::h5s space = H5Dget_space(dset);
        if(H5Tis_variable_str(type) > 0) {
            hsize_t vlen = 0;
            herr_t  err  = H5Dvlen_get_buf_size_safe(dset, type, space, &vlen);
            if(err >= 0)
                return vlen; // Returns the total number of bytes required to store the dataset
            else
                return getBytesTotal(space, type);
        }
        return getBytesTotal(space, type);
    }

    [[nodiscard]] inline size_t getBytesTotal(const hid::h5a &attr) {
        hid::h5t type  = H5Aget_type(attr);
        hid::h5s space = H5Aget_space(attr);
        if(H5Tis_variable_str(type) > 0) {
            hsize_t vlen = 0;
            herr_t  err  = H5Avlen_get_buf_size_safe(attr, type, space, &vlen);
            if(err >= 0)
                return vlen; // Returns the total number of bytes required to store the dataset
            else
                return getBytesTotal(space, type);
        }
        return getBytesTotal(space, type);
    }

    template<typename userDataType>
    [[nodiscard]] bool checkBytesPerElemMatch(const hid::h5t &h5_type) {
        size_t dsetTypeSize = h5pp::hdf5::getBytesPerElem(h5_type);
        size_t dataTypeSize = h5pp::util::getBytesPerElem<userDataType>();
        if(H5Tget_class(h5_type) == H5T_STRING) dsetTypeSize = H5Tget_size(H5T_C_S1);
        if(dataTypeSize != dsetTypeSize) {
            // The dsetType may have been generated by H5Tpack, in which case we should check against the native type
            size_t   packedTypesize = dsetTypeSize;
            hid::h5t nativetype     = H5Tget_native_type(h5_type, H5T_DIR_ASCEND);
            dsetTypeSize            = h5pp::hdf5::getBytesPerElem(nativetype);
            if(dataTypeSize != dsetTypeSize)
                h5pp::logger::log->debug("Type size mismatch: dataset type {} bytes | given type {} bytes", dsetTypeSize, dataTypeSize);
            else
                h5pp::logger::log->warn("Detected packed HDF5 type: packed size {} bytes | native size {} bytes. This is not supported by h5pp yet!", packedTypesize, dataTypeSize);
        }
        return dataTypeSize == dsetTypeSize;
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    void assertBytesPerElemMatch(const hid::h5t &h5_type) {
        //        if(h5pp::type::sfinae::is_container_of_v<DataType,std::string>) return; // Each element is potentially a different length!
        size_t dsetTypeSize = h5pp::hdf5::getBytesPerElem(h5_type);
        size_t dataTypeSize = h5pp::util::getBytesPerElem<DataType>();
        if(H5Tget_class(h5_type) == H5T_STRING) dsetTypeSize = H5Tget_size(H5T_C_S1);
        if(dataTypeSize != dsetTypeSize) {
            // The dsetType may have been generated by H5Tpack, in which case we should check against the native type
            size_t   packedTypesize = dsetTypeSize;
            hid::h5t nativetype     = H5Tget_native_type(h5_type, H5T_DIR_ASCEND);
            dsetTypeSize            = h5pp::hdf5::getBytesPerElem(nativetype);
            if(dataTypeSize != dsetTypeSize)
                throw std::runtime_error(h5pp::format("Type size mismatch: dataset type is [{}] bytes | Type of given data is [{}] bytes", dsetTypeSize, dataTypeSize));
            else
                h5pp::logger::log->warn("Detected packed HDF5 type: packed size {} bytes | native size {} bytes. This is not supported by h5pp yet!", packedTypesize, dataTypeSize);
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] bool checkBytesMatchTotal(const DataType &data, const std::vector<hsize_t> &dims, const hid::h5d &dataset) {
        size_t dsetsize = h5pp::hdf5::getBytesTotal(dataset);
        size_t datasize = h5pp::util::getBytesTotal(data, dims);
        if(datasize != dsetsize) {
            // The dsetType may have been generated by H5Tpack, in which case we should check against the native type
            hid::h5s space      = H5Dget_space(dataset);
            hid::h5t datatype   = H5Dget_type(dataset);
            hid::h5t nativetype = H5Tget_native_type(datatype, H5T_DIR_ASCEND);
            size_t   packedsize = dsetsize;
            dsetsize            = h5pp::hdf5::getBytesTotal(space, nativetype);
            if(datasize != dsetsize) throw std::runtime_error(h5pp::format("Storage size mismatch. Existing dataset is [{}] bytes | New data is [{}] bytes", dsetsize, datasize));
            //                h5pp::logger::log->error("Storage size mismatch: hdf5 {} bytes | given {} bytes", dsetsize, datasize);
            else
                h5pp::logger::log->warn(
                    "Detected packed HDF5 type: packed total size {} bytes | native total size {} bytes. This is not supported by h5pp yet!", packedsize, datasize);
        }
        return datasize == dsetsize;
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    void assertBytesMatchTotal(const DataType &data, const std::vector<hsize_t> &dims, const hid::h5d &h5_dset) {
        //        if constexpr(std::is_pointer_v<DataType>) return; // Data shape is checked elsewhere
        hid::h5t    h5_type  = H5Dget_type(h5_dset);
        H5T_class_t h5_class = H5Tget_class(h5_type);
        if(h5_class == H5T_STRING) return;          // Strings are resized on the fly
        if(H5Tis_variable_str(h5_type) > 0) return; // Strings are resized on the fly
        size_t dsetsize = h5pp::hdf5::getBytesTotal(h5_dset);
        size_t datasize = h5pp::util::getBytesTotal(data, dims);
        if(datasize != dsetsize) {
            // The dsetType may have been generated by H5Tpack, in which case we should check against the native type
            size_t   packedsize = dsetsize;
            hid::h5s space      = H5Dget_space(h5_dset);
            hid::h5t datatype   = H5Dget_type(h5_dset);
            hid::h5t nativetype = H5Tget_native_type(datatype, H5T_DIR_ASCEND);
            dsetsize            = h5pp::hdf5::getBytesTotal(space, nativetype);
            if(datasize != dsetsize)
                throw std::runtime_error(h5pp::format("Total byte size mismatch. Existing dataset is [{}] bytes | Given buffer is [{}] bytes", dsetsize, datasize));
            else
                h5pp::logger::log->warn(
                    "Detected packed HDF5 type: packed total size {} bytes | native total size {} bytes. This is not supported by h5pp yet!", packedsize, datasize);
        }
        //            throw std::runtime_error(h5pp::format("Total byte size mismatch. Existing dataset is [{}] bytes | Given buffer is [{}] bytes", dsetsize, datasize));
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    void assertBytesMatchTotal(const DataType &data, const std::vector<hsize_t> &dims, const hid::h5a &h5_attr) {
        hid::h5t h5_type = H5Aget_type(h5_attr);
        if(H5Tis_variable_str(h5_type) > 0) return;
        size_t attrSize = h5pp::hdf5::getBytesTotal(h5_attr);
        size_t datasize = h5pp::util::getBytesTotal(data, dims);
        if(datasize != attrSize) {
            // The dsetType may have been generated by H5Tpack, in which case we should check against the native type
            size_t   packedsize = attrSize;
            hid::h5s space      = H5Aget_space(h5_attr);
            hid::h5t datatype   = H5Aget_type(h5_attr);
            hid::h5t nativetype = H5Tget_native_type(datatype, H5T_DIR_ASCEND);
            attrSize            = h5pp::hdf5::getBytesTotal(space, nativetype);
            if(datasize != attrSize)
                throw std::runtime_error(h5pp::format("Storage size mismatch. Existing attribute is [{}] bytes | Given buffer is [{}] bytes", attrSize, datasize));
            else
                h5pp::logger::log->warn(
                    "Detected packed HDF5 type: packed total size {} bytes | native total size {} bytes. This is not supported by h5pp yet!", packedsize, datasize);
        }
    }

    inline void setStringSize(const hid::h5t &h5_type, hsize_t size) {
        // Pass H5T_VARIABLE instead of size for variable length strings!
        H5T_class_t dataclass = H5Tget_class(h5_type);
        if(dataclass == H5T_STRING) {
            herr_t retval = H5Tset_size(h5_type, size);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to set size [{}] on string", size));
            }
            // The following makes sure there is a single "\0" at the end of the string when written to file.
            // Note however that size here is supposed to be the number of characters NOT including null terminator.
            retval = H5Tset_strpad(h5_type, H5T_STR_NULLTERM);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to set strpad");
            }
        }
    }

    template<typename DataType>
    inline void setStringSize(const DataType &data, const hid::h5t &h5_type, std::optional<std::vector<hsize_t>> desiredDims = std::nullopt) {
        H5T_class_t dataclass = H5Tget_class(h5_type);
        if(dataclass == H5T_STRING) {
            // The datatype may either be text or a container of text.
            // If pure text e.g. std::string or char[], then check that desiredDims matches the size of the text.
            // If container, e.g. std::vector<std::string>, then the desiredDims are interpreted as the container dimensions
            // and therefore each element is H5T_VARIABLE
            herr_t retval;
            if constexpr(h5pp::type::sfinae::is_text_v<DataType>) {
                hsize_t stringSize = 0;
                if constexpr(h5pp::type::sfinae::has_size_v<DataType>)
                    stringSize = data.size() + 1;
                else if constexpr(std::is_array_v<DataType>)
                    stringSize = h5pp::util::getArraySize(data);
                if(desiredDims) {
                    hsize_t desiredSize = std::accumulate(desiredDims->begin(), desiredDims->end(), (hsize_t) 1, std::multiplies<>());
                    if(stringSize != desiredSize) h5pp::logger::log->debug("Size mismatch in the given string size and desired string size [{}] != [{}]", stringSize, desiredSize);
                    retval = H5Tset_size(h5_type, desiredSize);
                } else {
                    retval = H5Tset_size(h5_type, H5T_VARIABLE);
                }
            } else {
                retval = H5Tset_size(h5_type, H5T_VARIABLE);
            }

            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to set size on string");
            }
            // The following makes sure there is a single "\0" at the end of the string when written to file.
            // Note however that size here is supposed to be the number of characters NOT including null terminator.
            retval = H5Tset_strpad(h5_type, H5T_STR_NULLTERM);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to set strpad");
            }
        }
    }
    template<typename h5x, typename = std::enable_if_t<std::is_same_v<h5x, hid::h5f> or std::is_same_v<h5x, hid::h5g> or std::is_same_v<h5x, hid_t>>>
    [[nodiscard]] inline bool
        checkIfLinkExists(const h5x &loc, std::string_view linkName, std::optional<bool> linkExists = std::nullopt, const hid::h5p &link_access = H5P_DEFAULT) {
        if(linkExists) return linkExists.value();
        for(const auto &subPath : pathCumulativeSplit(linkName, "/")) {
            int exists = H5Lexists(loc, std::string(subPath).c_str(), link_access);
            if(exists == 0) {
                h5pp::logger::log->trace("Checking if link exists [{}] ... false", linkName);
                return false;
            }
            if(exists < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to check if link exists [{}]", linkName));
            }
        }
        h5pp::logger::log->trace("Checking if link exists [{}] ... true", linkName);
        return true;
    }

    [[nodiscard]] inline bool
        checkIfDatasetExists(const hid::h5f &file, std::string_view dsetName, std::optional<bool> dsetExists = std::nullopt, const hid::h5p &dset_access = H5P_DEFAULT) {
        if(dsetExists) return dsetExists.value();
        for(const auto &subPath : pathCumulativeSplit(dsetName, "/")) {
            int exists = H5Lexists(file, util::safe_str(subPath).c_str(), dset_access);
            if(exists == 0) {
                h5pp::logger::log->trace("Checking if dataset exists [{}] ... false", dsetName);
                return false;
            }
            if(exists < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to check if dataset exists [{}]", dsetName));
            }
        }
        hid::h5o   object     = H5Oopen(file, util::safe_str(dsetName).c_str(), dset_access);
        H5I_type_t objectType = H5Iget_type(object);
        if(objectType != H5I_DATASET) {
            h5pp::logger::log->trace("Checking if dataset exists [{}] ... false", dsetName);
            return false;
        } else {
            h5pp::logger::log->trace("Checking if dataset exists [{}] ... true", dsetName);
            return true;
        }
    }

    [[nodiscard]] inline hid::h5o
        openObject(const hid::h5f &file, std::string_view objectName, std::optional<bool> linkExists = std::nullopt, const hid::h5p &object_access = H5P_DEFAULT) {
        if(checkIfLinkExists(file, objectName, linkExists, object_access)) {
            h5pp::logger::log->trace("Opening link [{}]", objectName);
            hid::h5o linkObject = H5Oopen(file, util::safe_str(objectName).c_str(), object_access);
            if(linkObject < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to open existing object [{}]", objectName));
            } else {
                return linkObject;
            }
        } else {
            throw std::runtime_error(h5pp::format("Object does not exist [{}]", objectName));
        }
    }

    template<typename h5x, typename h5l, typename = std::enable_if_t<std::is_same_v<h5l, hid::h5f> or std::is_same_v<h5l, hid::h5g> or std::is_same_v<h5l, hid_t>>>
    [[nodiscard]] h5x openLink(const h5l &loc, std::string_view linkName, std::optional<bool> objectExists = std::nullopt, const hid::h5p &link_access = H5P_DEFAULT) {
        if(checkIfLinkExists(loc, linkName, objectExists)) {
            h5pp::logger::log->trace("Opening link [{}]", linkName);
            h5x object;
            if constexpr(std::is_same_v<h5x, hid::h5d>) object = H5Dopen(loc, util::safe_str(linkName).c_str(), link_access);
            if constexpr(std::is_same_v<h5x, hid::h5g>) object = H5Gopen(loc, util::safe_str(linkName).c_str(), link_access);
            if constexpr(std::is_same_v<h5x, hid::h5o>) object = H5Oopen(loc, util::safe_str(linkName).c_str(), link_access);

            if(object < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to open existing link [{}]", linkName));
            } else {
                return object;
            }
        } else {
            throw std::runtime_error(h5pp::format("Link does not exist [{}]", linkName));
        }
    }

    [[nodiscard]] inline bool checkIfAttributeExists(const hid::h5f &    file,
                                                     std::string_view    linkName,
                                                     std::string_view    attrName,
                                                     std::optional<bool> linkExists  = std::nullopt,
                                                     std::optional<bool> attrExists  = std::nullopt,
                                                     const hid::h5p &    link_access = H5P_DEFAULT) {
        if(linkExists and attrExists and linkExists.value() and attrExists.value()) return true;
        hid::h5o link = openObject(file, linkName, linkExists, link_access);
        h5pp::logger::log->trace("Checking if attribute [{}] exitst in link [{}]", attrName, linkName);
        bool exists = H5Aexists_by_name(link, std::string(".").c_str(), util::safe_str(attrName).c_str(), link_access) > 0;
        h5pp::logger::log->trace("Checking if attribute [{}] exitst in link [{}] ... {}", attrName, linkName, exists);
        return exists;
    }

    [[nodiscard]] inline bool H5Tequal_recurse(const hid::h5t &type1, const hid::h5t &type2) {
        // If types are compound, check recursively that all members have equal types and names
        H5T_class_t dataClass1 = H5Tget_class(type1);
        H5T_class_t dataClass2 = H5Tget_class(type1);
        if(dataClass1 == H5T_COMPOUND and dataClass2 == H5T_COMPOUND) {
            size_t size_type1 = H5Tget_size(type1);
            size_t size_type2 = H5Tget_size(type2);
            if(size_type1 != size_type2) return false;
            auto num_members1 = H5Tget_nmembers(type1);
            auto num_members2 = H5Tget_nmembers(type2);
            if(num_members1 != num_members2) return false;
            for(auto idx = 0; idx < num_members1; idx++) {
                hid::h5t         t1    = H5Tget_member_type(type1, (unsigned int) idx);
                hid::h5t         t2    = H5Tget_member_type(type2, (unsigned int) idx);
                char *           mem1  = H5Tget_member_name(type1, (unsigned int) idx);
                char *           mem2  = H5Tget_member_name(type2, (unsigned int) idx);
                std::string_view n1    = mem1;
                std::string_view n2    = mem2;
                bool             equal = n1 == n2;
                H5free_memory(mem1);
                H5free_memory(mem2);
                if(not equal) return false;
                if(not H5Tequal_recurse(t1, t2)) return false;
            }
            return true;
        } else if(dataClass1 == dataClass2) {
            if(dataClass1 == H5T_STRING)
                return true;
            else
                return type1 == type2;
        } else
            return false;
    }

    [[nodiscard]] inline bool checkIfCompressionIsAvailable() {
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

    [[nodiscard]] inline unsigned int getValidCompressionLevel(std::optional<unsigned int> desiredCompressionLevel = std::nullopt) {
        if(checkIfCompressionIsAvailable()) {
            if(desiredCompressionLevel) {
                if(desiredCompressionLevel.value() < 10) {
                    return desiredCompressionLevel.value();
                } else {
                    h5pp::logger::log->debug("Given compression level {} is too high. Expected value 0 (min) to 9 (max). Returning 9");
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
    template<typename h5x, typename = std::enable_if_t<std::is_same_v<h5x, hid::h5o> or std::is_same_v<h5x, hid::h5d> or std::is_same_v<h5x, hid::h5g>>>
    [[nodiscard]] inline std::vector<std::string> getAttributeNames(const h5x &link) {
        auto                     num_attrs = H5Aget_num_attrs(link);
        std::vector<std::string> attrNames;
        for(auto i = 0; i < num_attrs; i++) {
            hid::h5a attr_id  = H5Aopen_idx(link, (unsigned int) i);
            ssize_t  buf_size = 0;
            buf_size          = H5Aget_name(attr_id, (size_t) buf_size, nullptr);
            if(buf_size >= 0) {
                std::string buf;
                buf.resize((size_t) buf_size + 1);
                H5Aget_name(attr_id, (size_t) buf_size + 1, buf.data());
                attrNames.emplace_back(buf);
            }
        }
        return attrNames;
    }

    [[nodiscard]] inline std::vector<std::string>
        getAttributeNames(const hid::h5f &file, std::string_view linkName, std::optional<bool> linkExists = std::nullopt, const hid::h5p &link_access = H5P_DEFAULT) {
        hid::h5o link = openObject(file, linkName, linkExists, link_access);
        return getAttributeNames(link);
    }
    template<typename T>
    std::tuple<std::type_index, std::string, size_t> getCppType() {
        return {typeid(T), std::string(h5pp::type::sfinae::type_name<T>()), sizeof(T)};
    }

    inline std::tuple<std::type_index, std::string, size_t> getCppType(const hid::h5t &type) {
        if(H5Tequal(type, H5T_NATIVE_SHORT)) return getCppType<short>();
        if(H5Tequal(type, H5T_NATIVE_INT)) return getCppType<int>();
        if(H5Tequal(type, H5T_NATIVE_LONG)) return getCppType<long>();
        if(H5Tequal(type, H5T_NATIVE_LLONG)) return getCppType<long long>();
        if(H5Tequal(type, H5T_NATIVE_USHORT)) return getCppType<unsigned short>();
        if(H5Tequal(type, H5T_NATIVE_UINT)) return getCppType<unsigned int>();
        if(H5Tequal(type, H5T_NATIVE_ULONG)) return getCppType<unsigned long>();
        if(H5Tequal(type, H5T_NATIVE_ULLONG)) return getCppType<unsigned long long>();
        if(H5Tequal(type, H5T_NATIVE_DOUBLE)) return getCppType<double>();
        if(H5Tequal(type, H5T_NATIVE_LDOUBLE)) return getCppType<long double>();
        if(H5Tequal(type, H5T_NATIVE_FLOAT)) return getCppType<float>();
        if(H5Tequal(type, H5T_NATIVE_HBOOL)) return getCppType<bool>();
        if(H5Tequal(type, H5T_NATIVE_CHAR)) return getCppType<char>();
        if(H5Tequal_recurse(type, H5Tcopy(H5T_C_S1))) return getCppType<std::string>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_COMPLEX_SHORT)) return getCppType<std::complex<short>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_COMPLEX_INT)) return getCppType<std::complex<int>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_COMPLEX_LONG)) return getCppType<std::complex<long>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_COMPLEX_LLONG)) return getCppType<std::complex<long long>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_COMPLEX_USHORT)) return getCppType<std::complex<unsigned short>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_COMPLEX_UINT)) return getCppType<std::complex<unsigned int>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_COMPLEX_ULONG)) return getCppType<std::complex<unsigned long>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_COMPLEX_ULLONG)) return getCppType<std::complex<unsigned long long>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_COMPLEX_DOUBLE)) return getCppType<std::complex<double>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_COMPLEX_LDOUBLE)) return getCppType<std::complex<long double>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_COMPLEX_FLOAT)) return getCppType<std::complex<float>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR2_SHORT)) return getCppType<h5pp::type::compound::H5T_SCALAR2<short>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR2_INT)) return getCppType<h5pp::type::compound::H5T_SCALAR2<int>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR2_LONG)) return getCppType<h5pp::type::compound::H5T_SCALAR2<long>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR2_LLONG)) return getCppType<h5pp::type::compound::H5T_SCALAR2<long long>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR2_USHORT)) return getCppType<h5pp::type::compound::H5T_SCALAR2<unsigned short>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR2_UINT)) return getCppType<h5pp::type::compound::H5T_SCALAR2<unsigned int>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR2_ULONG)) return getCppType<h5pp::type::compound::H5T_SCALAR2<unsigned long>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR2_ULLONG)) return getCppType<h5pp::type::compound::H5T_SCALAR2<unsigned long long>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR2_DOUBLE)) return getCppType<h5pp::type::compound::H5T_SCALAR2<double>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR2_LDOUBLE)) return getCppType<h5pp::type::compound::H5T_SCALAR2<long double>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR2_FLOAT)) return getCppType<h5pp::type::compound::H5T_SCALAR2<float>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR3_SHORT)) return getCppType<h5pp::type::compound::H5T_SCALAR3<short>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR3_INT)) return getCppType<h5pp::type::compound::H5T_SCALAR3<int>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR3_LONG)) return getCppType<h5pp::type::compound::H5T_SCALAR3<long>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR3_LLONG)) return getCppType<h5pp::type::compound::H5T_SCALAR3<long long>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR3_USHORT)) return getCppType<h5pp::type::compound::H5T_SCALAR3<unsigned short>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR3_UINT)) return getCppType<h5pp::type::compound::H5T_SCALAR3<unsigned int>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR3_ULONG)) return getCppType<h5pp::type::compound::H5T_SCALAR3<unsigned long>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR3_ULLONG)) return getCppType<h5pp::type::compound::H5T_SCALAR3<unsigned long long>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR3_DOUBLE)) return getCppType<h5pp::type::compound::H5T_SCALAR3<double>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR3_LDOUBLE)) return getCppType<h5pp::type::compound::H5T_SCALAR3<long double>>();
        if(H5Tequal_recurse(type, h5pp::type::compound::H5T_SCALAR3_FLOAT)) return getCppType<h5pp::type::compound::H5T_SCALAR3<float>>();
        auto buf_size = H5Iget_name(type, nullptr, 0);
        h5pp::logger::log->debug("buf_size {}", buf_size);
        if(buf_size <= 0)
            h5pp::logger::log->debug("No C++ type match for HDF5 type");
        else {
            std::string name;
            name.resize(static_cast<std::string::size_type>(buf_size) + 1);
            H5Iget_name(type, name.data(), static_cast<size_t>(buf_size) + 1);
            H5Eprint(H5E_DEFAULT, stderr);
            h5pp::logger::log->debug("No C++ type match for HDF5 type [{}]", name);
        }

        return getCppType<std::nullopt_t>();
    }

    inline TypeInfo getTypeInfo(std::optional<std::string> objectPath, std::optional<std::string> objectName, const hid::h5s &h5_space, const hid::h5t &h5_type) {
        TypeInfo typeInfo;
        typeInfo.h5_name                                                                   = objectName;
        typeInfo.h5_path                                                                   = objectPath;
        typeInfo.h5_type                                                                   = h5_type;
        typeInfo.h5_rank                                                                   = h5pp::hdf5::getRank(h5_space);
        typeInfo.h5_size                                                                   = h5pp::hdf5::getSize(h5_space);
        typeInfo.h5_dims                                                                   = h5pp::hdf5::getDimensions(h5_space);
        std::tie(typeInfo.cpp_type_index, typeInfo.cpp_type_name, typeInfo.cpp_type_bytes) = getCppType(typeInfo.h5_type.value());
        return typeInfo;
    }

    inline TypeInfo getTypeInfo(const hid::h5d &dataset) {
        auto        buf_size = H5Iget_name(dataset, nullptr, 0);
        std::string dset_path;
        dset_path.resize((std::string::size_type) buf_size + 1);
        H5Iget_name(dataset, dset_path.data(), (size_t) buf_size + 1);
        h5pp::logger::log->trace("Collecting type info about dataset [{}]", dset_path);
        return getTypeInfo(dset_path, std::nullopt, H5Dget_space(dataset), H5Dget_type(dataset));
    }

    inline TypeInfo getTypeInfo(const hid::h5f &file, std::string_view dsetName, std::optional<bool> dsetExists = std::nullopt, const hid::h5p &dset_access = H5P_DEFAULT) {
        auto dataset = openLink<hid::h5d>(file, dsetName, dsetExists, dset_access);
        return getTypeInfo(dataset);
    }

    template<typename h5x, typename = std::enable_if_t<std::is_same_v<h5x, hid::h5f> or std::is_same_v<h5x, hid::h5g>>>
    inline TableTypeInfo getTableTypeInfo(const h5x &loc, std::string_view tableName, std::optional<bool> tableExists = std::nullopt, const hid::h5p &dset_access = H5P_DEFAULT) {
        auto          table_link = openLink<hid::h5o>(loc, tableName, tableExists, dset_access);
        hid::h5t      table_type = H5Dget_type(table_link);
        TableTypeInfo table_info;
        hsize_t       nfields, nrecords;
        H5TBget_table_info(loc, util::safe_str(tableName).c_str(), &nfields, &nrecords);

        std::vector<size_t> field_sizes(nfields);
        std::vector<size_t> field_offsets(nfields);
        size_t              table_bytes;
        char **             field_names = new char *[nfields];
        for(size_t i = 0; i < nfields; i++) field_names[i] = new char[255];
        H5TBget_field_info(loc, util::safe_str(tableName).c_str(), field_names, field_sizes.data(), field_offsets.data(), &table_bytes);

        std::vector<std::string> field_names_vec(nfields);
        std::vector<hid::h5t>    field_types;
        for(size_t i = 0; i < nfields; i++) field_names_vec[i] = field_names[i];
        for(size_t i = 0; i < nfields; i++) field_types[i] = H5Tget_member_type(table_link, i);
        /* release array of char arrays */
        for(size_t i = 0; i < nfields; i++) delete field_names[i];
        delete[] field_names;
        // Copy data to TableTypeInfo object
        table_info.nfields        = nfields;
        table_info.nrecords       = nrecords;
        table_info.recordBytes    = table_bytes;
        table_info.fieldSizes     = field_sizes;
        table_info.fieldOffsets   = field_offsets;
        table_info.h5_field_types = field_types;
        table_info.fieldNames     = field_names_vec;
        table_info.h5_table_link  = table_link;
        table_info.h5_table_type  = table_type;
        return table_info;
    }

    inline TypeInfo getTypeInfo(const hid::h5a &attribute) {
        auto        buf_size = H5Aget_name(attribute, 0, nullptr);
        std::string attr_name;
        attr_name.resize((std::string::size_type) buf_size + 1);
        H5Aget_name(attribute, (size_t) buf_size + 1, attr_name.data());
        buf_size = H5Iget_name(attribute, nullptr, 0);
        std::string link_path;
        link_path.resize((std::string::size_type) buf_size + 1);
        H5Iget_name(attribute, link_path.data(), (size_t) buf_size + 1);

        h5pp::logger::log->trace("Collecting type info about attribute [{}] in link [{}]", attr_name, link_path);
        return getTypeInfo(link_path, attr_name, H5Aget_space(attribute), H5Aget_type(attribute));
    }

    inline TypeInfo getTypeInfo(const hid::h5f &    file,
                                std::string_view    linkName,
                                std::string_view    attrName,
                                std::optional<bool> linkExists  = std::nullopt,
                                std::optional<bool> attrExists  = std::nullopt,
                                const hid::h5p &    link_access = H5P_DEFAULT) {
        auto link = openLink<hid::h5o>(file, linkName, linkExists, link_access);
        if(checkIfAttributeExists(file, linkName, attrName, linkExists, attrExists, link_access)) {
            hid::h5a attribute = H5Aopen_name(link, util::safe_str(attrName).c_str());
            return getTypeInfo(attribute);
        } else {
            throw std::runtime_error(h5pp::format("Attribute [{}] does not exist in link [{}]", attrName, linkName));
        }
    }

    template<typename h5x, typename = std::enable_if_t<std::is_same_v<h5x, hid::h5o> or std::is_same_v<h5x, hid::h5d> or std::is_same_v<h5x, hid::h5g>>>
    std::vector<TypeInfo> getTypeInfo_allAttributes(const h5x &link) {
        std::vector<TypeInfo> allAttrInfo;
        auto                  num_attrs = H5Aget_num_attrs(link);
        for(auto idx = 0; idx < num_attrs; idx++) {
            hid::h5a attribute = H5Aopen_idx(link, (unsigned int) idx);
            allAttrInfo.emplace_back(getTypeInfo(attribute));
        }
        return allAttrInfo;
    }

    inline std::vector<TypeInfo>
        getTypeInfo_allAttributes(const hid::h5f &file, std::string_view linkName, std::optional<bool> linkExists = std::nullopt, const hid::h5p &link_access = H5P_DEFAULT) {
        auto link = openLink<hid::h5o>(file, linkName, linkExists, link_access);
        return getTypeInfo_allAttributes(link);
    }

    inline void extendDataset(hid::h5f &          file,
                              std::string_view    datasetRelativeName,
                              const int           dim,
                              const hsize_t       extent,
                              std::optional<bool> linkExists  = std::nullopt,
                              const hid::h5p &    dset_access = H5P_DEFAULT) {
        auto dataset = openLink<hid::h5d>(file, datasetRelativeName, linkExists, dset_access);
        h5pp::logger::log->trace("Extending dataset [ {} ] dimension [{}] to extent [{}]", datasetRelativeName, dim, extent);
        // Retrieve the current size of the memSpace (act as if you don't know its size and want to append)
        hid::h5a             dataSpace = H5Dget_space(dataset);
        const int            ndims     = H5Sget_simple_extent_ndims(dataSpace);
        std::vector<hsize_t> oldDims((size_t) ndims);
        std::vector<hsize_t> newDims((size_t) ndims);
        H5Sget_simple_extent_dims(dataSpace, oldDims.data(), nullptr);
        newDims = oldDims;
        newDims[(size_t) dim] += extent;
        H5Dset_extent(dataset, newDims.data());
    }

    template<typename DataType>
    void extendDataset(hid::h5f &file, const DataType &data, std::string_view dsetPath) {
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_core_v<DataType>) {
            extendDataset(file, dsetPath, 0, data.rows());
            hid::h5d             dataSet   = openLink<hid::h5d>(file, dsetPath);
            hid::h5s             fileSpace = H5Dget_space(dataSet);
            int                  ndims     = H5Sget_simple_extent_ndims(fileSpace);
            std::vector<hsize_t> dims((size_t) ndims);
            H5Sget_simple_extent_dims(fileSpace, dims.data(), nullptr);
            H5Sclose(fileSpace);
            if(dims[1] < (hsize_t) data.cols()) extendDataset(file, dsetPath, 1, data.cols());
        } else
#endif
        {
            extendDataset(file, dsetPath, 0, h5pp::util::getSize(data));
        }
    }

    inline void
        createGroup(const hid::h5f &file, std::string_view groupRelativeName, std::optional<bool> linkExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        // Check if group exists already
        linkExists = checkIfLinkExists(file, groupRelativeName, linkExists, plists.link_access);
        if(linkExists.value()) {
            h5pp::logger::log->trace("Group already exists [{}]", groupRelativeName);
            return;
        } else {
            h5pp::logger::log->trace("Creating group link [{}]", groupRelativeName);
            hid::h5g group = H5Gcreate(file, util::safe_str(groupRelativeName).c_str(), plists.link_create, plists.group_create, plists.group_access);
        }
    }

    inline void writeSymbolicLink(hid::h5f &file, std::string_view src_path, std::string_view tgt_path, const PropertyLists &plists = PropertyLists()) {
        if(checkIfLinkExists(file, src_path, std::nullopt, plists.link_access)) {
            h5pp::logger::log->trace("Creating symbolic link [{}] --> [{}]", src_path, tgt_path);
            herr_t retval = H5Lcreate_soft(util::safe_str(src_path).c_str(), file, util::safe_str(tgt_path).c_str(), plists.link_create, plists.link_access);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to write symbolic link [{}]  ", src_path));
            }
        } else {
            throw std::runtime_error(h5pp::format("Trying to write soft link to non-existing path [{}]", src_path));
        }
    }

    inline void setProperty_layout(MetaDset &metaDset) {
        if(not metaDset.h5_plist_dset_create) throw std::logic_error("Could not configure the H5D layout: the dataset creation property list has not been initialized");
        if(not metaDset.h5_layout) throw std::logic_error("Could not configure the H5D layout: the H5D layout parameter has not been initialized");
        switch(metaDset.h5_layout.value()) {
            case H5D_CHUNKED: h5pp::logger::log->trace("Setting layout H5D_CHUNKED"); break;
            case H5D_COMPACT: h5pp::logger::log->trace("Setting layout H5D_COMPACT"); break;
            case H5D_CONTIGUOUS: h5pp::logger::log->trace("Setting layout H5D_CONTIGUOUS"); break;
            default: throw std::runtime_error("Given invalid layout when creating dataset property list. Choose one of H5D_COMPACT,H5D_CONTIGUOUS,H5D_CHUNKED");
        }
        herr_t err = H5Pset_layout(metaDset.h5_plist_dset_create.value(), metaDset.h5_layout.value());
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Could not set layout");
        }
    }

    inline void setProperty_chunkDims(MetaDset &metaDset) {
        if(not metaDset.chunkDims) return;
        if(not metaDset.h5_layout) throw std::logic_error("Could not configure chunk dimensions: the H5D layout has not been initialized");
        if(metaDset.h5_layout.value() != H5D_CHUNKED) {
            h5pp::logger::log->trace("Chunking ignored: Layout is not H5D_CHUNKED");
            metaDset.chunkDims = std::nullopt;
            return;
        }
        if(metaDset.chunkDims->empty()) {
            h5pp::logger::log->trace("Chunking ignored: No chunk dimensions detected");
            metaDset.chunkDims = std::nullopt;
            metaDset.h5_layout = H5D_CONTIGUOUS;
            setProperty_layout(metaDset);
            return;
        }

        if(H5Sget_simple_extent_type(metaDset.h5_space.value()) == H5S_SCALAR) {
            h5pp::logger::log->trace("Chunking ignored: Space is H5S_SCALAR");
            metaDset.chunkDims = std::nullopt;
            metaDset.h5_layout = H5D_CONTIGUOUS;
            setProperty_layout(metaDset);
            return;
        }

        if(not metaDset.h5_plist_dset_create) throw std::logic_error("Could not configure chunk dimensions: the dataset creation property list has not been initialized");
        if(not metaDset.dsetRank) throw std::logic_error("Could not configure chunk dimensions: the dataset rank (n dims) has not been initialized");
        if(not metaDset.dsetDims) throw std::logic_error("Could not configure chunk dimensions: the dataset dimensions have not been initialized");
        int rank = (int) metaDset.chunkDims->size();
        if(rank != metaDset.dsetRank.value())
            throw std::logic_error(h5pp::format("Could not configure chunk dimensions: the chunk dimensions [{}] and dataset dimensions [{}] must have equal number of elements",
                                                metaDset.dsetDims.value(),
                                                metaDset.chunkDims.value()));

        h5pp::logger::log->trace("Setting chunk dimensions {}", metaDset.chunkDims.value());
        herr_t err = H5Pset_chunk(metaDset.h5_plist_dset_create.value(), (int) metaDset.chunkDims->size(), metaDset.chunkDims->data());
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Could not set chunk dimensions");
        }
    }

    inline void setProperty_compression(MetaDset &metaDset) {
        if(not metaDset.compression) return;
        if(not checkIfCompressionIsAvailable()) return;
        if(not metaDset.h5_plist_dset_create) throw std::runtime_error("Could not configure compression: field h5_plist_dset_create has not been initialized");
        if(not metaDset.h5_layout) throw std::logic_error("Could not configure compression: field h5_layout has not been initialized");

        if(metaDset.h5_layout.value() != H5D_CHUNKED) {
            h5pp::logger::log->trace("Compression ignored: Layout is not H5D_CHUNKED");
            metaDset.compression = std::nullopt;
            return;
        }
        if(metaDset.compression and metaDset.compression.value() > 9) {
            h5pp::logger::log->warn("Compression level too high: [{}]. Reducing to [9]", metaDset.compression.value());
            metaDset.compression = 9;
        }
        h5pp::logger::log->trace("Setting compression level {}", metaDset.compression.value());
        herr_t err = H5Pset_deflate(metaDset.h5_plist_dset_create.value(), metaDset.compression.value());
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to set compression level. Check that your HDF5 version has zlib enabled.");
        }
    }

    inline void setSpaceExtent(const hid::h5s &h5_space, const std::vector<hsize_t> &dims, std::optional<std::vector<hsize_t>> maxDims = std::nullopt) {
        if(H5Sget_simple_extent_type(h5_space) == H5S_SCALAR) return;
        if(dims.empty()) return;
        herr_t err;
        if(maxDims) {
            if(dims.size() != maxDims->size()) throw std::runtime_error(h5pp::format("Rank mismatch in dimensions {} and max dimensions {}", dims, maxDims.value()));
            std::vector<long> maxDimsLong;
            for(auto &dim : maxDims.value()) {
                if(dim == H5S_UNLIMITED)
                    maxDimsLong.emplace_back(-1);
                else
                    maxDimsLong.emplace_back((long) dim);
            }
            h5pp::logger::log->trace("Setting dataspace extents: dims {} | max dims {}", dims, maxDimsLong);
            err = H5Sset_extent_simple(h5_space, (int) dims.size(), dims.data(), maxDims->data());
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to set extents on space: dims {} | max dims {}", dims, maxDims.value()));
            }
        } else {
            h5pp::logger::log->trace("Setting dataspace extents: dims {}", dims);
            err = H5Sset_extent_simple(h5_space, (int) dims.size(), dims.data(), nullptr);
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to set extents on space. Dims {}", dims));
            }
        }
    }

    inline void setSpaceExtent(MetaDset &metaDset) {
        if(not metaDset.h5_space) throw std::logic_error("Could not set space extent: the space is not initialized");
        if(not metaDset.h5_space->valid()) throw std::runtime_error("Could not set space extent. Space is not valid");
        if(H5Sget_simple_extent_type(metaDset.h5_space.value()) == H5S_SCALAR) return;
        if(not metaDset.dsetDims) throw std::runtime_error("Could not set space extent: dataset dimensions are not defined");

        if(metaDset.h5_layout and metaDset.h5_layout.value() == H5D_CHUNKED and not metaDset.dsetDimsMax) {
            // Chunked datasets are unlimited unless told explicitly otherwise
            metaDset.dsetDimsMax = std::vector<hsize_t>((size_t) metaDset.dsetRank.value(), 0);
            std::fill_n(metaDset.dsetDimsMax->begin(), metaDset.dsetDimsMax->size(), H5S_UNLIMITED);
        }
        if(metaDset.h5_layout and metaDset.h5_layout.value() != H5D_CHUNKED and metaDset.dsetDims.value() != metaDset.dsetDimsMax.value()) {
            throw std::runtime_error(h5pp::format("When h5_layout != H5D_CHUNKED on a dataset, its dimensions {} must exactly match the maximum dimensions {}",
                                                  metaDset.dsetDims.value(),
                                                  metaDset.dsetDimsMax.value()));
        }

        try {
            setSpaceExtent(metaDset.h5_space.value(), metaDset.dsetDims.value(), metaDset.dsetDimsMax);
        } catch(const std::exception &err) { throw std::runtime_error(h5pp::format("Failed to set extent on dataset {} \n Reason {}", metaDset.string(), err.what())); }
    }

    inline void resizeDataset(MetaDset &metaDset, MetaData &metaData) {
        metaDset.assertWriteReady();
        metaData.assertWriteReady();
        if(H5Tis_variable_str(metaDset.h5_type.value()) > 0) {
            // These are resized on the fly
            return;
        } else {
            if(metaDset.dsetDims.value() == metaData.dataDims.value()) return;
            auto oldSpace = metaDset.string();
            H5Dset_extent(metaDset.h5_dset.value(), metaData.dataDims->data());
            metaDset.dsetByte    = h5pp::hdf5::getBytesTotal(metaDset.h5_dset.value());
            metaDset.dsetSize    = h5pp::hdf5::getSize(metaDset.h5_dset.value());
            metaDset.dsetRank    = h5pp::hdf5::getRank(metaDset.h5_dset.value());
            metaDset.dsetDims    = h5pp::hdf5::getDimensions(metaDset.h5_dset.value());
            metaDset.dsetDimsMax = h5pp::hdf5::getMaxDimensions(metaDset.h5_dset.value());
            metaDset.h5_space    = H5Dget_space(metaDset.h5_dset->value());
            auto newSpace        = metaDset.string();
            h5pp::logger::log->debug("Resized dataset \n \t old: {} \n \t new: {}", oldSpace, newSpace);
        }
    }

    template<typename DataType>
    inline void resizeData(DataType &data, const hid::h5s &space, const hid::h5t &type, size_t bytes) {
        // This function is used when reading data from file into memory.
        // It resizes the data so the space in memory can fit the data read from file.
        // Note that this resizes the data to fit the bounding box of the data selected in the fileSpace.
        // A selection of elements in memory space must occurr after calling this function.
        if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>) return; // h5pp never uses malloc
        if(bytes == 0) return;
        if(H5Tget_class(type) == H5T_STRING) {
            //            if(H5Tis_variable_str(type) > 0)return; // These are resized on the fly
            if constexpr(h5pp::type::sfinae::is_text_v<DataType>)
                h5pp::util::resizeData(data, {(hsize_t) bytes - 1}); // Minus one: String resize allocates the null-terminator automatically
            else
                h5pp::util::resizeData(data, {(hsize_t) H5Sget_simple_extent_npoints(space)});
        } else if(H5Sget_simple_extent_type(space) == H5S_SCALAR)
            h5pp::util::resizeData(data, {(hsize_t) 1});
        else {
            int                  rank = H5Sget_simple_extent_ndims(space); // This will define the bounding box of the selected elements in the file space
            std::vector<hsize_t> extent((size_t) rank, 0);
            H5Sget_simple_extent_dims(space, extent.data(), nullptr);
            h5pp::util::resizeData(data, extent);
            if(bytes != h5pp::util::getBytesTotal(data))
                h5pp::logger::log->debug("Size mismatch after resize: data [{}] bytes | dset [{}] bytes ", h5pp::util::getBytesTotal(data), bytes);
        }
    }

    template<typename DataType>
    inline void resizeData(DataType &data, const hid::h5d &dset) {
        hid::h5s space = H5Dget_space(dset);
        hid::h5t type  = H5Dget_type(dset);
        hsize_t  bytes = getBytesTotal(dset);
        resizeData(data, space, type, bytes);
    }
    template<typename DataType>
    inline void resizeData(DataType &data, const hid::h5a &attr) {
        hid::h5s space = H5Aget_space(attr);
        hid::h5t type  = H5Aget_type(attr);
        hsize_t  bytes = getBytesTotal(attr);
        resizeData(data, space, type, bytes);
    }

    inline std::string getSpaceString(const hid::h5s &space) {
        std::string msg;
        //        msg.append(h5pp::format(" | size {}", getSize(space, type)));
        //        msg.append(h5pp::format(" | rank {}", getRank(space)));
        //        msg.append(h5pp::format(" | dims {}", getDimensions(space, type)));
        msg.append(h5pp::format(" | size {}", H5Sget_simple_extent_npoints(space)));
        int                  rank = H5Sget_simple_extent_ndims(space);
        std::vector<hsize_t> dims((size_t) rank, 0);
        H5Sget_simple_extent_dims(space, dims.data(), nullptr);
        msg.append(h5pp::format(" | rank {}", rank));
        msg.append(h5pp::format(" | dims {}", dims));
        if(H5Sget_select_type(space) == H5S_SEL_HYPERSLABS) {
            HyperSlab slab(space);
            msg.append(slab.string());
        }
        return msg;
    }

    inline void assertSpacesEqual(const hid::h5s &dataSpace, const hid::h5s &dsetSpace, const hid::h5t &h5_type) {
        if(H5Tis_variable_str(h5_type)) {
            // Strings are a special case, e.g. we can multiple string elements into just one.
            // Also space is allocated on the fly during read by HDF5.. so size comparisons are useless here.
            return;
        }
        //        if(h5_layout == H5D_CHUNKED) return; // Chunked layouts are allowed to differ
        htri_t equal = H5Sextent_equal(dataSpace, dsetSpace);
        if(equal == 0) {
            // One of the maxDims may be H5S_UNLIMITED, in which case, we just check the dimensions
            auto dataDims = getDimensions(dataSpace);
            auto dsetDims = getDimensions(dsetSpace);
            if(getDimensions(dataSpace) == getDimensions(dsetSpace)) return;
            auto dataSize = getSize(dataSpace);
            auto dsetSize = getSize(dsetSpace);
            if(dataSize != dsetSize) {
                auto msg1 = getSpaceString(dataSpace);
                auto msg2 = getSpaceString(dsetSpace);
                throw std::runtime_error(h5pp::format("Spaces are not equal size \n\t data space \t {} \n\t dset space \t {}", msg1, msg2));
            } else if(getDimensions(dataSpace) != getDimensions(dsetSpace)) {
                h5pp::logger::log->debug("Spaces have different shape:");
                h5pp::logger::log->debug("  --  data space {}", getSpaceString(dataSpace));
                h5pp::logger::log->debug("  --  dset space {}", getSpaceString(dsetSpace));
            }
        } else if(equal < 0) {
            throw std::runtime_error("Failed to compare space extents");
        }
    }

    inline void logSpaceTranfer(const hid::h5s &src, const hid::h5s &tgt) {
        if(h5pp::logger::getLogLevel() == 0) {
            auto msg_src = getSpaceString(src);
            auto msg_tgt = getSpaceString(tgt);
            if(msg_src.empty() and msg_tgt.empty()) return;
            h5pp::logger::log->trace("Source space {}", msg_src);
            h5pp::logger::log->trace("Target space {}", msg_tgt);
        }
    }

    inline void logSpaceCreate(const hid::h5s &tgt) {
        if(h5pp::logger::getLogLevel() == 0) {
            auto msg_tgt = getSpaceString(tgt);
            if(msg_tgt.empty()) return;
            h5pp::logger::log->trace("Creating space {}", msg_tgt);
        }
    }

    template<typename DimType = std::initializer_list<hsize_t>>
    inline void selectHyperslab(hid::h5s &       space,
                                const HyperSlab &hyperSlab,
                                H5S_seloper_t    selectOp = H5S_SELECT_OR) { // DimType slabOffset_, const DimType slabExtent_, DimType slabStride_ = {}, DimType slabBlock_ = {}) {
        if(hyperSlab.empty()) return;
        int ndims = H5Sget_simple_extent_ndims(space);
        if(ndims < 0) throw std::runtime_error("Failed to read space rank");
        auto                 rank = (size_t) ndims;
        std::vector<hsize_t> dims(rank);
        H5Sget_simple_extent_dims(space, dims.data(), nullptr);
        // If one of slabOffset or slabExtent is given, then the other must also be given
        if(hyperSlab.offset and not hyperSlab.extent) throw std::logic_error("Could not setup metadata for read operation: Given hyperslab offset but not extent");
        if(not hyperSlab.offset and hyperSlab.extent) throw std::logic_error("Could not setup metadata for read operation: Given hyperslab extent but not offset");

        // If given, ranks of slabOffset and slabExtent must be identical to each other and to the rank of the existing dataset
        if(hyperSlab.offset and hyperSlab.extent and (hyperSlab.offset.value().size() != hyperSlab.extent.value().size()))
            throw std::logic_error(h5pp::format(
                "Could not setup metadata for read operation: Size mismatch in given hyperslab arrays: offset {} | extent {}", hyperSlab.offset.value(), hyperSlab.extent.value()));

        if(hyperSlab.offset and hyperSlab.offset.value().size() != rank)
            throw std::logic_error(h5pp::format("Could not setup metadata for read operation for dataset [{}]: Hyperslab arrays have different rank compared to the given space: "
                                                "offset {} | extent {} | space dims {}",
                                                hyperSlab.offset.value(),
                                                hyperSlab.extent.value(),
                                                dims));

        // If given, slabStride must have the same rank as the dataset
        if(hyperSlab.stride and hyperSlab.stride.value().size() != rank)
            throw std::logic_error(h5pp::format("Could not setup metadata for read operation for dataset [{}]: Hyperslab stride has a different rank compared to the dataset: "
                                                "stride {} | dataset dims {}",
                                                hyperSlab.stride.value(),
                                                dims));
        // If given, slabBlock must have the same rank as the dataset
        if(hyperSlab.block and hyperSlab.block.value().size() != rank)
            throw std::logic_error(h5pp::format("Could not setup metadata for read operation for dataset [{}]: Hyperslab stride has a different rank compared to the dataset: "
                                                "block {} | dataset dims {}",
                                                hyperSlab.block.value(),
                                                dims));

        if(H5Sget_select_type(space) != H5S_SEL_HYPERSLABS) selectOp = H5S_SELECT_SET; // First operation must be H5S_SELECT_SET.

        /* clang-format off */
        if(hyperSlab.offset and hyperSlab.extent and hyperSlab.stride and hyperSlab.block)
            H5Sselect_hyperslab(space, selectOp, hyperSlab.offset.value().data(), hyperSlab.stride.value().data(), hyperSlab.extent.value().data(), hyperSlab.block.value().data());
        else if (hyperSlab.offset and hyperSlab.extent and hyperSlab.stride)
            H5Sselect_hyperslab(space, selectOp, hyperSlab.offset.value().data(), hyperSlab.stride.value().data(), hyperSlab.extent.value().data(), nullptr);
        else if (hyperSlab.offset and hyperSlab.extent and hyperSlab.block)
            H5Sselect_hyperslab(space, selectOp, hyperSlab.offset.value().data(), nullptr, hyperSlab.extent.value().data(), hyperSlab.block.value().data());
        else if (hyperSlab.offset and hyperSlab.extent)
            H5Sselect_hyperslab(space, selectOp, hyperSlab.offset.value().data(), nullptr, hyperSlab.extent.value().data(), nullptr);

        /* clang-format on */

        htri_t valid = H5Sselect_valid(space);
        if(valid < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            HyperSlab slab(space);
            throw std::runtime_error(h5pp::format("Hyperslab selection is invalid {}", slab.string()));
        }
    }

    inline void selectHyperSlabs(hid::h5s &space, const std::vector<HyperSlab> &hyperSlabs, std::optional<std::vector<H5S_seloper_t>> hyperSlabSelectOps = std::nullopt) {
        if(hyperSlabSelectOps and not hyperSlabSelectOps->empty()) {
            if(hyperSlabs.size() != hyperSlabSelectOps->size())
                for(const auto &slab : hyperSlabs) selectHyperslab(space, slab, hyperSlabSelectOps->at(0));
            else
                for(size_t num = 0; num < hyperSlabs.size(); num++) selectHyperslab(space, hyperSlabs[num], hyperSlabSelectOps->at(num));

        } else
            for(const auto &slab : hyperSlabs) selectHyperslab(space, slab);
    }

    namespace internal {
        inline long        maxSearchHits = -1;
        inline std::string searchKey;
        template<H5O_type_t ObjType>
        inline herr_t matcher([[maybe_unused]] hid_t id, const char *name, [[maybe_unused]] const H5O_info_t *oinfo, void *opdata) {
            try {
                if(oinfo->type == ObjType or ObjType == H5O_TYPE_UNKNOWN) {
                    auto matchList = reinterpret_cast<std::vector<std::string> *>(opdata);
                    if(searchKey.empty() or std::string_view(name).find(searchKey) != std::string::npos) {
                        matchList->push_back(name);
                        if(maxSearchHits > 0 and (long) matchList->size() >= maxSearchHits) return 1;
                    }
                }
                return 0;
            } catch(...) { throw std::logic_error(h5pp::format("Could not match object [{}] | loc_id [{}]", name, id)); }
        }
        template<H5O_type_t ObjType>
        inline herr_t collector([[maybe_unused]] hid_t loc_id, const char *name, [[maybe_unused]] const H5L_info_t *linfo, void *opdata) {
            try {
                auto linkNames = reinterpret_cast<std::vector<std::string> *>(opdata);
                linkNames->push_back(name);
                return 0;
            } catch(...) { throw std::logic_error(h5pp::format("Could not collect object [{}] | loc_id [{}]", name, loc_id)); }
        }
    }

    template<H5O_type_t ObjType>
    inline std::vector<std::string>
        findLinks(const hid::h5f &file, std::string_view searchKey = "", std::string_view searchRoot = "/", long maxSearchHits = -1, const hid::h5p &link_access = H5P_DEFAULT) {
        std::string linkType = "any";
        if constexpr(ObjType == H5O_TYPE_DATASET) linkType = "dataset";
        if constexpr(ObjType == H5O_TYPE_NAMED_DATATYPE) linkType = "named datatype";
        if constexpr(ObjType == H5O_TYPE_GROUP) linkType = "group";
        h5pp::logger::log->trace("Search key: {} | search root: {} | link type: {} | max search hits {}", searchKey, searchRoot, linkType, maxSearchHits);
        std::vector<std::string> matchList;
        internal::maxSearchHits = maxSearchHits;
        internal::searchKey     = searchKey;

#if defined(H5Ovisit_by_name3) || (defined(H5Ovisit_by_name_vers) && H5Ovisit_by_name_vers == 3)
        herr_t err = H5Ovisit_by_name3(file, util::safe_str(searchRoot).c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, internal::matcher<ObjType>, &matchList, H5O_INFO_ALL, link_access);
#elif defined(H5Ovisit_by_name2) || (defined(H5Ovisit_by_name_vers) && H5Ovisit_by_name_vers == 2)
        herr_t err = H5Ovisit_by_name2(file, util::safe_str(searchRoot).c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, internal::matcher<ObjType>, &matchList, H5O_INFO_ALL, link_access);
#elif defined(H5Ovisit_by_name1) || (defined(H5Ovisit_by_name_vers) && H5Ovisit_by_name_vers == 1)
        herr_t err = H5Ovisit_by_name1(file, util::safe_str(searchRoot).c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, internal::matcher<ObjType>, &matchList, link_access);
#else
        herr_t err = H5Ovisit_by_name(file, util::safe_str(searchRoot).c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, internal::matcher<ObjType>, &matchList, link_access);
#endif

        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to iterate from group [{}]", searchRoot));
        }
        return matchList;
    }

    template<H5O_type_t ObjType>
    inline std::vector<std::string> getContentsOfLink(const hid::h5f &file, const std::string &linkName, const hid::h5p &link_access = H5P_DEFAULT) {
        std::string linkType = "any";
        if constexpr(ObjType == H5O_TYPE_DATASET) linkType = "dataset";
        if constexpr(ObjType == H5O_TYPE_NAMED_DATATYPE) linkType = "named datatype";
        if constexpr(ObjType == H5O_TYPE_GROUP) linkType = "group";
        std::vector<std::string> contents;
        herr_t                   err = H5Literate_by_name(file, linkName.c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, nullptr, internal::collector<ObjType>, &contents, link_access);
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to iterate link [{}]", linkName));
        }
        return contents;
    }

    inline void createDataset(const h5pp::MetaDset &metaDset, const PropertyLists &plists = PropertyLists()) {
        // Here we create, the dataset id and set its properties before writing data to it.
        metaDset.assertCreateReady();
        if(metaDset.dsetExists.value()) return;
        h5pp::logger::log->debug("Creating dataset {}", metaDset.string());
        hid_t dsetID = H5Dcreate(metaDset.h5_file.value(),
                                 util::safe_str(metaDset.dsetPath.value()).c_str(),
                                 metaDset.h5_type.value(),
                                 metaDset.h5_space.value(),
                                 plists.link_create,
                                 metaDset.h5_plist_dset_create.value(),
                                 metaDset.h5_plist_dset_access.value());
        if(dsetID <= 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to create dataset {}", metaDset.string()));
        }
    }

    inline void createAttribute(const MetaAttr &metaAttr) {
        // Here we create, or register, the attribute id and set its properties before writing data to it.
        metaAttr.assertCreateReady();
        if(metaAttr.attrExists.value()) return;
        h5pp::logger::log->trace("Creating attribute {}", metaAttr.string());
        hid::h5a attrID = H5Acreate(metaAttr.h5_link.value(),
                                    util::safe_str(metaAttr.attrName.value()).c_str(),
                                    metaAttr.h5_type.value(),
                                    metaAttr.h5_space.value(),
                                    metaAttr.h5_plist_attr_create.value(),
                                    metaAttr.h5_plist_attr_access.value());
        if(attrID <= 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to create attribute [{}] for link [{}]", metaAttr.attrName.value(), metaAttr.linkPath.value()));
        }
    }

    template<typename DataType>
    std::vector<const char *> getCharPtrVector(const DataType &data) {
        std::vector<const char *> sv;
        if constexpr(h5pp::type::sfinae::is_text_v<DataType> and h5pp::type::sfinae::has_data_v<DataType>) // Takes care of std::string
            sv.push_back(data.data());
        else if constexpr(h5pp::type::sfinae::is_text_v<DataType>) // Takes care of char pointers and arrays
            sv.push_back(data);
        else if constexpr(h5pp::type::sfinae::is_iterable_v<DataType>) // Takes care of containers with text
            for(auto &elem : data) {
                if constexpr(h5pp::type::sfinae::is_text_v<decltype(elem)> and h5pp::type::sfinae::has_data_v<decltype(elem)>) // Takes care of containers with std::string
                    sv.push_back(elem.data());
                else if constexpr(h5pp::type::sfinae::is_text_v<decltype(elem)>) // Takes care of containers  of char pointers and arrays
                    sv.push_back(elem);
                else
                    sv.push_back(&elem); // Takes care of other things?
            }
        return sv;
    }

    template<typename DataType>
    void writeDataset(const DataType &data, const MetaData &metaData, const MetaDset &metaDset, const PropertyLists &plists = PropertyLists()) {
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_colmajor_v<DataType> and not h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            h5pp::logger::log->debug("Converting data to row-major storage order");
            const auto tempRowm = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::writeDataset(tempRowm, metaData, metaDset, plists);
            return;
        }
#endif
        metaDset.assertWriteReady();
        metaData.assertWriteReady();
        h5pp::logger::log->debug("Writing from memory  {}", metaData.string());
        h5pp::logger::log->debug("Writing into dataset {}", metaDset.string());

        // TODO: REMEMBER TO EXTEND CHUNKED DATASETS TO THE NEW SIZE

        h5pp::hdf5::assertBytesPerElemMatch<DataType>(metaDset.h5_type.value());
        h5pp::hdf5::assertBytesMatchTotal<DataType>(data, metaData.dataDims.value(), metaDset.h5_dset.value());
        h5pp::hdf5::assertSpacesEqual(metaData.h5_space.value(), metaDset.h5_space.value(), metaDset.h5_type.value());
        h5pp::hdf5::logSpaceTranfer(metaData.h5_space.value(), metaDset.h5_space.value());
        herr_t retval = 0;
        if constexpr(h5pp::type::sfinae::is_text_v<DataType> or h5pp::type::sfinae::has_text_v<DataType>) {
            auto vec = getCharPtrVector(data);
            // When H5T_VARIABLE, this function expects [const char **], which is what we get from vec.data()
            if(H5Tis_variable_str(metaDset.h5_type->value()) > 0)
                retval = H5Dwrite(metaDset.h5_dset.value(), metaDset.h5_type.value(), metaData.h5_space.value(), metaDset.h5_space.value(), plists.dset_xfer, vec.data());
            else
                retval = H5Dwrite(metaDset.h5_dset.value(), metaDset.h5_type.value(), metaData.h5_space.value(), metaDset.h5_space.value(), plists.dset_xfer, *vec.data());
        } else if constexpr(h5pp::type::sfinae::has_data_v<DataType>)
            retval = H5Dwrite(metaDset.h5_dset.value(), metaDset.h5_type.value(), metaData.h5_space.value(), metaDset.h5_space.value(), plists.dset_xfer, data.data());
        else if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>)
            retval = H5Dwrite(metaDset.h5_dset.value(), metaDset.h5_type.value(), metaData.h5_space.value(), metaDset.h5_space.value(), plists.dset_xfer, data);
        else
            retval = H5Dwrite(metaDset.h5_dset.value(), metaDset.h5_type.value(), metaData.h5_space.value(), metaDset.h5_space.value(), plists.dset_xfer, &data);

        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to write into dataset \n\t {} \n from memory \n\t {}", metaDset.string(), metaData.string()));
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    void readDataset(DataType &data, const MetaData &metaData, const MetaDset &metaDset, const PropertyLists &plists = PropertyLists()) {
// Transpose the data container before reading
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_colmajor_v<DataType> and not h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            h5pp::logger::log->debug("Converting data to row-major storage order");
            auto tempRowMajor = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::readDataset(tempRowMajor, metaData, metaDset, plists);
            data = eigen::to_ColMajor(tempRowMajor);
            return;
        }
#endif

        metaDset.assertReadReady();
        metaData.assertReadReady();
        h5pp::logger::log->debug("Reading into memory  {}", metaData.string());
        h5pp::logger::log->debug("Reading from dataset {}", metaDset.string());

        h5pp::hdf5::assertSpacesEqual(metaData.h5_space.value(), metaDset.h5_space.value(), metaDset.h5_type.value());
        h5pp::hdf5::assertBytesPerElemMatch<DataType>(metaDset.h5_type.value());
        h5pp::hdf5::assertBytesMatchTotal(data, metaData.dataDims.value(), metaDset.h5_dset.value());
        h5pp::hdf5::logSpaceTranfer(metaDset.h5_space.value(), metaData.h5_space.value());
        herr_t retval = 0;

        // Read the data
        if constexpr(h5pp::type::sfinae::is_text_v<DataType> or h5pp::type::sfinae::has_text_v<DataType>) {
            // When H5T_VARIABLE,
            //      1) H5Dread expects [const char **], which is what we get from vdata.data().
            //      2) H5Dread allocates memory on each const char * which has to be reclaimed later.
            // Otherwise,
            //      1) H5Dread expects [char *], i.e. *vdata.data()
            //      2) Allocation on char * must be done before reading.
            //
            if(H5Tis_variable_str(metaDset.h5_type.value())) {
                auto                size = H5Sget_select_npoints(metaDset.h5_space.value());
                std::vector<char *> vdata(static_cast<size_t>(size)); // Allocate pointers for "size" number of strings
                retval = H5Dread(metaDset.h5_dset.value(), metaDset.h5_type.value(), H5S_ALL, metaDset.h5_space.value(), plists.dset_xfer, vdata.data());
                // HDF5 allocates space for each string
                // Now vdata contains the whole dataset and we need to put the data into the user-given container.

                if constexpr(std::is_same_v<DataType, std::string>) {
                    // A vector of strings (vdata) can be put into a single string (data) with entries separated by new-lines
                    data.clear();
                    for(size_t i = 0; i < vdata.size(); i++) {
                        if(vdata[i] != nullptr) data.append(vdata[i]);
                        ;
                        if(i < vdata.size() - 1) data.append("\n");
                    }
                } else if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::string> and h5pp::type::sfinae::has_resize_v<DataType>) {
                    data.clear();
                    data.resize(vdata.size());
                    for(size_t i = 0; i < data.size(); i++) data[i] = std::string(vdata[i]);
                } else {
                    throw std::runtime_error("To read text-data, please use std::string or a container of std::string like std::vector<std::string>");
                }
                // Free memory allocated by HDF5
                H5Dvlen_reclaim(metaDset.h5_type->value(), metaDset.h5_space.value(), plists.dset_xfer, vdata.data());
            } else {
                // We expect the given data container to be properly sized
                if constexpr(h5pp::type::sfinae::has_data_v<DataType>) {
                    retval = H5Dread(metaDset.h5_dset.value(), metaDset.h5_type.value(), metaData.h5_space.value(), metaDset.h5_space.value(), plists.dset_xfer, data.data());
                } else if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>)
                    retval = H5Dread(metaDset.h5_dset.value(), metaDset.h5_type.value(), metaData.h5_space.value(), metaDset.h5_space.value(), plists.dset_xfer, data);
            }

        }

        else if constexpr(h5pp::type::sfinae::has_data_v<DataType>)
            retval = H5Dread(metaDset.h5_dset.value(), metaDset.h5_type.value(), metaData.h5_space.value(), metaDset.h5_space.value(), plists.dset_xfer, data.data());
        else if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>)
            retval = H5Dread(metaDset.h5_dset.value(), metaDset.h5_type.value(), metaData.h5_space.value(), metaDset.h5_space.value(), plists.dset_xfer, data);
        else
            retval = H5Dread(metaDset.h5_dset.value(), metaDset.h5_type.value(), metaData.h5_space.value(), metaDset.h5_space.value(), plists.dset_xfer, &data);

        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to read from dataset \n\t {} \n into memory \n\t {}", metaDset.string(), metaData.string()));
        }
    }

    template<typename DataType>
    void writeAttribute(const DataType &data, const MetaData &metaData, const MetaAttr &metaAttr) {
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_colmajor_v<DataType> and not h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            h5pp::logger::log->debug("Converting attribute data to row-major storage order");
            const auto tempRowm = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::writeAttribute(tempRowm, metaData, metaAttr);
            return;
        }
#endif
        metaData.assertWriteReady();
        metaAttr.assertWriteReady();

        h5pp::logger::log->debug("Writing from memory    {}", metaData.string());
        h5pp::logger::log->debug("Writing into attribute {}", metaAttr.string());
        h5pp::hdf5::assertBytesPerElemMatch<DataType>(metaAttr.h5_type.value());
        h5pp::hdf5::assertBytesMatchTotal(data, metaData.dataDims.value(), metaAttr.h5_attr.value());
        h5pp::hdf5::assertSpacesEqual(metaData.h5_space.value(), metaAttr.h5_space.value(), metaAttr.h5_type.value());
        h5pp::hdf5::logSpaceTranfer(metaData.h5_space.value(), metaAttr.h5_space.value());

        herr_t retval = 0;
        if constexpr(h5pp::type::sfinae::is_text_v<DataType> or h5pp::type::sfinae::has_text_v<DataType>) {
            auto vec = getCharPtrVector(data);
            retval   = H5Awrite(metaAttr.h5_attr.value(), metaAttr.h5_type.value(), vec.data());
        } else if constexpr(h5pp::type::sfinae::has_data_v<DataType>)
            retval = H5Awrite(metaAttr.h5_attr.value(), metaAttr.h5_type.value(), data.data());
        else if constexpr(std::is_pointer_v<DataType>)
            retval = H5Awrite(metaAttr.h5_attr.value(), metaAttr.h5_type.value(), data);
        else
            retval = H5Awrite(metaAttr.h5_attr.value(), metaAttr.h5_type.value(), &data);

        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to write into attribute \n\t {} \n from memory \n\t {}", metaAttr.string(), metaData.string()));
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    void readAttribute(DataType &data, const MetaData &metaData, const MetaAttr &metaAttr) {
        // Transpose the data container before reading
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_colmajor_v<DataType> and not h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            h5pp::logger::log->debug("Converting data to row-major storage order");
            auto tempRowMajor = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::readAttribute(tempRowMajor, metaData, metaAttr);
            data = eigen::to_ColMajor(tempRowMajor);
            return;
        }
#endif
        metaData.assertReadReady();
        metaAttr.assertReadReady();
        h5pp::logger::log->debug("Reading into memory {}", metaData.string());
        h5pp::logger::log->debug("Reading from file   {}", metaAttr.string());
        h5pp::hdf5::assertBytesPerElemMatch<DataType>(metaAttr.h5_type.value());
        h5pp::hdf5::assertBytesMatchTotal(data, metaData.dataDims.value(), metaAttr.h5_attr.value());
        h5pp::hdf5::logSpaceTranfer(metaAttr.h5_space.value(), metaData.h5_space.value());
        h5pp::hdf5::assertSpacesEqual(metaData.h5_space.value(), metaAttr.h5_space.value(), metaAttr.h5_type.value());
        herr_t retval = 0;

        // Read the data
        if constexpr(h5pp::type::sfinae::is_text_v<DataType> or h5pp::type::sfinae::has_text_v<DataType>) {
            //            if (H5Tis_variable_str(metaAttr.h5_type.value()) > 0) {
            std::vector<const char *> vdata{metaAttr.attrSize.value()}; // Allocate for pointers for "size" number of strings
            // HDF5 allocates space for each string
            retval = H5Aread(metaAttr.h5_attr.value(), metaAttr.h5_type.value(), vdata.data());
            // Now sdata contains the whole dataset and we need to put the data into the user-given container.
            if constexpr(std::is_same_v<DataType, std::string>) {
                data.clear();
                for(size_t i = 0; i < vdata.size(); i++) {
                    data.append(vdata[i]);
                    if(i < vdata.size() - 1) data.append("\n");
                }
            } else if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::string> and h5pp::type::sfinae::has_resize_v<DataType>) {
                data.clear();
                data.resize(vdata.size());
                for(size_t i = 0; i < data.size(); i++) data[i] = std::string(vdata[i]);
            } else {
                throw std::runtime_error("To read text-data, please use std::string or a container of std::string like std::vector<std::string>");
            }
            // Free memory allocated by HDF5
            H5Dvlen_reclaim(metaAttr.h5_type.value(), metaAttr.h5_space.value(), H5P_DEFAULT, vdata.data());
            //            }
        }

        else if constexpr(h5pp::type::sfinae::has_data_v<DataType>)
            retval = H5Aread(metaAttr.h5_attr.value(), metaAttr.h5_type.value(), data.data());
        else if constexpr(std::is_pointer_v<DataType>)
            retval = H5Aread(metaAttr.h5_attr.value(), metaAttr.h5_type.value(), data);
        else
            retval = H5Aread(metaAttr.h5_attr.value(), metaAttr.h5_type.value(), &data);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to read from attribute \n\t {} \n into memory \n\t {}", metaAttr.string(), metaData.string()));
        }
    }

    inline bool fileIsValid(const fs::path &filePath) { return fs::exists(filePath) and H5Fis_hdf5(filePath.string().c_str()) > 0; }

    [[nodiscard]] inline fs::path getAvailableFileName(const fs::path &filePath) {
        int      i           = 1;
        fs::path newFileName = filePath;
        while(fs::exists(newFileName)) { newFileName.replace_filename(filePath.stem().string() + "-" + std::to_string(i++) + filePath.extension().string()); }
        return newFileName;
    }

    [[nodiscard]] inline fs::path getBackupFileName(const fs::path &filePath) {
        int      i           = 1;
        fs::path newFilePath = filePath;
        while(fs::exists(newFilePath)) { newFilePath.replace_extension(filePath.extension().string() + ".bak_" + std::to_string(i++)); }
        return newFilePath;
    }

    inline h5pp::FilePermission convertFileAccessFlags(unsigned int H5F_ACC_FLAGS) {
        h5pp::FilePermission permission = h5pp::FilePermission::RENAME;
        if((H5F_ACC_FLAGS & (H5F_ACC_TRUNC | H5F_ACC_EXCL)) == (H5F_ACC_TRUNC | H5F_ACC_EXCL))
            throw std::runtime_error("File access modes H5F_ACC_EXCL and H5F_ACC_TRUNC are mutually exclusive");
        if((H5F_ACC_FLAGS & H5F_ACC_RDONLY) == H5F_ACC_RDONLY) permission = h5pp::FilePermission::READONLY;
        if((H5F_ACC_FLAGS & H5F_ACC_RDWR) == H5F_ACC_RDWR) permission = h5pp::FilePermission::READWRITE;
        if((H5F_ACC_FLAGS & H5F_ACC_EXCL) == H5F_ACC_EXCL) permission = h5pp::FilePermission::COLLISION_FAIL;
        if((H5F_ACC_FLAGS & H5F_ACC_TRUNC) == H5F_ACC_TRUNC) permission = h5pp::FilePermission::REPLACE;
        return permission;
    }

    inline unsigned int convertFileAccessFlags(h5pp::FilePermission permission) {
        unsigned int H5F_ACC_MODE = H5F_ACC_RDONLY;
        if(permission == h5pp::FilePermission::COLLISION_FAIL) H5F_ACC_MODE |= H5F_ACC_EXCL;
        if(permission == h5pp::FilePermission::REPLACE) H5F_ACC_MODE |= H5F_ACC_TRUNC;
        if(permission == h5pp::FilePermission::RENAME) H5F_ACC_MODE |= H5F_ACC_TRUNC;
        if(permission == h5pp::FilePermission::READONLY) H5F_ACC_MODE |= H5F_ACC_RDONLY;
        if(permission == h5pp::FilePermission::READWRITE) H5F_ACC_MODE |= H5F_ACC_RDWR;
        return H5F_ACC_MODE;
    }

    inline fs::path createFile(const h5pp::fs::path &filePath_, const h5pp::FilePermission &permission, const PropertyLists &plists = PropertyLists()) {
        fs::path filePath = fs::absolute(filePath_);
        fs::path fileName = filePath_.filename();
        if(fs::exists(filePath)) {
            if(not fileIsValid(filePath)) h5pp::logger::log->debug("Pre-existing file may be corrupted [{}]", filePath.string());
            if(permission == h5pp::FilePermission::READONLY) return filePath;
            if(permission == h5pp::FilePermission::COLLISION_FAIL)
                throw std::runtime_error(h5pp::format("[COLLISION_FAIL]: Previous file exists with the same name [{}]", filePath.string()));
            if(permission == h5pp::FilePermission::RENAME) {
                auto newFilePath = getAvailableFileName(filePath);
                h5pp::logger::log->info("[RENAME]: Previous file exists. Choosing a new file name [{}] --> [{}]", filePath.filename().string(), newFilePath.filename().string());
                filePath = newFilePath;
                fileName = filePath.filename();
            }
            if(permission == h5pp::FilePermission::READWRITE) return filePath;
            if(permission == h5pp::FilePermission::BACKUP) {
                auto backupPath = getBackupFileName(filePath);
                h5pp::logger::log->info("[BACKUP]: Backing up existing file [{}] --> [{}]", filePath.filename().string(), backupPath.filename().string());
                fs::rename(filePath, backupPath);
            }
            if(permission == h5pp::FilePermission::REPLACE) {} // Do nothing
        } else {
            if(permission == h5pp::FilePermission::READONLY) throw std::runtime_error(h5pp::format("[READONLY]: File does not exist [{}]", filePath.string()));
            if(permission == h5pp::FilePermission::COLLISION_FAIL) {} // Do nothing
            if(permission == h5pp::FilePermission::RENAME) {}         // Do nothing
            if(permission == h5pp::FilePermission::READWRITE) {}      // Do nothing;
            if(permission == h5pp::FilePermission::BACKUP) {}         // Do nothing
            if(permission == h5pp::FilePermission::REPLACE) {}        // Do nothing
            try {
                if(fs::create_directories(filePath.parent_path()))
                    h5pp::logger::log->trace("Created directory: {}", filePath.parent_path().string());
                else
                    h5pp::logger::log->trace("Directory already exists: {}", filePath.parent_path().string());
            } catch(std::exception &ex) { throw std::runtime_error(h5pp::format("Failed to create directory: {}", ex.what())); }
        }

        // One last sanity check
        if(permission == h5pp::FilePermission::READONLY) throw std::logic_error("About to create/truncate a file even though READONLY was specified. This is a programming error!");

        // Go ahead
        hid_t file = H5Fcreate(filePath.string().c_str(), H5F_ACC_TRUNC, plists.file_create, plists.file_access);
        if(file < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to create file [{}]", filePath.string()));
        }
        H5Fclose(file);
        return fs::canonical(filePath);
    }

    inline void createTable(const hid::h5f &file, const TableProperties &tableProps, const PropertyLists &plists = PropertyLists()) {
        h5pp::logger::log->debug(
            "Creating table [{}] | num fields {} | record size {} bytes", tableProps.tableName.value(), tableProps.NFIELDS.value(), tableProps.entrySize.value());
        createGroup(file, tableProps.groupName.value(), std::nullopt, plists);

        if(checkIfLinkExists(file, tableProps.tableName.value(), std::nullopt, plists.link_access)) {
            h5pp::logger::log->debug("Table [{}] already exists", tableProps.tableName.value());
            return;
        }

        // Copy member type data to a vector of hid_t for compatibility
        std::vector<hid_t> fieldTypesHidT(tableProps.fieldTypes.value().begin(), tableProps.fieldTypes.value().end());

        // Copy member name data to a vector of const char * for compatibility
        std::vector<const char *> fieldNames;
        for(auto &name : tableProps.fieldNames.value()) fieldNames.push_back(name.c_str());

        H5TBmake_table(util::safe_str(tableProps.tableTitle.value()).c_str(),
                       file,
                       util::safe_str(tableProps.tableName.value()).c_str(),
                       tableProps.NFIELDS.value(),
                       tableProps.NRECORDS.value(),
                       tableProps.entrySize.value(),
                       fieldNames.data(),
                       tableProps.fieldOffsets.value().data(),
                       fieldTypesHidT.data(),
                       tableProps.chunkSize.value(),
                       nullptr,
                       (int) tableProps.compressionLevel.value(),
                       nullptr);
        h5pp::logger::log->trace("Successfully created table [{}]", tableProps.tableName.value());
        //        h5pp::logger::log->trace("Committing new named table type [{}]", tableProps.tableTitle.value());
        //        auto table = openLink<hid::h5d>(file,util::safe_str(tableProps.tableName.value()).c_str(),true,plists.link_access);
        //        H5Tcommit(table, util::safe_str(tableProps.tableTitle.value()).c_str(),tableProps.entryType, H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
    }

    template<typename DataType>
    inline void appendTableEntries(const hid::h5f &file, const DataType &data, const TableProperties &tableProps) {
        h5pp::logger::log->debug(
            "Appending records to table [{}] | num records {} | record size {} bytes", tableProps.tableName.value(), tableProps.NRECORDS.value(), tableProps.entrySize.value());

        // Make sure the given container and the registerd table entry have the same size.
        // If there is a mismatch here it can cause horrible bugs/segfaults
        if constexpr(h5pp::type::sfinae::has_value_type_v<DataType>) {
            if(sizeof(typename DataType::value_type) != tableProps.entrySize.value())
                throw std::runtime_error(h5pp::format("Size mismatch: Given container of type {} has elements of {} bytes, but the table entries on file are {} bytes each ",
                                                      h5pp::type::sfinae::type_name<DataType>(),
                                                      sizeof(typename DataType::value_type),
                                                      tableProps.entrySize.value()));
        } else if constexpr(h5pp::type::sfinae::has_data_v<DataType> and h5pp::type::sfinae::is_iterable_v<DataType>) {
            if(sizeof(&data.data()) != tableProps.entrySize.value())
                throw std::runtime_error(h5pp::format("Size mismatch: Given container of type {} has elements of {} bytes, but the table entries on file are {} bytes each ",
                                                      h5pp::type::sfinae::type_name<DataType>(),
                                                      sizeof(&data.data()),
                                                      tableProps.entrySize.value()));
        } else {
            if(sizeof(DataType) != tableProps.entrySize.value())
                throw std::runtime_error(h5pp::format("Size mismatch: Given data type {} is of {} bytes, but the table entries on file are {} bytes each ",
                                                      h5pp::type::sfinae::type_name<DataType>(),
                                                      sizeof(DataType),
                                                      tableProps.entrySize.value()));
        }

        if constexpr(h5pp::type::sfinae::has_data_v<DataType>) {
            H5TBappend_records(file,
                               util::safe_str(tableProps.tableName.value()).c_str(),
                               tableProps.NRECORDS.value(),
                               tableProps.entrySize.value(),
                               tableProps.fieldOffsets.value().data(),
                               tableProps.fieldSizes.value().data(),
                               data.data());
        } else {
            H5TBappend_records(file,
                               util::safe_str(tableProps.tableName.value()).c_str(),
                               tableProps.NRECORDS.value(),
                               tableProps.entrySize.value(),
                               tableProps.fieldOffsets.value().data(),
                               tableProps.fieldSizes.value().data(),
                               &data);
        }
    }

    template<typename h5x_src,
             typename h5x_tgt,
             typename = std::enable_if_t<std::is_same_v<h5x_src, hid::h5f> or std::is_same_v<h5x_src, hid::h5g>>,
             typename = std::enable_if_t<std::is_same_v<h5x_tgt, hid::h5f> or std::is_same_v<h5x_tgt, hid::h5g>>>
    inline void appendTableEntries(const h5x_src &      srcLocation,
                                   std::string_view     srcTableName,
                                   const h5x_tgt &      tgtLocation,
                                   std::string_view     tgtTableName,
                                   size_t startEntry,
                                   size_t numEntries,
                                   const PropertyLists &plists = PropertyLists()) {
        auto srcInfo = getTableTypeInfo(srcLocation, srcTableName, plists.link_access);
        auto tgtInfo = getTableTypeInfo(tgtLocation, tgtTableName, plists.link_access);
        if(srcInfo.h5_table_type != tgtInfo.h5_table_type) throw std::runtime_error("Table type mismatch");
        if(srcInfo.recordBytes != tgtInfo.recordBytes) throw std::runtime_error("Table size mismatch");



        std::vector<std::byte> data(numEntries * tgtInfo.recordBytes.value());
        H5TBread_records(srcLocation,
                         util::safe_str(srcTableName).c_str(),
                         startEntry,
                         numEntries,
                         srcInfo.recordBytes.value(),
                         srcInfo.fieldOffsets.value().data(),
                         srcInfo.fieldSizes.value().data(),
                         data.data());

        H5TBappend_records(tgtLocation,
                           util::safe_str(tgtTableName).c_str(),
                           numEntries,
                           tgtInfo.recordBytes.value(),
                           tgtInfo.fieldOffsets.value().data(),
                           srcInfo.fieldSizes.value().data(),
                           data.data());
    }

    template<typename DataType>
    inline void readTableEntries(const hid::h5f &       file,
                                 DataType &             data,
                                 const TableProperties &tableProps,
                                 std::optional<size_t>  startEntry = std::nullopt,
                                 std::optional<size_t>  numEntries = std::nullopt) {
        // If none of startEntry or numEntries are given:
        //          If data resizeable: startEntry = 0, numEntries = totalRecords
        //          If data not resizeable: startEntry = last entry, numEntries = 1.
        // If startEntry given but numEntries is not:
        //          If data resizeable -> read from startEntries to the end
        //          If data not resizeable -> read from startEntries a single entry
        // If numEntries given but startEntries is not -> read the last numEntries records

        hsize_t totalRecords = tableProps.NRECORDS.value();
        if(not startEntry and not numEntries) {
            if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) {
                startEntry = 0;
                numEntries = totalRecords;

            } else {
                startEntry = totalRecords - 1;
                numEntries = 1;
            }
        } else if(startEntry and not numEntries) {
            if(startEntry.value() > totalRecords - 1)
                throw std::runtime_error(
                    h5pp::format("Invalid start record for table [{}] | nrecords [{}] | start entry [{}]", tableProps.tableName.value(), totalRecords, startEntry.value()));
            if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) {
                numEntries = totalRecords - startEntry.value();
            } else {
                numEntries = 1;
            }

        } else if(numEntries and not startEntry) {
            if(numEntries and numEntries.value() > totalRecords)
                throw std::runtime_error(h5pp::format(
                    "Asked for more records than available in table [{}] | nrecords [{}] | num_entries [{}]", tableProps.tableName.value(), totalRecords, numEntries.value()));
            startEntry = totalRecords - numEntries.value();
        }

        // Sanity check
        if(numEntries.value() > totalRecords)
            throw std::logic_error(h5pp::format("Asked for more records than available in table [{}] | nrecords [{}]", tableProps.tableName.value(), totalRecords));
        if(startEntry.value() + numEntries.value() > totalRecords)
            throw std::logic_error(h5pp::format("Asked for more records than available in table [{}] | nrecords [{}]", tableProps.tableName.value(), totalRecords));

        h5pp::logger::log->debug("Reading table [{}] | read from record {} | read num records {} | available records {} | record size {} bytes",
                                 tableProps.tableName.value(),
                                 startEntry.value(),
                                 numEntries.value(),
                                 tableProps.NRECORDS.value(),
                                 tableProps.entrySize.value());

        // Make sure the given container and the registerd table entry have the same size.
        // If there is a mismatch here it can cause horrible bugs/segfaults
        if constexpr(h5pp::type::sfinae::has_value_type_v<DataType>) {
            if(sizeof(typename DataType::value_type) != tableProps.entrySize.value())
                throw std::runtime_error(h5pp::format("Size mismatch: Given container of type {} has elements of {} bytes, but the table records on file are {} bytes each ",
                                                      h5pp::type::sfinae::type_name<DataType>(),
                                                      sizeof(typename DataType::value_type),
                                                      tableProps.entrySize.value()));
        } else if constexpr(h5pp::type::sfinae::has_data_v<DataType> and h5pp::type::sfinae::is_iterable_v<DataType>) {
            if(sizeof(&data.data()) != tableProps.entrySize.value())
                throw std::runtime_error(h5pp::format("Size mismatch: Given container of type {} has elements of {} bytes, but the table records on file are {} bytes each ",
                                                      h5pp::type::sfinae::type_name<DataType>(),
                                                      sizeof(&data.data()),
                                                      tableProps.entrySize.value()));
        } else {
            if(sizeof(DataType) != tableProps.entrySize.value())
                throw std::runtime_error(h5pp::format("Size mismatch: Given data type {} is of {} bytes, but the table records on file are {} bytes each ",
                                                      h5pp::type::sfinae::type_name<DataType>(),
                                                      sizeof(DataType),
                                                      tableProps.entrySize.value()));
        }

        if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) { data.resize(numEntries.value()); }

        if constexpr(h5pp::type::sfinae::has_data_v<DataType>) {
            H5TBread_records(file,
                             util::safe_str(tableProps.tableName.value()).c_str(),
                             startEntry.value(),
                             numEntries.value(),
                             tableProps.entrySize.value(),
                             tableProps.fieldOffsets.value().data(),
                             tableProps.fieldSizes.value().data(),
                             data.data());
        } else {
            H5TBread_records(file,
                             util::safe_str(tableProps.tableName.value()).c_str(),
                             startEntry.value(),
                             numEntries.value(),
                             tableProps.entrySize.value(),
                             tableProps.fieldOffsets.value().data(),
                             tableProps.fieldSizes.value().data(),
                             &data);
        }
    }

    inline fs::path
        copyFile(const std::string &src, const std::string &tgt, FilePermission permission = FilePermission::COLLISION_FAIL, const PropertyLists &plists = PropertyLists()) {
        h5pp::logger::log->trace("Copying file [{}] --> [{}]", src, tgt);
        auto tgtPath = h5pp::hdf5::createFile(tgt, permission, plists);
        auto srcPath = fs::absolute(src);
        if(not fs::exists(srcPath)) throw std::runtime_error(h5pp::format("Could not copy file: source path does not exist [{}]", srcPath.string()));

        if(tgtPath == srcPath) throw std::runtime_error(h5pp::format("Could not copy file: source and target paths are the same [{}]", srcPath.string()));

        hid::h5f tgtFile = H5Fopen(tgtPath.string().c_str(), H5F_ACC_RDWR, plists.file_access);
        hid::h5f srcFile = H5Fopen(srcPath.string().c_str(), H5F_ACC_RDONLY, plists.file_access);

        if(tgtFile < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Could not copy file: failed to open target file in read-write mode [{}]", tgtPath.string()));
        }

        if(srcFile < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Could not copy file: failed to open source file in read-only mode [{}]", srcPath.string()));
        }

        // Copy all the groups in the file root recursively
        for(const auto &link : getContentsOfLink<H5O_TYPE_UNKNOWN>(srcFile, "/", plists.link_access)) {
            auto retval = H5Ocopy(srcFile, link.c_str(), tgtFile, link.c_str(), H5P_DEFAULT, H5P_DEFAULT);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(
                    h5pp::format("Could not copy file: failed to copy contents of source file [{}] into target file [{}] ", srcPath.string(), tgtPath.string()));
            }
        }
        // ... Find out how to copy attributes that are written on the root itself
        return tgtPath;
    }

    inline fs::path
        moveFile(const std::string &src, const std::string &tgt, FilePermission permission = FilePermission::COLLISION_FAIL, const PropertyLists &plists = PropertyLists()) {
        h5pp::logger::log->trace("Moving file by copy+remove: [{}] --> [{}]", src, tgt);
        auto tgtPath = copyFile(src, tgt, permission, plists); // Returns the path to the newly created file
        auto srcPath = fs::absolute(src);
        if(fs::exists(tgtPath)) {
            h5pp::logger::log->trace("Removing file [{}]", srcPath.string());
            try {
                fs::remove(srcPath);
            } catch(const std::exception &err) { throw std::runtime_error(h5pp::format("Remove failed. File may be locked [{}] | what(): {} ", srcPath.string(), err.what())); }
            return tgtPath;
        } else
            throw std::runtime_error(h5pp::format("Could not copy file [{}] to target [{}]", srcPath.string(), tgt));

        return tgtPath;
    }

}
