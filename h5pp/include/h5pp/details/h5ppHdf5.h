#pragma once
#include "h5ppEigen.h"
#include "h5ppEnums.h"
#include "h5ppFilesystem.h"
#include "h5ppHyperslab.h"
#include "h5ppInfo.h"
#include "h5ppLogger.h"
#include "h5ppPropertyLists.h"
#include "h5ppTypeSfinae.h"
#include "h5ppUtils.h"
#include <hdf5.h>
#include <map>
#include <typeindex>
#include <utility>
/*!
 * \brief A collection of functions to create (or get information about) datasets and attributes in HDF5 files
 */
namespace h5pp::hdf5 {

    [[nodiscard]] inline std::vector<std::string_view> pathCumulativeSplit(std::string_view strv, std::string_view delim) {
        std::vector<std::string_view> output;
        size_t                        currentPosition = 0;
        while(currentPosition < strv.size()) {
            const auto foundPosition = strv.find_first_of(delim, currentPosition);
            if(currentPosition != foundPosition) { output.emplace_back(strv.substr(0, foundPosition)); }
            if(foundPosition == std::string_view::npos) break;
            currentPosition = foundPosition + 1;
        }
        return output;
    }

    template<typename h5x, typename = std::enable_if_t<std::is_base_of_v<hid::hid_base<h5x>, h5x>>>
    [[nodiscard]] std::string getName(const h5x &object) {
        std::string buf;
        ssize_t     namesize = H5Iget_name(object, nullptr, 0);
        if(namesize > 0) {
            buf.resize(static_cast<size_t>(namesize) + 1);
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

    [[nodiscard]] inline hsize_t getSize(const hid::h5s &space) { return static_cast<hsize_t>(H5Sget_simple_extent_npoints(space)); }

    [[nodiscard]] inline hsize_t getSize(const hid::h5d &dataset) {
        hid::h5s space = H5Dget_space(dataset);
        return getSize(space);
    }

    [[nodiscard]] inline hsize_t getSize(const hid::h5a &attribute) {
        hid::h5s space = H5Aget_space(attribute);
        return getSize(space);
    }

    [[nodiscard]] inline hsize_t getSizeSelected(const hid::h5s &space) { return static_cast<hsize_t>(H5Sget_select_npoints(space)); }

    [[nodiscard]] inline std::vector<hsize_t> getDimensions(const hid::h5s &space) {
        int ndims = H5Sget_simple_extent_ndims(space);
        if(ndims < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to get dimensions");
        }
        std::vector<hsize_t> dims(static_cast<size_t>(ndims));
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

    [[nodiscard]] inline H5D_layout_t getLayout(const hid::h5p &dataset_creation_property_list) { return H5Pget_layout(dataset_creation_property_list); }

    [[nodiscard]] inline H5D_layout_t getLayout(const hid::h5d &dataset) {
        hid::h5p dcpl = H5Dget_create_plist(dataset);
        return H5Pget_layout(dcpl);
    }

    [[nodiscard]] inline std::optional<std::vector<hsize_t>> getChunkDimensions(const hid::h5p &dsetCreatePropertyList) {
        auto layout = H5Pget_layout(dsetCreatePropertyList);
        if(layout != H5D_CHUNKED) return std::nullopt;
        auto ndims = H5Pget_chunk(dsetCreatePropertyList, 0, nullptr);
        if(ndims < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to get chunk dimensions");
        } else if(ndims > 0) {
            std::vector<hsize_t> chunkDims(static_cast<size_t>(ndims));
            H5Pget_chunk(dsetCreatePropertyList, ndims, chunkDims.data());
            return chunkDims;
        } else
            return {};
    }

    [[nodiscard]] inline std::optional<std::vector<hsize_t>> getChunkDimensions(const hid::h5d &dataset) {
        hid::h5p dcpl = H5Dget_create_plist(dataset);
        return getChunkDimensions(dcpl);
    }

    [[nodiscard]] inline std::optional<std::vector<hsize_t>> getMaxDimensions(const hid::h5s &space, H5D_layout_t layout) {
        if(layout != H5D_CHUNKED) return std::nullopt;
        if(H5Sget_simple_extent_type(space) != H5S_SIMPLE) return std::nullopt;
        int rank = H5Sget_simple_extent_ndims(space);
        if(rank < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to get dimensions");
        }
        std::vector<hsize_t> maxdims(static_cast<size_t>(rank));
        H5Sget_simple_extent_dims(space, nullptr, maxdims.data());
        return maxdims;
    }

    [[nodiscard]] inline std::optional<std::vector<hsize_t>> getMaxDimensions(const hid::h5d &dataset) {
        hid::h5s space = H5Dget_space(dataset);
        hid::h5p dcpl  = H5Dget_create_plist(dataset);
        return getMaxDimensions(space, getLayout(dcpl));
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
        std::vector<const char *> vdata{static_cast<size_t>(size)}; // Allocate for pointers for "size" number of strings
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
            *vlen += static_cast<hsize_t>(strnlen(elem, max_len)) + 1; // Add null-terminator
        }
        H5Dvlen_reclaim(type, space, H5P_DEFAULT, vdata.data());
        return 1;
    }

    inline herr_t H5Avlen_get_buf_size_safe(const hid::h5a &attr, const hid::h5t &type, const hid::h5s &space, hsize_t *vlen) {
        *vlen = 0;
        if(H5Tis_variable_str(type) <= 0) return -1;
        if(H5Aget_storage_size(attr) <= 0) return 0;

        auto                      size = H5Sget_simple_extent_npoints(space);
        std::vector<const char *> vdata{static_cast<size_t>(size)}; // Allocate pointers for "size" number of strings
        // HDF5 allocates space for each string
        herr_t retval = H5Aread(attr, type, vdata.data());
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            return 0;
        }
        // Sum up the number of bytes
        size_t maxLen = h5pp::constants::maxSizeCompact;
        for(auto elem : vdata) {
            if(elem == nullptr) continue;
            *vlen += static_cast<hsize_t>(strnlen(elem, maxLen)) + 1; // Add null-terminator
        }
        H5Dvlen_reclaim(type, space, H5P_DEFAULT, vdata.data());
        return 1;
    }

    [[nodiscard]] inline size_t getBytesPerElem(const hid::h5t &h5Type) { return H5Tget_size(h5Type); }

    [[nodiscard]] inline size_t getBytesTotal(const hid::h5s &space, const hid::h5t &type) { return getBytesPerElem(type) * getSize(space); }

    [[nodiscard]] inline size_t getBytesTotal(const hid::h5d &dset, std::optional<hid::h5s> space, std::optional<hid::h5t> type) {
        if(not type) type = H5Dget_type(dset);
        if(not space) space = H5Dget_space(dset);
        if(H5Tis_variable_str(type.value()) > 0) {
            hsize_t vlen = 0;
            herr_t  err  = H5Dvlen_get_buf_size_safe(dset, type.value(), space.value(), &vlen);
            if(err >= 0)
                return vlen; // Returns the total number of bytes required to store the dataset
            else
                return getBytesTotal(space.value(), type.value());
        }
        return getBytesTotal(space.value(), type.value());
    }

    [[nodiscard]] inline size_t getBytesTotal(const hid::h5a &attr, std::optional<hid::h5s> space, std::optional<hid::h5t> type) {
        if(not type) type = H5Aget_type(attr);
        if(not space) space = H5Aget_space(attr);
        if(H5Tis_variable_str(type.value()) > 0) {
            hsize_t vlen = 0;
            herr_t  err  = H5Avlen_get_buf_size_safe(attr, type.value(), space.value(), &vlen);
            if(err >= 0)
                return vlen; // Returns the total number of bytes required to store the dataset
            else
                return getBytesTotal(space.value(), type.value());
        }
        return getBytesTotal(space.value(), type.value());
    }

    [[nodiscard]] inline size_t getBytesSelected(const hid::h5s &space, const hid::h5t &type) { return getBytesPerElem(type) * getSizeSelected(space); }

    template<typename DataType>
    void assertWriteBufferIsLargeEnough(const DataType &data, const hid::h5s &space, const hid::h5t &type) {
        if(H5Tget_class(type) == H5T_STRING) {
            if(H5Tis_variable_str(type)) return; // This transfers the string from memory until finding a null terminator
            if constexpr(h5pp::type::sfinae::is_text_v<DataType>) {
                auto hdf5Byte = H5Tget_size(type); // Chars including null-terminator. The memory buffer must fit this size. Also, these many bytes will participate in IO
                auto hdf5Size = getSizeSelected(space);
                auto dataByte = h5pp::util::getCharArraySize(data, false); // Allocated chars including null terminator
                auto dataSize = h5pp::util::getSize(data);
                if(dataByte < hdf5Byte)
                    throw std::runtime_error(h5pp::format("The text buffer given for this write operation is smaller than the selected space in memory.\n"
                                                          "\t Data transfer would read from memory out of bounds\n"
                                                          "\t given    : num strings {} | bytes {} = {} chars + '\\0'\n"
                                                          "\t selected : num strings {} | bytes {} = {} chars + '\\0'\n"
                                                          "\t type     : [{}]",
                                                          dataSize,
                                                          dataByte,
                                                          dataByte - 1,
                                                          hdf5Size,
                                                          hdf5Byte,
                                                          hdf5Byte - 1,
                                                          h5pp::type::sfinae::type_name<DataType>()));
            }
        } else {
            if constexpr(std::is_pointer_v<DataType>) return;
            if constexpr(not h5pp::type::sfinae::has_size_v<DataType>) return;
            auto hdf5Size = getSizeSelected(space);
            auto hdf5Byte = h5pp::util::getBytesPerElem<DataType>() * hdf5Size;
            auto dataByte = h5pp::util::getBytesTotal(data);
            auto dataSize = h5pp::util::getSize(data);
            if(dataByte < hdf5Byte)
                throw std::runtime_error(h5pp::format("The buffer given for this write operation is smaller than the selected space in memory.\n"
                                                      "\t Data transfer would read from memory out of bounds\n"
                                                      "\t given   : size {} | bytes {}\n"
                                                      "\t selected: size {} | bytes {}\n"
                                                      "\t type     : [{}]",
                                                      dataSize,
                                                      dataByte,
                                                      hdf5Size,
                                                      hdf5Byte,
                                                      h5pp::type::sfinae::type_name<DataType>()));
        }
    }

    template<typename DataType>
    void assertReadBufferIsLargeEnough(const DataType &data, const hid::h5s &space, const hid::h5t &type) {
        if(H5Tget_class(type) == H5T_STRING) {
            if(H5Tis_variable_str(type)) return; // These are resized on the fly
            if constexpr(h5pp::type::sfinae::is_text_v<DataType>) {
                auto hdf5Byte = H5Tget_size(type); // Chars including null-terminator. The memory buffer must fit this size. Also, these many bytes will participate in IO
                auto hdf5Size = getSizeSelected(space);
                auto dataByte = h5pp::util::getCharArraySize(data) + 1; // Chars including null terminator
                auto dataSize = h5pp::util::getSize(data);
                if(dataByte < hdf5Byte)
                    throw std::runtime_error(h5pp::format("The text buffer allocated for this read operation is smaller than the string buffer found on the dataset.\n"
                                                          "\t Data transfer would write into memory out of bounds\n"
                                                          "\t allocated buffer: num strings {} | bytes {} = {} chars + '\\0'\n"
                                                          "\t selected  buffer: num strings {} | bytes {} = {} chars + '\\0'\n"
                                                          "\t type     : [{}]",
                                                          dataSize,
                                                          dataByte,
                                                          dataByte - 1,
                                                          hdf5Size,
                                                          hdf5Byte,
                                                          hdf5Byte - 1,
                                                          h5pp::type::sfinae::type_name<DataType>()));
            }
        } else {
            if constexpr(std::is_pointer_v<DataType>) return;
            if constexpr(not h5pp::type::sfinae::has_size_v<DataType>) return;
            auto hdf5Size = getSizeSelected(space);
            auto hdf5Byte = h5pp::util::getBytesPerElem<DataType>() * hdf5Size;
            auto dataByte = h5pp::util::getBytesTotal(data);
            auto dataSize = h5pp::util::getSize(data);
            if(dataByte < hdf5Byte)
                throw std::runtime_error(h5pp::format("The buffer allocated for this read operation is smaller than the selected space in memory.\n"
                                                      "\t Data transfer would write into memory out of bounds\n"
                                                      "\t allocated : size {} | bytes {}\n"
                                                      "\t selected  : size {} | bytes {}\n"
                                                      "\t type     : [{}]",
                                                      dataSize,
                                                      dataByte,
                                                      hdf5Size,
                                                      hdf5Byte,
                                                      h5pp::type::sfinae::type_name<DataType>()));
        }
    }

    template<typename userDataType>
    [[nodiscard]] bool checkBytesPerElemMatch(const hid::h5t &h5Type) {
        size_t dsetTypeSize = h5pp::hdf5::getBytesPerElem(h5Type);
        size_t dataTypeSize = h5pp::util::getBytesPerElem<userDataType>();
        if(H5Tget_class(h5Type) == H5T_STRING) dsetTypeSize = H5Tget_size(H5T_C_S1);
        if(dataTypeSize != dsetTypeSize) {
            // The dsetType may have been generated by H5Tpack, in which case we should check against the native type
            size_t   packedTypesize = dsetTypeSize;
            hid::h5t nativetype     = H5Tget_native_type(h5Type, H5T_DIR_ASCEND);
            dsetTypeSize            = h5pp::hdf5::getBytesPerElem(nativetype);
            if(dataTypeSize != dsetTypeSize)
                h5pp::logger::log->debug("Type size mismatch: dataset type {} bytes | given type {} bytes", dsetTypeSize, dataTypeSize);
            else
                h5pp::logger::log->warn("Detected packed HDF5 type: packed size {} bytes | native size {} bytes. This is not supported by h5pp yet!", packedTypesize, dataTypeSize);
        }
        return dataTypeSize == dsetTypeSize;
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    void assertBytesPerElemMatch(const hid::h5t &h5Type) {
        //        if(h5pp::type::sfinae::is_container_of_v<DataType,std::string>) return; // Each element is potentially a different length!
        size_t dsetTypeSize = h5pp::hdf5::getBytesPerElem(h5Type);
        size_t dataTypeSize = h5pp::util::getBytesPerElem<DataType>();
        if(H5Tget_class(h5Type) == H5T_STRING) dsetTypeSize = H5Tget_size(H5T_C_S1);
        if(dataTypeSize != dsetTypeSize) {
            // The dsetType may have been generated by H5Tpack, in which case we should check against the native type
            size_t   packedTypesize = dsetTypeSize;
            hid::h5t nativetype     = H5Tget_native_type(h5Type, H5T_DIR_ASCEND);
            dsetTypeSize            = h5pp::hdf5::getBytesPerElem(nativetype);
            if(dataTypeSize != dsetTypeSize)
                throw std::runtime_error(h5pp::format("Type size mismatch: dataset type is [{}] bytes | Type of given data is [{}] bytes", dsetTypeSize, dataTypeSize));
            else
                h5pp::logger::log->warn("Detected packed HDF5 type: packed size {} bytes | native size {} bytes. This is not supported by h5pp yet!", packedTypesize, dataTypeSize);
        }
    }

    template<typename DataType>
    inline void setStringSize(const DataType &data, hid::h5t &type, hsize_t &size, size_t &bytes, std::vector<hsize_t> &dims) {
        H5T_class_t dataClass = H5Tget_class(type);
        if(dataClass == H5T_STRING) {
            // The datatype may either be text or a container of text.
            // Examples of pure text are std::string or char[]
            // Example of a container of text is std::vector<std::string>
            // When
            //      1) it is pure text and dimensions are {}
            //          * Space is H5S_SCALAR because dimensions are {}
            //          * Rank is 0 because dimensions are {}
            //          * Size is 1 because size = prod 1*dim(i) * dim(j)...
            //          * We set size H5T_VARIABLE
            //      2) it is pure text and dimensions were specified other than {}
            //          * Space is H5S_SIMPLE
            //          * Rank is 1 or more because dimensions were given as {i,j,k...}
            //          * Size is n, because size = prod 1*dim(i) * dim(j)...
            //          * Here n is number of chars to get from the string buffer
            //          * We set the string size to n because each element is a char.
            //          * We set the dimension to {1}
            //      3) it is a container of text
            //          * Space is H5S_SIMPLE
            //          * The rank is 1 or more
            //          * The space size is the number of strings in the container
            //          * We set size H5T_VARIABLE
            //          * The dimensions remain the size of the container

            if(H5Tget_size(type) != 1) return; // String properties have already been set, probably by the user

            herr_t retval = 0;
            if constexpr(h5pp::type::sfinae::is_text_v<DataType>) {
                if(dims.empty()) {
                    // Case 1
                    retval = H5Tset_size(type, H5T_VARIABLE);
                    bytes  = h5pp::util::getBytesTotal(data);
                } else {
                    // Case 2
                    hsize_t desiredSize = h5pp::util::getSizeFromDimensions(dims);
                    // Consider a case where the text is "this is a text"
                    // but the desired size is 12.
                    // The resulting string should be "this is a t'\0'",
                    // with 12 characters in total including the null terminator.
                    // HDF5 seems to do this automatically. From the manual:
                    // "If the short string is H5T_STR_NULLTERM, it is truncated
                    // and a null terminator is appended."
                    // Furthermore
                    // "The size set for a string datatype should include space for
                    // the null-terminator character, otherwise it will not be
                    // stored on (or retrieved from) disk"
                    retval = H5Tset_size(type, desiredSize); // Desired size should be num chars + null terminator
                    dims   = {};
                    size   = 1;
                    bytes  = desiredSize;
                }
            } else if(h5pp::type::sfinae::has_text_v<DataType>) {
                // Case 3
                retval = H5Tset_size(type, H5T_VARIABLE);
                bytes  = h5pp::util::getBytesTotal(data);
            }

            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to set size on string");
            }
            // The following makes sure there is a single "\0" at the end of the string when written to file.
            // Note however that bytes here is supposed to be the number of characters including null terminator.
            retval = H5Tset_strpad(type, H5T_STR_NULLTERM);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to set strpad");
            }
            // Sets the character set to UTF-8
            retval = H5Tset_cset(type, H5T_cset_t::H5T_CSET_UTF8);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to set char-set UTF-8");
            }
        }
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc_or_hid_t<h5x>>
    [[nodiscard]] inline bool
        checkIfLinkExists(const h5x &loc, std::string_view linkPath, std::optional<bool> linkExists = std::nullopt, const hid::h5p &linkAccess = H5P_DEFAULT) {
        if(linkExists) return linkExists.value();
        for(const auto &subPath : pathCumulativeSplit(linkPath, "/")) {
            int exists = H5Lexists(loc, util::safe_str(subPath).c_str(), linkAccess);
            if(exists == 0) {
                h5pp::logger::log->trace("Checking if link exists [{}] ... false", linkPath);
                return false;
            }
            if(exists < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to check if link exists [{}]", linkPath));
            }
        }
        h5pp::logger::log->trace("Checking if link exists [{}] ... true", linkPath);
        return true;
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc_or_hid_t<h5x>>
    [[nodiscard]] inline bool
        checkIfDatasetExists(const h5x &loc, std::string_view dsetName, std::optional<bool> dsetExists = std::nullopt, const hid::h5p &dsetAccess = H5P_DEFAULT) {
        if(dsetExists) return dsetExists.value();
        dsetExists = checkIfLinkExists(loc, dsetName, dsetExists, dsetAccess);
        if(not dsetExists.value()) return false;
        hid::h5o   object     = H5Oopen(loc, util::safe_str(dsetName).c_str(), dsetAccess);
        H5I_type_t objectType = H5Iget_type(object);
        if(objectType != H5I_DATASET) {
            h5pp::logger::log->trace("Checking if link is a dataset [{}] ... false", dsetName);
            return false;
        } else {
            h5pp::logger::log->trace("Checking if link is a dataset [{}] ... true", dsetName);
            return true;
        }
    }

    template<typename h5x, typename h5x_loc, typename = h5pp::type::sfinae::enable_if_is_h5_loc_or_hid_t<h5x_loc>>
    [[nodiscard]] h5x openLink(const h5x_loc &loc, std::string_view linkPath, std::optional<bool> linkExists = std::nullopt, const hid::h5p &linkAccess = H5P_DEFAULT) {
        if(checkIfLinkExists(loc, linkPath, linkExists)) {
            if constexpr(std::is_same_v<h5x, hid::h5d>) h5pp::logger::log->trace("Opening dataset [{}]", linkPath);
            if constexpr(std::is_same_v<h5x, hid::h5g>) h5pp::logger::log->trace("Opening group [{}]", linkPath);
            if constexpr(std::is_same_v<h5x, hid::h5o>) h5pp::logger::log->trace("Opening object [{}]", linkPath);
            h5x link;
            if constexpr(std::is_same_v<h5x, hid::h5d>) link = H5Dopen(loc, util::safe_str(linkPath).c_str(), linkAccess);
            if constexpr(std::is_same_v<h5x, hid::h5g>) link = H5Gopen(loc, util::safe_str(linkPath).c_str(), linkAccess);
            if constexpr(std::is_same_v<h5x, hid::h5o>) link = H5Oopen(loc, util::safe_str(linkPath).c_str(), linkAccess);
            if(link < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to open existing link [{}]", linkPath));
            } else {
                return link;
            }
        } else {
            throw std::runtime_error(h5pp::format("Link does not exist [{}]", linkPath));
        }
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_link_or_hid_t<h5x>>
    [[nodiscard]] inline bool checkIfAttributeExists(const h5x &         link,
                                                     std::string_view    linkPath,
                                                     std::string_view    attrName,
                                                     std::optional<bool> attrExists = std::nullopt,
                                                     const hid::h5p &    linkAccess = H5P_DEFAULT) {
        if(attrExists and attrExists.value()) return true;
        h5pp::logger::log->trace("Checking if attribute [{}] exitst in link [{}]", attrName, linkPath);
        bool exists = H5Aexists_by_name(link, std::string(".").c_str(), util::safe_str(attrName).c_str(), linkAccess) > 0;
        h5pp::logger::log->trace("Checking if attribute [{}] exitst in link [{}] ... {}", attrName, linkPath, exists);
        return exists;
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc_or_hid_t<h5x>>
    [[nodiscard]] inline bool checkIfAttributeExists(const h5x &         loc,
                                                     std::string_view    linkPath,
                                                     std::string_view    attrName,
                                                     std::optional<bool> linkExists = std::nullopt,
                                                     std::optional<bool> attrExists = std::nullopt,
                                                     const hid::h5p &    linkAccess = H5P_DEFAULT) {
        if(linkExists and attrExists and linkExists.value() and attrExists.value()) return true;
        auto link = openLink<hid::h5o>(loc, linkPath, linkExists, linkAccess);
        return checkIfAttributeExists(link, linkPath, attrName, attrExists, linkAccess);
    }

    [[nodiscard]] inline bool H5Tequal_recurse(const hid::h5t &type1, const hid::h5t &type2) {
        // If types are compound, check recursively that all members have equal types and names
        H5T_class_t dataClass1 = H5Tget_class(type1);
        H5T_class_t dataClass2 = H5Tget_class(type1);
        if(dataClass1 == H5T_COMPOUND and dataClass2 == H5T_COMPOUND) {
            size_t sizeType1 = H5Tget_size(type1);
            size_t sizeType2 = H5Tget_size(type2);
            if(sizeType1 != sizeType2) return false;
            auto nMembers1 = H5Tget_nmembers(type1);
            auto nMembers2 = H5Tget_nmembers(type2);
            if(nMembers1 != nMembers2) return false;
            for(auto idx = 0; idx < nMembers1; idx++) {
                hid::h5t         t1    = H5Tget_member_type(type1, static_cast<unsigned int>(idx));
                hid::h5t         t2    = H5Tget_member_type(type2, static_cast<unsigned int>(idx));
                char *           mem1  = H5Tget_member_name(type1, static_cast<unsigned int>(idx));
                char *           mem2  = H5Tget_member_name(type2, static_cast<unsigned int>(idx));
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
        htri_t zlibAvail = H5Zfilter_avail(H5Z_FILTER_DEFLATE);
        if(zlibAvail) {
            unsigned int filterInfo;
            H5Zget_filter_info(H5Z_FILTER_DEFLATE, &filterInfo);
            bool zlib_encode = (filterInfo & H5Z_FILTER_CONFIG_ENCODE_ENABLED);
            bool zlib_decode = (filterInfo & H5Z_FILTER_CONFIG_DECODE_ENABLED);
            return zlibAvail and zlib_encode and zlib_decode;
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

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_link<h5x>>
    [[nodiscard]] inline std::vector<std::string> getAttributeNames(const h5x &link) {
        auto                     numAttrs = H5Aget_num_attrs(link);
        std::vector<std::string> attrNames;
        for(auto i = 0; i < numAttrs; i++) {
            hid::h5a attrId  = H5Aopen_idx(link, static_cast<unsigned int>(i));
            ssize_t  bufSize = 0;
            bufSize          = H5Aget_name(attrId, static_cast<size_t>(bufSize), nullptr);
            if(bufSize >= 0) {
                std::string buf;
                buf.resize(static_cast<size_t>(bufSize) + 1);
                H5Aget_name(attrId, static_cast<size_t>(bufSize) + 1, buf.data());
                attrNames.emplace_back(buf);
            } else {
                H5Eprint(H5E_DEFAULT, stderr);
                h5pp::logger::log->debug("Failed to get attribute names");
            }
        }
        return attrNames;
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    [[nodiscard]] inline std::vector<std::string>
        getAttributeNames(const h5x &loc, std::string_view linkPath, std::optional<bool> linkExists = std::nullopt, const hid::h5p &linkAccess = H5P_DEFAULT) {
        auto link = openLink<hid::h5o>(loc, linkPath, linkExists, linkAccess);
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
        if(H5Tcommitted(type) > 0) {
            auto bufSize = H5Iget_name(type, nullptr, 0);
            h5pp::logger::log->debug("buf_size {}", bufSize);
            std::string name;
            if(bufSize > 0) {
                name.resize(static_cast<std::string::size_type>(bufSize) + 1);
                H5Iget_name(type, name.data(), static_cast<size_t>(bufSize) + 1);
                H5Eprint(H5E_DEFAULT, stderr);
            }
            h5pp::logger::log->debug("No C++ type match for HDF5 type [{}]", name);
        } else {
            h5pp::logger::log->debug("No C++ type match for non-committed HDF5 type");
        }

        return getCppType<std::nullopt_t>();
    }

    inline TypeInfo getTypeInfo(std::optional<std::string> objectPath, std::optional<std::string> objectName, const hid::h5s &h5Space, const hid::h5t &h5Type) {
        TypeInfo typeInfo;
        typeInfo.h5Name                                                              = std::move(objectName);
        typeInfo.h5Path                                                              = std::move(objectPath);
        typeInfo.h5Type                                                              = h5Type;
        typeInfo.h5Rank                                                              = h5pp::hdf5::getRank(h5Space);
        typeInfo.h5Size                                                              = h5pp::hdf5::getSize(h5Space);
        typeInfo.h5Dims                                                              = h5pp::hdf5::getDimensions(h5Space);
        std::tie(typeInfo.cppTypeIndex, typeInfo.cppTypeName, typeInfo.cppTypeBytes) = getCppType(typeInfo.h5Type.value());
        return typeInfo;
    }

    inline TypeInfo getTypeInfo(const hid::h5d &dataset) {
        auto        bufSize = H5Iget_name(dataset, nullptr, 0);
        std::string dsetPath;
        dsetPath.resize(static_cast<std::string::size_type>(bufSize) + 1);
        H5Iget_name(dataset, dsetPath.data(), static_cast<size_t>(bufSize) + 1);
        h5pp::logger::log->trace("Collecting type info about dataset [{}]", dsetPath);
        return getTypeInfo(dsetPath, std::nullopt, H5Dget_space(dataset), H5Dget_type(dataset));
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline TypeInfo getTypeInfo(const h5x &loc, std::string_view dsetName, std::optional<bool> dsetExists = std::nullopt, const hid::h5p &dsetAccess = H5P_DEFAULT) {
        auto dataset = openLink<hid::h5d>(loc, dsetName, dsetExists, dsetAccess);
        return getTypeInfo(dataset);
    }

    inline TypeInfo getTypeInfo(const hid::h5a &attribute) {
        auto        bufSize = H5Aget_name(attribute, 0, nullptr);
        std::string attrName;
        attrName.resize(static_cast<std::string::size_type>(bufSize) + 1);
        H5Aget_name(attribute, static_cast<size_t>(bufSize) + 1, attrName.data());
        bufSize = H5Iget_name(attribute, nullptr, 0);
        std::string linkPath;
        linkPath.resize(static_cast<std::string::size_type>(bufSize) + 1);
        H5Iget_name(attribute, linkPath.data(), static_cast<size_t>(bufSize) + 1);

        h5pp::logger::log->trace("Collecting type info about attribute [{}] in link [{}]", attrName, linkPath);
        return getTypeInfo(linkPath, attrName, H5Aget_space(attribute), H5Aget_type(attribute));
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline TypeInfo getTypeInfo(const h5x &         loc,
                                std::string_view    linkPath,
                                std::string_view    attrName,
                                std::optional<bool> linkExists = std::nullopt,
                                std::optional<bool> attrExists = std::nullopt,
                                const hid::h5p &    linkAccess = H5P_DEFAULT) {
        auto link = openLink<hid::h5o>(loc, linkPath, linkExists, linkAccess);
        if(checkIfAttributeExists(link, linkPath, attrName, attrExists, linkAccess)) {
            hid::h5a attribute = H5Aopen_name(link, util::safe_str(attrName).c_str());
            return getTypeInfo(attribute);
        } else {
            throw std::runtime_error(h5pp::format("Attribute [{}] does not exist in link [{}]", attrName, linkPath));
        }
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_link<h5x>>
    std::vector<TypeInfo> getTypeInfo_allAttributes(const h5x &link) {
        std::vector<TypeInfo> allAttrInfo;
        int                   num_attrs = H5Aget_num_attrs(link);
        if(num_attrs < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to get number of attributes in link");
        }
        for(int idx = 0; idx < num_attrs; idx++) {
            hid::h5a attribute = H5Aopen_idx(link, static_cast<unsigned int>(idx));
            allAttrInfo.emplace_back(getTypeInfo(attribute));
        }
        return allAttrInfo;
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline std::vector<TypeInfo>
        getTypeInfo_allAttributes(const h5x &loc, std::string_view linkPath, std::optional<bool> linkExists = std::nullopt, const hid::h5p &linkAccess = H5P_DEFAULT) {
        auto link = openLink<hid::h5o>(loc, linkPath, linkExists, linkAccess);
        return getTypeInfo_allAttributes(link);
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline void createGroup(const h5x &loc, std::string_view groupRelativeName, std::optional<bool> linkExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        // Check if group exists already
        linkExists = checkIfLinkExists(loc, groupRelativeName, linkExists, plists.linkAccess);
        if(linkExists.value()) {
            h5pp::logger::log->trace("Group already exists [{}]", groupRelativeName);
            return;
        } else {
            h5pp::logger::log->trace("Creating group link [{}]", groupRelativeName);
            hid::h5g group = H5Gcreate(loc, util::safe_str(groupRelativeName).c_str(), plists.linkCreate, plists.groupCreate, plists.groupAccess);
        }
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline void writeSymbolicLink(const h5x &loc, std::string_view srcPath, std::string_view tgtPath, const PropertyLists &plists = PropertyLists()) {
        if(checkIfLinkExists(loc, srcPath, std::nullopt, plists.linkAccess)) {
            h5pp::logger::log->trace("Creating symbolic link [{}] --> [{}]", srcPath, tgtPath);
            herr_t retval = H5Lcreate_soft(util::safe_str(srcPath).c_str(), loc, util::safe_str(tgtPath).c_str(), plists.linkCreate, plists.linkAccess);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to write symbolic link [{}]  ", srcPath));
            }
        } else {
            throw std::runtime_error(h5pp::format("Trying to write soft link to non-existing path [{}]", srcPath));
        }
    }

    inline void setProperty_layout(DsetInfo &dsetInfo) {
        if(not dsetInfo.h5PlistDsetCreate) throw std::logic_error("Could not configure the H5D layout: the dataset creation property list has not been initialized");
        if(not dsetInfo.h5Layout) throw std::logic_error("Could not configure the H5D layout: the H5D layout parameter has not been initialized");
        switch(dsetInfo.h5Layout.value()) {
            case H5D_CHUNKED: h5pp::logger::log->trace("Setting layout H5D_CHUNKED"); break;
            case H5D_COMPACT: h5pp::logger::log->trace("Setting layout H5D_COMPACT"); break;
            case H5D_CONTIGUOUS: h5pp::logger::log->trace("Setting layout H5D_CONTIGUOUS"); break;
            default: throw std::runtime_error("Given invalid layout when creating dataset property list. Choose one of H5D_COMPACT,H5D_CONTIGUOUS,H5D_CHUNKED");
        }
        herr_t err = H5Pset_layout(dsetInfo.h5PlistDsetCreate.value(), dsetInfo.h5Layout.value());
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Could not set layout");
        }
    }

    inline void setProperty_chunkDims(DsetInfo &dsetInfo) {
        if(dsetInfo.h5Layout != H5D_CHUNKED and not dsetInfo.dsetChunk) return;
        if(dsetInfo.h5Layout != H5D_CHUNKED and dsetInfo.dsetChunk) {
            h5pp::logger::log->warn("Chunk dimensions {} ignored: The given dataset layout is not H5D_CHUNKED", dsetInfo.dsetChunk.value());
            dsetInfo.dsetChunk = std::nullopt;
            return;
        }

        if(H5Sget_simple_extent_type(dsetInfo.h5Space.value()) == H5S_SCALAR) {
            h5pp::logger::log->warn("Chunk dimensions ignored: Space is H5S_SCALAR");
            dsetInfo.dsetChunk   = std::nullopt;
            dsetInfo.dsetDimsMax = std::nullopt;
            dsetInfo.h5Layout    = H5D_CONTIGUOUS; // In case it's a big text
            dsetInfo.resizeMode  = h5pp::ResizeMode::DO_NOT_RESIZE;
            setProperty_layout(dsetInfo);
            return;
        }

        if(not dsetInfo.h5PlistDsetCreate) throw std::logic_error("Could not configure chunk dimensions: the dataset creation property list has not been initialized");
        if(not dsetInfo.dsetRank) throw std::logic_error("Could not configure chunk dimensions: the dataset rank (n dims) has not been initialized");
        if(not dsetInfo.dsetDims) throw std::logic_error("Could not configure chunk dimensions: the dataset dimensions have not been initialized");
        if(dsetInfo.dsetRank.value() != static_cast<int>(dsetInfo.dsetDims->size()))
            throw std::logic_error(
                h5pp::format("Could not set chunk dimensions properties: Rank mismatch: dataset dimensions {} has different number of elements than reported rank {}",
                             dsetInfo.dsetDims.value(),
                             dsetInfo.dsetRank.value()));
        if(dsetInfo.dsetDims->size() != dsetInfo.dsetChunk->size())
            throw std::logic_error(
                h5pp::format("Could not configure chunk dimensions: Rank mismatch: dataset dimensions {} and chunk dimensions {} do not have the same number of elements",
                             dsetInfo.dsetDims->size(),
                             dsetInfo.dsetChunk->size()));

        h5pp::logger::log->trace("Setting chunk dimensions {}", dsetInfo.dsetChunk.value());
        herr_t err = H5Pset_chunk(dsetInfo.h5PlistDsetCreate.value(), static_cast<int>(dsetInfo.dsetChunk->size()), dsetInfo.dsetChunk->data());
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Could not set chunk dimensions");
        }
    }

    inline void setProperty_compression(DsetInfo &dsetInfo) {
        if(not dsetInfo.compression) return;
        if(not checkIfCompressionIsAvailable()) return;
        if(not dsetInfo.h5PlistDsetCreate) throw std::runtime_error("Could not configure compression: field h5_plist_dset_create has not been initialized");
        if(not dsetInfo.h5Layout) throw std::logic_error("Could not configure compression: field h5_layout has not been initialized");

        if(dsetInfo.h5Layout.value() != H5D_CHUNKED) {
            h5pp::logger::log->trace("Compression ignored: Layout is not H5D_CHUNKED");
            dsetInfo.compression = std::nullopt;
            return;
        }
        if(dsetInfo.compression and dsetInfo.compression.value() > 9) {
            h5pp::logger::log->warn("Compression level too high: [{}]. Reducing to [9]", dsetInfo.compression.value());
            dsetInfo.compression = 9;
        }
        h5pp::logger::log->trace("Setting compression level {}", dsetInfo.compression.value());
        herr_t err = H5Pset_deflate(dsetInfo.h5PlistDsetCreate.value(), dsetInfo.compression.value());
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to set compression level. Check that your HDF5 version has zlib enabled.");
        }
    }

    inline void selectHyperslab(const hid::h5s &space, const Hyperslab &hyperSlab, H5S_seloper_t selectOp = H5S_SELECT_OR) {
        if(hyperSlab.empty()) return;
        int rank = H5Sget_simple_extent_ndims(space);
        if(rank < 0) throw std::runtime_error("Failed to read space rank");
        std::vector<hsize_t> dims(static_cast<size_t>(rank));
        H5Sget_simple_extent_dims(space, dims.data(), nullptr);
        // If one of slabOffset or slabExtent is given, then the other must also be given
        if(hyperSlab.offset and not hyperSlab.extent) throw std::logic_error("Could not setup hyperslab metadata: Given hyperslab offset but not extent");
        if(not hyperSlab.offset and hyperSlab.extent) throw std::logic_error("Could not setup hyperslab metadata: Given hyperslab extent but not offset");

        // If given, ranks of slabOffset and slabExtent must be identical to each other and to the rank of the existing dataset
        if(hyperSlab.offset and hyperSlab.extent and (hyperSlab.offset.value().size() != hyperSlab.extent.value().size()))
            throw std::logic_error(h5pp::format(
                "Could not setup hyperslab metadata: Size mismatch in given hyperslab arrays: offset {} | extent {}", hyperSlab.offset.value(), hyperSlab.extent.value()));

        if(hyperSlab.offset and hyperSlab.offset.value().size() != static_cast<size_t>(rank))
            throw std::logic_error(h5pp::format("Could not setup hyperslab metadata: Hyperslab arrays have different rank compared to the given space: "
                                                "offset {} | extent {} | space dims {}",
                                                hyperSlab.offset.value(),
                                                hyperSlab.extent.value(),
                                                dims));

        // If given, slabStride must have the same rank as the dataset
        if(hyperSlab.stride and hyperSlab.stride.value().size() != static_cast<size_t>(rank))
            throw std::logic_error(h5pp::format("Could not setup hyperslab metadata: Hyperslab stride has a different rank compared to the dataset: "
                                                "stride {} | dataset dims {}",
                                                hyperSlab.stride.value(),
                                                dims));
        // If given, slabBlock must have the same rank as the dataset
        if(hyperSlab.blocks and hyperSlab.blocks.value().size() != static_cast<size_t>(rank))
            throw std::logic_error(h5pp::format("Could not setup hyperslab metadata: Hyperslab blocks has a different rank compared to the dataset: "
                                                "blocks {} | dataset dims {}",
                                                hyperSlab.blocks.value(),
                                                dims));

        if(H5Sget_select_type(space) != H5S_SEL_HYPERSLABS) selectOp = H5S_SELECT_SET; // First operation must be H5S_SELECT_SET.

        /* clang-format off */
        if(hyperSlab.offset and hyperSlab.extent and hyperSlab.stride and hyperSlab.blocks)
            H5Sselect_hyperslab(space, selectOp, hyperSlab.offset.value().data(), hyperSlab.stride.value().data(), hyperSlab.extent.value().data(), hyperSlab.blocks.value().data());
        else if (hyperSlab.offset and hyperSlab.extent and hyperSlab.stride)
            H5Sselect_hyperslab(space, selectOp, hyperSlab.offset.value().data(), hyperSlab.stride.value().data(), hyperSlab.extent.value().data(), nullptr);
        else if (hyperSlab.offset and hyperSlab.extent and hyperSlab.blocks)
            H5Sselect_hyperslab(space, selectOp, hyperSlab.offset.value().data(), nullptr, hyperSlab.extent.value().data(), hyperSlab.blocks.value().data());
        else if (hyperSlab.offset and hyperSlab.extent)
            H5Sselect_hyperslab(space, selectOp, hyperSlab.offset.value().data(), nullptr, hyperSlab.extent.value().data(), nullptr);

            /* clang-format on */
#if H5_VERSION_GE(1, 10, 0)
        htri_t is_regular = H5Sis_regular_hyperslab(space);
        if(not is_regular) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Hyperslab selection is irregular (non-rectangular).\n"
                                                  "This is not yet supported by h5pp"));
        }
#endif
        htri_t valid = H5Sselect_valid(space);
        if(valid < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            Hyperslab slab(space);
            throw std::runtime_error(h5pp::format("Hyperslab selection is invalid {}", slab.string()));
        }
    }

    inline void selectHyperslabs(hid::h5s &space, const std::vector<Hyperslab> &hyperSlabs, std::optional<std::vector<H5S_seloper_t>> hyperSlabSelectOps = std::nullopt) {
        if(hyperSlabSelectOps and not hyperSlabSelectOps->empty()) {
            if(hyperSlabs.size() != hyperSlabSelectOps->size())
                for(const auto &slab : hyperSlabs) selectHyperslab(space, slab, hyperSlabSelectOps->at(0));
            else
                for(size_t num = 0; num < hyperSlabs.size(); num++) selectHyperslab(space, hyperSlabs[num], hyperSlabSelectOps->at(num));

        } else
            for(const auto &slab : hyperSlabs) selectHyperslab(space, slab);
    }

    inline void setSpaceExtent(const hid::h5s &h5Space, const std::vector<hsize_t> &dims, std::optional<std::vector<hsize_t>> maxDims = std::nullopt) {
        if(H5Sget_simple_extent_type(h5Space) == H5S_SCALAR) return;
        if(dims.empty()) return;
        herr_t err;
        if(maxDims) {
            if(dims.size() != maxDims->size()) throw std::runtime_error(h5pp::format("Rank mismatch in dimensions {} and max dimensions {}", dims, maxDims.value()));
            std::vector<long> maxDimsLong;
            for(auto &dim : maxDims.value()) {
                if(dim == H5S_UNLIMITED)
                    maxDimsLong.emplace_back(-1);
                else
                    maxDimsLong.emplace_back(static_cast<long>(dim));
            }
            h5pp::logger::log->trace("Setting dataspace extents: dims {} | max dims {}", dims, maxDimsLong);
            err = H5Sset_extent_simple(h5Space, static_cast<int>(dims.size()), dims.data(), maxDims->data());
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to set extents on space: dims {} | max dims {}", dims, maxDims.value()));
            }
        } else {
            h5pp::logger::log->trace("Setting dataspace extents: dims {}", dims);
            err = H5Sset_extent_simple(h5Space, static_cast<int>(dims.size()), dims.data(), nullptr);
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to set extents on space. Dims {}", dims));
            }
        }
    }

    inline void setSpaceExtent(DsetInfo &dsetInfo) {
        if(not dsetInfo.h5Space) throw std::logic_error("Could not set space extent: the space is not initialized");
        if(not dsetInfo.h5Space->valid()) throw std::runtime_error("Could not set space extent. Space is not valid");
        if(H5Sget_simple_extent_type(dsetInfo.h5Space.value()) == H5S_SCALAR) return;
        if(not dsetInfo.dsetDims) throw std::runtime_error("Could not set space extent: dataset dimensions are not defined");

        if(dsetInfo.h5Layout and dsetInfo.h5Layout.value() == H5D_CHUNKED and not dsetInfo.dsetDimsMax) {
            // Chunked datasets are unlimited unless told explicitly otherwise
            dsetInfo.dsetDimsMax = std::vector<hsize_t>(static_cast<size_t>(dsetInfo.dsetRank.value()), 0);
            std::fill_n(dsetInfo.dsetDimsMax->begin(), dsetInfo.dsetDimsMax->size(), H5S_UNLIMITED);
        }
        try {
            setSpaceExtent(dsetInfo.h5Space.value(), dsetInfo.dsetDims.value(), dsetInfo.dsetDimsMax);
        } catch(const std::exception &err) { throw std::runtime_error(h5pp::format("Failed to set extent on dataset {} \n Reason {}", dsetInfo.string(), err.what())); }
    }

    inline void extendSpace(const hid::h5s &space, const int dim, const hsize_t extent) {
        h5pp::logger::log->trace("Extending space dimension [{}] to extent [{}]", dim, extent);
        // Retrieve the current extent of this space
        const int            oldRank = H5Sget_simple_extent_ndims(space);
        std::vector<hsize_t> oldDims(static_cast<size_t>(oldRank));
        H5Sget_simple_extent_dims(space, oldDims.data(), nullptr);

        // We may need to change the rank, for instance, if we are appending a new column
        // to a vector of size n, so it becomes an (n x 2) "matrix".
        const int            newRank = std::max(dim + 1, oldRank);
        std::vector<hsize_t> newDims(static_cast<size_t>(newRank), 1);
        std::copy(oldDims.begin(), oldDims.end(), newDims.begin());
        newDims[static_cast<size_t>(dim)] = extent;
        setSpaceExtent(space, newDims);
        //        H5Sset_extent_simple(space,newRank,newDims.data(),nullptr);
    }

    inline void extendDataset(const hid::h5d &dataset, const int dim, const hsize_t extent) {
        // Retrieve the current size of the memSpace (act as if you don't know its size and want to append)
        hid::h5s space = H5Dget_space(dataset);
        extendSpace(space, dim, extent);
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline void extendDataset(const h5x &         loc,
                              std::string_view    datasetRelativeName,
                              const int           dim,
                              const hsize_t       extent,
                              std::optional<bool> linkExists = std::nullopt,
                              const hid::h5p &    dsetAccess = H5P_DEFAULT) {
        auto dataset = openLink<hid::h5d>(loc, datasetRelativeName, linkExists, dsetAccess);
        extendDataset(dataset, dim, extent);
    }

    template<typename DataType, typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    void extendDataset(const h5x &loc, const DataType &data, std::string_view dsetPath) {
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_core_v<DataType>) {
            extendDataset(loc, dsetPath, 0, data.rows());
            hid::h5d             dataSet   = openLink<hid::h5d>(loc, dsetPath);
            hid::h5s             fileSpace = H5Dget_space(dataSet);
            int                  ndims     = H5Sget_simple_extent_ndims(fileSpace);
            std::vector<hsize_t> dims(static_cast<size_t>(ndims));
            H5Sget_simple_extent_dims(fileSpace, dims.data(), nullptr);
            H5Sclose(fileSpace);
            if(dims[1] < static_cast<hsize_t>(data.cols())) extendDataset(loc, dsetPath, 1, data.cols());
        } else
#endif
        {
            extendDataset(loc, dsetPath, 0, h5pp::util::getSize(data));
        }
    }

    inline void extendDataset(DsetInfo &info, const std::vector<hsize_t> &appDimensions, size_t axis) {
        // We use this function to EXTEND the dataset to APPEND given data
        info.assertResizeReady();
        int appRank = static_cast<int>(appDimensions.size());
        if(H5Tis_variable_str(info.h5Type.value()) > 0) {
            // These are resized on the fly
            return;
        } else {
            // Sanity checks
            if(info.dsetRank.value() <= static_cast<int>(axis))
                throw std::runtime_error(h5pp::format("Could not append to dataset [{}] along axis {}: Dataset rank ({}) must be strictly larger than the given axis ({})",
                                                      info.dsetPath.value(),
                                                      axis,
                                                      info.dsetRank.value(),
                                                      axis));
            if(info.dsetRank.value() < appRank)
                throw std::runtime_error(
                    h5pp::format("Cannot append to dataset [{}] along axis {}: Dataset rank {} < appended rank {}", info.dsetPath.value(), axis, info.dsetRank.value(), appRank));

            // If we have a dataset with dimensions ijkl and we want to append along j, say, then the remaining
            // ikl should be at least as large as the corresponding dimensions on the given data.
            for(size_t idx = 0; idx < appDimensions.size(); idx++)
                if(idx != axis and appDimensions[idx] > info.dsetDims.value()[idx])
                    throw std::runtime_error(h5pp::format("Could not append to dataset [{}] along axis {}: Dimension {} size mismatch: data {} > dset{}",
                                                          info.dsetPath.value(),
                                                          axis,
                                                          idx,
                                                          appDimensions[idx],
                                                          info.dsetDims.value()[idx]));

            // Compute the new dset dimension. Note that dataRank <= dsetRank,
            // For instance when we add a column to a matrix, the column may be an nx1 vector.
            // Therefore we embed the data dimensions in a (possibly) higher-dimensional space
            auto embeddedDims = std::vector<hsize_t>(static_cast<size_t>(info.dsetRank.value()), 1);
            std::copy(appDimensions.begin(), appDimensions.end(), embeddedDims.begin()); // In the example above, we get nx1
            auto oldAxisSize  = info.dsetDims.value()[axis];                             // Will need this later when drawing the hyperspace
            auto newAxisSize  = embeddedDims[axis];                                      // Will need this later when drawing the hyperspace
            auto newDsetDims  = info.dsetDims.value();
            newDsetDims[axis] = oldAxisSize + newAxisSize;

            // Set the new dimensions
            std::string oldInfoStr;
            if(h5pp::logger::getLogLevel() <= 1) oldInfoStr = info.string();
            herr_t err = H5Dset_extent(info.h5Dset.value(), newDsetDims.data());
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to set extent {} on dataset [{}]", newDsetDims, info.dsetPath.value()));
            }
            // By default, all the space (old and new) is selected
            info.dsetDims = newDsetDims;
            info.h5Space  = H5Dget_space(info.h5Dset->value()); // Needs to be refreshed after H5Dset_extent
            info.dsetByte = h5pp::hdf5::getBytesTotal(info.h5Dset.value(), info.h5Space, info.h5Type);
            info.dsetSize = h5pp::hdf5::getSize(info.h5Space.value());
            info.dsetRank = h5pp::hdf5::getRank(info.h5Space.value());

            // Now se select the space on the extended dataset where the given data will fit
            // Draw the target space on a hyperslab
            Hyperslab slab;
            slab.extent               = embeddedDims;
            slab.offset               = std::vector<hsize_t>(static_cast<size_t>(info.dsetRank.value()), 0);
            slab.offset.value()[axis] = oldAxisSize;
            h5pp::hdf5::selectHyperslab(info.h5Space.value(), slab);
            h5pp::logger::log->debug("Extended dataset \n \t old: {} \n \t new: {}", oldInfoStr, info.string());
        }
    }

    inline void extendDataset(DsetInfo &dsetInfo, const DataInfo &dataInfo, size_t axis) {
        // We use this function to EXTEND the dataset to APPEND given data
        dataInfo.assertWriteReady();
        extendDataset(dsetInfo, dataInfo.dataDims.value(), axis);
    }

    inline void resizeDataset(DsetInfo &info, const std::vector<hsize_t> &newDimensions, std::optional<h5pp::ResizeMode> mode = std::nullopt) {
        if(not mode) mode = info.resizeMode;
        if(not mode) mode = h5pp::ResizeMode::RESIZE_TO_FIT;
        if(mode == h5pp::ResizeMode::DO_NOT_RESIZE) return;
        if(info.h5Layout and info.h5Layout.value() != H5D_CHUNKED) switch(info.h5Layout.value()) {
                case H5D_COMPACT: throw std::runtime_error("Datasets with H5D_COMPACT layout cannot be resized");
                case H5D_CONTIGUOUS: throw std::runtime_error("Datasets with H5D_CONTIGUOUS layout cannot be resized");
                default: break;
            }
        if(not info.dsetPath) throw std::runtime_error("Could not resize dataset: Path undefined");
        if(not info.h5Space) throw std::runtime_error(h5pp::format("Could not resize dataset [{}]: info.h5Space undefined", info.dsetPath.value()));
        if(not info.h5Type) throw std::runtime_error(h5pp::format("Could not resize dataset [{}]: info.h5Type undefined", info.dsetPath.value()));
        if(H5Sget_simple_extent_type(info.h5Space.value()) == H5S_SCALAR) return; // These are not supposed to be resized. Typically strings
        if(H5Tis_variable_str(info.h5Type.value()) > 0) return;                   // These are resized on the fly
        info.assertResizeReady();

        // Return if there is no change compared to the current dimensions
        if(info.dsetDims.value() == newDimensions) return;
        // Compare ranks
        if(info.dsetDims->size() != newDimensions.size())
            throw std::runtime_error(h5pp::format("Could not resize dataset [{}]: "
                                                  "Rank mismatch: "
                                                  "The given dimensions {} must have the same number of elements as the target dimensions {}",
                                                  info.dsetPath.value(),
                                                  info.dsetDims.value(),
                                                  newDimensions));
        if(mode == h5pp::ResizeMode::INCREASE_ONLY) {
            bool allDimsAreSmaller = true;
            for(size_t idx = 0; idx < newDimensions.size(); idx++)
                if(newDimensions[idx] > info.dsetDims.value()[idx]) allDimsAreSmaller = false;
            if(allDimsAreSmaller) return;
        }
        std::string oldInfoStr;
        if(h5pp::logger::getLogLevel() <= 1) oldInfoStr = info.string();
        // Chunked datasets can shrink and grow in any direction
        // Non-chunked datasets can't be resized at all

        for(size_t idx = 0; idx < newDimensions.size(); idx++) {
            if(newDimensions[idx] > info.dsetDimsMax.value()[idx])
                throw std::runtime_error(h5pp::format("Could not resize dataset [{}]: "
                                                      "Dimension size error: "
                                                      "The target dimensions {} are larger than the maximum dimensions {} for this dataset. "
                                                      "Consider creating the dataset with larger maximum dimensions or use H5D_CHUNKED layout to enable unlimited resizing",
                                                      info.dsetPath.value(),
                                                      newDimensions,
                                                      info.dsetDimsMax.value()));
        }

        herr_t err = H5Dset_extent(info.h5Dset.value(), newDimensions.data());
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to resize dataset [{}] from dimensions {} to {}", info.dsetPath.value(), info.dsetDims.value(), newDimensions));
        }
        // By default, all the space (old and new) is selected
        info.dsetDims = newDimensions;
        info.h5Space  = H5Dget_space(info.h5Dset->value()); // Needs to be refreshed after H5Dset_extent
        info.dsetByte = h5pp::hdf5::getBytesTotal(info.h5Dset.value(), info.h5Space, info.h5Type);
        info.dsetSize = h5pp::hdf5::getSize(info.h5Space.value());
        h5pp::logger::log->debug("Resized dataset \n \t old: {} \n \t new: {}", oldInfoStr, info.string());
    }

    inline void resizeDataset(DsetInfo &dsetInfo, const DataInfo &dataInfo) {
        // We use this function to RESIZE the dataset to FIT given data
        dataInfo.assertWriteReady();
        resizeDataset(dsetInfo, dataInfo.dataDims.value());
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
            if constexpr(h5pp::type::sfinae::is_text_v<DataType>)
                // Minus one: String resize allocates the null-terminator automatically, and bytes is the number of characters including null-terminator
                h5pp::util::resizeData(data, {static_cast<hsize_t>(bytes) - 1});
            else if constexpr(h5pp::type::sfinae::has_text_v<DataType> and h5pp::type::sfinae::is_iterable_v<DataType>) {
                // We have a container such as std::vector<std::string> here, and the dataset may have multiple string elements
                auto size = getSizeSelected(space);
                h5pp::util::resizeData(data, {static_cast<hsize_t>(size)});
                // In variable length arrays each string element is dynamically resized when read.
                // For fixed-size we can resize already.
                if(not H5Tis_variable_str(type)) {
                    auto fixedStringSize = H5Tget_size(type) - 1; // Subtract null terminator
                    for(auto &str : data) h5pp::util::resizeData(str, {static_cast<hsize_t>(fixedStringSize)});
                }
            } else {
                throw std::runtime_error(
                    h5pp::format("Could not resize given container for text data: Unrecognized type for text [{}]", h5pp::type::sfinae::type_name<DataType>()));
            }
        } else if(H5Sget_simple_extent_type(space) == H5S_SCALAR)
            h5pp::util::resizeData(data, {static_cast<hsize_t>(1)});
        else {
            int                  rank = H5Sget_simple_extent_ndims(space); // This will define the bounding box of the selected elements in the file space
            std::vector<hsize_t> extent(static_cast<size_t>(rank), 0);
            H5Sget_simple_extent_dims(space, extent.data(), nullptr);
            h5pp::util::resizeData(data, extent);
            if(bytes != h5pp::util::getBytesTotal(data))
                h5pp::logger::log->debug("Size mismatch after resize: data [{}] bytes | dset [{}] bytes ", h5pp::util::getBytesTotal(data), bytes);
        }
    }

    template<typename DataType>
    inline void resizeData(DataType &data, const DsetInfo &info) {
        resizeData(data, info.h5Space.value(), info.h5Type.value(), info.dsetByte.value());
    }
    template<typename DataType>
    inline void resizeData(DataType &data, const AttrInfo &info) {
        resizeData(data, info.h5Space.value(), info.h5Type.value(), info.attrByte.value());
    }

    inline std::string getSpaceString(const hid::h5s &space) {
        std::string msg;
        msg.append(h5pp::format(" | size {}", H5Sget_simple_extent_npoints(space)));
        int                  rank = H5Sget_simple_extent_ndims(space);
        std::vector<hsize_t> dims(static_cast<size_t>(rank), 0);
        H5Sget_simple_extent_dims(space, dims.data(), nullptr);
        msg.append(h5pp::format(" | rank {}", rank));
        msg.append(h5pp::format(" | dims {}", dims));
        if(H5Sget_select_type(space) == H5S_SEL_HYPERSLABS) {
            Hyperslab slab(space);
            msg.append(slab.string());
        }
        return msg;
    }

    inline void assertSpacesEqual(const hid::h5s &dataSpace, const hid::h5s &dsetSpace, const hid::h5t &h5Type) {
        if(H5Tis_variable_str(h5Type) or H5Tget_class(h5Type) == H5T_STRING) {
            // Strings are a special case, e.g. we can write multiple string elements into just one.
            // Also space is allocated on the fly during read by HDF5.. so size comparisons are useless here.
            return;
        }
        //        if(h5_layout == H5D_CHUNKED) return; // Chunked layouts are allowed to differ
        htri_t equal = H5Sextent_equal(dataSpace, dsetSpace);
        if(equal == 0) {
            H5S_sel_type dataSelType = H5Sget_select_type(dataSpace);
            H5S_sel_type dsetSelType = H5Sget_select_type(dsetSpace);
            if(dataSelType == H5S_sel_type::H5S_SEL_HYPERSLABS or dsetSelType == H5S_sel_type::H5S_SEL_HYPERSLABS) {
                auto dataSize = getSizeSelected(dataSpace);
                auto dsetSize = getSizeSelected(dsetSpace);
                if(dataSize != dsetSize) {
                    auto msg1 = getSpaceString(dataSpace);
                    auto msg2 = getSpaceString(dsetSpace);
                    throw std::runtime_error(h5pp::format("Spaces are not equal size \n\t data space \t {} \n\t dset space \t {}", msg1, msg2));
                }
            } else {
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
                    h5pp::logger::log->debug(" data space {}", getSpaceString(dataSpace));
                    h5pp::logger::log->debug(" dset space {}", getSpaceString(dsetSpace));
                }
            }

        } else if(equal < 0) {
            throw std::runtime_error("Failed to compare space extents");
        }
    }

    namespace internal {
        inline long        maxHits  = -1;
        inline long        maxDepth = -1;
        inline std::string searchKey;
        template<H5O_type_t ObjType, typename InfoType>
        inline herr_t matcher([[maybe_unused]] hid_t id, const char *name, [[maybe_unused]] const InfoType *info, void *opdata) {
            // If object type is the one requested, and name matches the search key, then add it to the match list (a vector<string>)
            // If the search depth is passed the depth specified, return immediately
            // Return 0 to continue searching
            // Return 1 to finish the search. Normally when we've reached max search hits.
            std::string_view nameView(name);
            if(maxDepth >= 0 and std::count(nameView.begin(), nameView.end(), '/') > maxDepth) return 0;
            auto matchList = reinterpret_cast<std::vector<std::string> *>(opdata);
            try {
                if constexpr(std::is_same_v<InfoType, H5O_info_t>) {
                    if(info->type == ObjType or ObjType == H5O_TYPE_UNKNOWN) {
                        if(searchKey.empty() or nameView.find(searchKey) != std::string::npos) matchList->push_back(name);
                    }
                }

                else if constexpr(std::is_same_v<InfoType, H5L_info_t>) {
                    H5O_info_t oInfo;
                    hid::h5o   obj_id = H5Oopen(id, name, H5P_DEFAULT);
                    /* clang-format off */
                    #if defined(H5Ovisit_vers)
                        #if H5Ovisit_vers == 1
                            H5Oget_info(obj_id, &oInfo);
                        #elif H5Ovisit_vers == 2
                            H5Oget_info2(obj_id, &oInfo, H5O_INFO_ALL);
                        #elif H5Ovisit_vers == 3
                            H5Oget_info3(obj_id, &oInfo, H5O_INFO_ALL);
                        #endif
                    #else
                        H5Oget_info(obj_id, &oInfo);
                    #endif
                    /* clang-format on */
                    if(oInfo.type == ObjType or ObjType == H5O_TYPE_UNKNOWN) {
                        if(searchKey.empty() or nameView.find(searchKey) != std::string::npos) matchList->push_back(name);
                    }
                } else {
                    if(searchKey.empty() or nameView.find(searchKey) != std::string::npos) { matchList->push_back(name); }
                }

                if(maxHits > 0 and static_cast<long>(matchList->size()) >= maxHits)
                    return 1;
                else
                    return 0;
            } catch(...) { throw std::logic_error(h5pp::format("Could not match object [{}] | loc_id [{}]", name, id)); }
        }

        template<H5O_type_t ObjType>
        inline constexpr std::string_view getObjTypeName() {
            if constexpr(ObjType == H5O_type_t::H5O_TYPE_DATASET)
                return "dataset";
            else if constexpr(ObjType == H5O_type_t::H5O_TYPE_GROUP)
                return "group";
            else if constexpr(ObjType == H5O_type_t::H5O_TYPE_UNKNOWN)
                return "unknown";
            else if constexpr(ObjType == H5O_type_t::H5O_TYPE_NAMED_DATATYPE)
                return "named datatype";
            else if constexpr(ObjType == H5O_type_t::H5O_TYPE_NTYPES)
                return "ntypes";
            else
                return "map"; // Only in HDF5 v 1.12
        }
        template<H5O_type_t ObjType, typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
        inline herr_t visit_by_name(const h5x &loc, std::string_view root, std::vector<std::string> &matchList, const hid::h5p &linkAccess = H5P_DEFAULT) {
            if(internal::maxDepth == 0)
                // Faster when we don't need to iterate recursively
                return H5Literate_by_name(loc, util::safe_str(root).c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, nullptr, internal::matcher<ObjType>, &matchList, linkAccess);
#if defined(H5Ovisit_by_name3) || (defined(H5Ovisit_by_name_vers) && H5Ovisit_by_name_vers == 3)
            return H5Ovisit_by_name3(loc, util::safe_str(root).c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, internal::matcher<ObjType>, &matchList, H5O_INFO_ALL, linkAccess);
#elif defined(H5Ovisit_by_name2) || (defined(H5Ovisit_by_name_vers) && H5Ovisit_by_name_vers == 2)
            return H5Ovisit_by_name2(loc, util::safe_str(root).c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, internal::matcher<ObjType>, &matchList, H5O_INFO_ALL, linkAccess);
#elif defined(H5Ovisit_by_name1) || (defined(H5Ovisit_by_name_vers) && H5Ovisit_by_name_vers == 1)
            return H5Ovisit_by_name1(loc, util::safe_str(root).c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, internal::matcher<ObjType>, &matchList, linkAccess);
#else
            return H5Ovisit_by_name(loc, util::safe_str(root).c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, internal::matcher<ObjType>, &matchList, linkAccess);
#endif
        }

    }

    template<H5O_type_t ObjType, typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline std::vector<std::string> findLinks(const h5x &      loc,
                                              std::string_view searchKey  = "",
                                              std::string_view searchRoot = "/",
                                              long             maxHits    = -1,
                                              long             maxDepth   = -1,
                                              const hid::h5p & linkAccess = H5P_DEFAULT) {
        h5pp::logger::log->trace("Search key: {} | target type: {} | search root: {} | max search hits {}", searchKey, internal::getObjTypeName<ObjType>(), searchRoot, maxHits);
        std::vector<std::string> matchList;
        internal::maxHits   = maxHits;
        internal::maxDepth  = maxDepth;
        internal::searchKey = searchKey;
        herr_t err          = internal::visit_by_name<ObjType>(loc, searchRoot, matchList, linkAccess);
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to find links of type [{}] while iterating from root [{}]", internal::getObjTypeName<ObjType>(), searchRoot));
        }
        return matchList;
    }

    template<H5O_type_t ObjType, typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline std::vector<std::string> getContentsOfLink(const h5x &loc, std::string_view linkPath, long maxDepth = 1, const hid::h5p &linkAccess = H5P_DEFAULT) {
        std::vector<std::string> contents;
        internal::maxHits  = -1;
        internal::maxDepth = maxDepth;
        internal::searchKey.clear();
        herr_t err = internal::visit_by_name<ObjType>(loc, linkPath, contents, linkAccess);
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to iterate link [{}] of type [{}]", linkPath, internal::getObjTypeName<ObjType>()));
        }
        return contents;
    }

    inline void createDataset(h5pp::DsetInfo &dsetInfo, const PropertyLists &plists = PropertyLists()) {
        // Here we create, the dataset id and set its properties before writing data to it.
        dsetInfo.assertCreateReady();
        if(dsetInfo.dsetExists and dsetInfo.dsetExists.value()) {
            h5pp::logger::log->trace("No need to create dataset [{}]: exists already", dsetInfo.dsetPath.value());
            return;
        }
        h5pp::logger::log->debug("Creating dataset {}", dsetInfo.string());
        hid_t dsetId = H5Dcreate(dsetInfo.getLocId(),
                                 util::safe_str(dsetInfo.dsetPath.value()).c_str(),
                                 dsetInfo.h5Type.value(),
                                 dsetInfo.h5Space.value(),
                                 plists.linkCreate,
                                 dsetInfo.h5PlistDsetCreate.value(),
                                 dsetInfo.h5PlistDsetAccess.value());
        if(dsetId <= 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to create dataset {}", dsetInfo.string()));
        }
        dsetInfo.h5Dset     = dsetId;
        dsetInfo.dsetExists = true;
    }

    inline void createAttribute(AttrInfo &attrInfo) {
        // Here we create, or register, the attribute id and set its properties before writing data to it.
        attrInfo.assertCreateReady();
        if(attrInfo.attrExists and attrInfo.attrExists.value()) {
            h5pp::logger::log->trace("No need to create attribute [{}] in link [{}]: exists already", attrInfo.attrName.value(), attrInfo.linkPath.value());
            return;
        }
        h5pp::logger::log->trace("Creating attribute {}", attrInfo.string());
        hid_t attrId = H5Acreate(attrInfo.h5Link.value(),
                                 util::safe_str(attrInfo.attrName.value()).c_str(),
                                 attrInfo.h5Type.value(),
                                 attrInfo.h5Space.value(),
                                 attrInfo.h5PlistAttrCreate.value(),
                                 attrInfo.h5PlistAttrAccess.value());
        if(attrId <= 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to create attribute [{}] for link [{}]", attrInfo.attrName.value(), attrInfo.linkPath.value()));
        }
        attrInfo.h5Attr     = attrId;
        attrInfo.attrExists = true;
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
        else
            throw std::runtime_error(h5pp::format("Failed to get char pointer of datatype [{}]", h5pp::type::sfinae::type_name<DataType>()));
        return sv;
    }

    template<typename DataType>
    void writeDataset(const DataType &data, const DataInfo &dataInfo, const DsetInfo &dsetInfo, const PropertyLists &plists = PropertyLists()) {
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_colmajor_v<DataType> and not h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            h5pp::logger::log->debug("Converting data to row-major storage order");
            const auto tempRowm = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::writeDataset(tempRowm, dataInfo, dsetInfo, plists);
            return;
        }
#endif
        dsetInfo.assertWriteReady();
        dataInfo.assertWriteReady();
        if(dataInfo.dataSlab) selectHyperslab(dataInfo.h5Space.value(), dataInfo.dataSlab.value());
        if(dsetInfo.dsetSlab) selectHyperslab(dsetInfo.h5Space.value(), dsetInfo.dsetSlab.value());
        h5pp::logger::log->debug("Writing from memory  {}", dataInfo.string());
        h5pp::logger::log->debug("Writing into dataset {}", dsetInfo.string());
        h5pp::hdf5::assertWriteBufferIsLargeEnough(data, dataInfo.h5Space.value(), dsetInfo.h5Type.value());
        h5pp::hdf5::assertBytesPerElemMatch<DataType>(dsetInfo.h5Type.value());
        h5pp::hdf5::assertSpacesEqual(dataInfo.h5Space.value(), dsetInfo.h5Space.value(), dsetInfo.h5Type.value());
        herr_t      retval  = 0;
        const void *dataPtr = nullptr;
        if constexpr(h5pp::type::sfinae::has_data_v<DataType>)
            dataPtr = data.data();
        else if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>)
            dataPtr = data;
        else
            dataPtr = &data;
        if constexpr(h5pp::type::sfinae::is_text_v<DataType> or h5pp::type::sfinae::has_text_v<DataType>) {
            auto vec = getCharPtrVector(data);
            // When H5T_VARIABLE, this function expects [const char **], which is what we get from vec.data()
            if(H5Tis_variable_str(dsetInfo.h5Type->value()) > 0)
                retval = H5Dwrite(dsetInfo.h5Dset.value(), dsetInfo.h5Type.value(), dataInfo.h5Space.value(), dsetInfo.h5Space.value(), plists.dsetXfer, vec.data());
            else {
                if(vec.size() == 1) {
                    retval = H5Dwrite(dsetInfo.h5Dset.value(), dsetInfo.h5Type.value(), dataInfo.h5Space.value(), dsetInfo.h5Space.value(), plists.dsetXfer, *vec.data());
                } else {
                    if constexpr(h5pp::type::sfinae::has_text_v<DataType> and h5pp::type::sfinae::is_iterable_v<DataType>) {
                        // We have a fixed-size string array now. We have to copy the strings to a contiguous array
                        // vdata already contains the pointer to each string, and bytes should be the size of the whole array
                        // including null terminators. so
                        std::string strContiguous;
                        size_t      bytesPerStr = H5Tget_size(dsetInfo.h5Type.value()); // Includes null term
                        strContiguous.resize(bytesPerStr * vec.size());
                        for(size_t i = 0; i < vec.size(); i++) {
                            auto start_src = strContiguous.data() + static_cast<long>(i * bytesPerStr);
                            strncpy(start_src, vec[i], strnlen(vec[i], bytesPerStr - 1)); // Do not copy the null term
                        }
                        retval =
                            H5Dwrite(dsetInfo.h5Dset.value(), dsetInfo.h5Type.value(), dataInfo.h5Space.value(), dsetInfo.h5Space.value(), plists.dsetXfer, strContiguous.data());
                    } else {
                        // Assume contigous array and hope for the best
                        retval = H5Dwrite(dsetInfo.h5Dset.value(), dsetInfo.h5Type.value(), dataInfo.h5Space.value(), dsetInfo.h5Space.value(), plists.dsetXfer, dataPtr);
                    }
                }
            }
        } else
            retval = H5Dwrite(dsetInfo.h5Dset.value(), dsetInfo.h5Type.value(), dataInfo.h5Space.value(), dsetInfo.h5Space.value(), plists.dsetXfer, dataPtr);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to write into dataset \n\t {} \n from memory \n\t {}", dsetInfo.string(), dataInfo.string()));
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    void readDataset(DataType &data, const DataInfo &dataInfo, const DsetInfo &dsetInfo, const PropertyLists &plists = PropertyLists()) {
        // Transpose the data container before reading
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_colmajor_v<DataType> and not h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            h5pp::logger::log->debug("Converting data to row-major storage order");
            auto tempRowMajor = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::readDataset(tempRowMajor, dataInfo, dsetInfo, plists);
            data = eigen::to_ColMajor(tempRowMajor);
            return;
        }
#endif

        dsetInfo.assertReadReady();
        dataInfo.assertReadReady();
        if(dataInfo.dataSlab) selectHyperslab(dataInfo.h5Space.value(), dataInfo.dataSlab.value());
        if(dsetInfo.dsetSlab) selectHyperslab(dsetInfo.h5Space.value(), dsetInfo.dsetSlab.value());
        h5pp::logger::log->debug("Reading into memory  {}", dataInfo.string());
        h5pp::logger::log->debug("Reading from dataset {}", dsetInfo.string());
        h5pp::hdf5::assertReadBufferIsLargeEnough(data, dataInfo.h5Space.value(), dsetInfo.h5Type.value());
        h5pp::hdf5::assertSpacesEqual(dataInfo.h5Space.value(), dsetInfo.h5Space.value(), dsetInfo.h5Type.value());
        h5pp::hdf5::assertBytesPerElemMatch<DataType>(dsetInfo.h5Type.value());
        herr_t retval = 0;

        [[maybe_unused]] void *dataPtr = nullptr;
        if constexpr(h5pp::type::sfinae::has_data_v<DataType>)
            dataPtr = data.data();
        else if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>)
            dataPtr = data;
        else
            dataPtr = &data;

        // Read the data
        if constexpr(h5pp::type::sfinae::is_text_v<DataType> or h5pp::type::sfinae::has_text_v<DataType>) {
            // When H5T_VARIABLE,
            //      1) H5Dread expects [const char **], which is what we get from vdata.data().
            //      2) H5Dread allocates memory on each const char * which has to be reclaimed later.
            // Otherwise,
            //      1) H5Dread expects [char *], i.e. *vdata.data()
            //      2) Allocation on char * must be done before reading.

            if(H5Tis_variable_str(dsetInfo.h5Type.value())) {
                auto                size = H5Sget_select_npoints(dsetInfo.h5Space.value());
                std::vector<char *> vdata(static_cast<size_t>(size)); // Allocate pointers for "size" number of strings
                // HDF5 allocates space for each string in vdata
                retval = H5Dread(dsetInfo.h5Dset.value(), dsetInfo.h5Type.value(), H5S_ALL, dsetInfo.h5Space.value(), plists.dsetXfer, vdata.data());
                // Now vdata contains the whole dataset and we need to put the data into the user-given container.
                if constexpr(std::is_same_v<DataType, std::string>) {
                    // A vector of strings (vdata) can be put into a single string (data) with entries separated by new-lines
                    data.clear();
                    for(size_t i = 0; i < vdata.size(); i++) {
                        if(!vdata.empty() and vdata[i] != nullptr) data.append(vdata[i]);
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
                H5Dvlen_reclaim(dsetInfo.h5Type.value(), dsetInfo.h5Space.value(), plists.dsetXfer, vdata.data());
            } else {
                // All the elements in the dataset have the same string size
                // The whole dataset is read into a contiguous block of memory.
                size_t      bytesPerString = H5Tget_size(dsetInfo.h5Type.value()); // Includes null terminator
                auto        size           = H5Sget_select_npoints(dsetInfo.h5Space.value());
                std::string fdata;
                fdata.resize(static_cast<size_t>(size) * bytesPerString);
                retval = H5Dread(dsetInfo.h5Dset.value(), dsetInfo.h5Type.value(), dataInfo.h5Space.value(), dsetInfo.h5Space.value(), plists.dsetXfer, fdata.data());
                // Now fdata contains the whole dataset and we need to put the data into the user-given container.
                if constexpr(std::is_same_v<DataType, std::string>) {
                    // A vector of strings (fdata) can be put into a single string (data) with entries separated by new-lines
                    data.clear();
                    for(size_t i = 0; i < static_cast<size_t>(size); i++) {
                        data.append(fdata.substr(i * bytesPerString, bytesPerString));
                        if(data.size() < fdata.size() - 1) data.append("\n");
                    }
                    data.erase(std::find(data.begin(), data.end(), '\0'), data.end()); // Prune all but the last null terminator
                } else if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::string> and h5pp::type::sfinae::has_resize_v<DataType>) {
                    if(data.size() != static_cast<size_t>(size))
                        throw std::runtime_error(h5pp::format("Given container of strings has the wrong size: dset size {} | container size {}", size, data.size()));
                    for(size_t i = 0; i < static_cast<size_t>(size); i++) {
                        strncpy(data[i].data(), fdata.data() + i * bytesPerString, bytesPerString);
                        data[i].erase(std::find(data[i].begin(), data[i].end(), '\0'), data[i].end()); // Prune all but the last null terminator
                    }
                } else {
                    throw std::runtime_error("To read text-data, please use std::string or a container of std::string like std::vector<std::string>");
                }
            }
        } else
            retval = H5Dread(dsetInfo.h5Dset.value(), dsetInfo.h5Type.value(), dataInfo.h5Space.value(), dsetInfo.h5Space.value(), plists.dsetXfer, dataPtr);

        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to read from dataset \n\t {} \n into memory \n\t {}", dsetInfo.string(), dataInfo.string()));
        }
    }

    template<typename DataType>
    void writeAttribute(const DataType &data, const DataInfo &dataInfo, const AttrInfo &attrInfo) {
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_colmajor_v<DataType> and not h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            h5pp::logger::log->debug("Converting attribute data to row-major storage order");
            const auto tempRowm = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::writeAttribute(tempRowm, dataInfo, attrInfo);
            return;
        }
#endif
        dataInfo.assertWriteReady();
        attrInfo.assertWriteReady();
        if(dataInfo.dataSlab) selectHyperslab(dataInfo.h5Space.value(), dataInfo.dataSlab.value());
        if(attrInfo.attrSlab) selectHyperslab(attrInfo.h5Space.value(), attrInfo.attrSlab.value());
        h5pp::logger::log->debug("Writing from memory    {}", dataInfo.string());
        h5pp::logger::log->debug("Writing into attribute {}", attrInfo.string());
        h5pp::hdf5::assertWriteBufferIsLargeEnough(data, dataInfo.h5Space.value(), attrInfo.h5Type.value());
        h5pp::hdf5::assertBytesPerElemMatch<DataType>(attrInfo.h5Type.value());
        h5pp::hdf5::assertSpacesEqual(dataInfo.h5Space.value(), attrInfo.h5Space.value(), attrInfo.h5Type.value());
        herr_t                       retval  = 0;
        [[maybe_unused]] const void *dataPtr = nullptr;
        if constexpr(h5pp::type::sfinae::has_data_v<DataType>)
            dataPtr = data.data();
        else if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>)
            dataPtr = data;
        else
            dataPtr = &data;

        if constexpr(h5pp::type::sfinae::is_text_v<DataType> or h5pp::type::sfinae::has_text_v<DataType>) {
            auto vec = getCharPtrVector(data);
            if(H5Tis_variable_str(attrInfo.h5Type->value()) > 0)
                retval = H5Awrite(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), vec.data());
            else
                retval = H5Awrite(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), *vec.data());
        } else
            retval = H5Awrite(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), dataPtr);

        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to write into attribute \n\t {} \n from memory \n\t {}", attrInfo.string(), dataInfo.string()));
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    void readAttribute(DataType &data, const DataInfo &dataInfo, const AttrInfo &attrInfo) {
        // Transpose the data container before reading
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_colmajor_v<DataType> and not h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            h5pp::logger::log->debug("Converting data to row-major storage order");
            auto tempRowMajor = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::readAttribute(tempRowMajor, dataInfo, attrInfo);
            data = eigen::to_ColMajor(tempRowMajor);
            return;
        }
#endif
        dataInfo.assertReadReady();
        attrInfo.assertReadReady();
        h5pp::logger::log->debug("Reading into memory {}", dataInfo.string());
        h5pp::logger::log->debug("Reading from file   {}", attrInfo.string());
        h5pp::hdf5::assertReadBufferIsLargeEnough(data, dataInfo.h5Space.value(), attrInfo.h5Type.value());
        h5pp::hdf5::assertBytesPerElemMatch<DataType>(attrInfo.h5Type.value());
        h5pp::hdf5::assertSpacesEqual(dataInfo.h5Space.value(), attrInfo.h5Space.value(), attrInfo.h5Type.value());
        herr_t                 retval  = 0;
        [[maybe_unused]] void *dataPtr = nullptr;
        if constexpr(h5pp::type::sfinae::has_data_v<DataType>)
            dataPtr = data.data();
        else if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>)
            dataPtr = data;
        else
            dataPtr = &data;
        // Read the data
        if constexpr(h5pp::type::sfinae::is_text_v<DataType> or h5pp::type::sfinae::has_text_v<DataType>) {
            // When H5T_VARIABLE,
            //      1) H5Aread expects [const char **], which is what we get from vdata.data().
            //      2) H5Aread allocates memory on each const char * which has to be reclaimed later.
            // Otherwise,
            //      1) H5Aread expects [char *], i.e. *vdata.data()
            //      2) Allocation on char * must be done before reading.
            if(H5Tis_variable_str(attrInfo.h5Type.value()) > 0) {
                auto                size = H5Sget_select_npoints(attrInfo.h5Space.value());
                std::vector<char *> vdata(static_cast<size_t>(size)); // Allocate pointers for "size" number of strings
                // HDF5 allocates space for each string
                retval = H5Aread(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), vdata.data());
                // Now vdata contains the whole dataset and we need to put the data into the user-given container.
                if constexpr(std::is_same_v<DataType, std::string>) {
                    // A vector of strings (vdata) can be put into a single string (data) with entries separated by new-lines
                    data.clear();
                    for(size_t i = 0; i < vdata.size(); i++) {
                        if(!vdata.empty() and vdata[i] != nullptr) data.append(vdata[i]);
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
                H5Dvlen_reclaim(attrInfo.h5Type.value(), attrInfo.h5Space.value(), H5P_DEFAULT, vdata.data());
            } else {
                // All the elements in the dataset have the same string size
                // The whole dataset is read into a contiguous block of memory.
                size_t      bytesPerString = H5Tget_size(attrInfo.h5Type.value()); // Includes null terminator
                auto        size           = H5Sget_select_npoints(attrInfo.h5Space.value());
                std::string fdata;
                fdata.resize(static_cast<size_t>(size) * bytesPerString);
                retval = H5Aread(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), fdata.data());
                // Now fdata contains the whole dataset and we need to put the data into the user-given container.
                if constexpr(std::is_same_v<DataType, std::string>) {
                    // A vector of strings (fdata) can be put into a single string (data) with entries separated by new-lines
                    data.clear();
                    for(size_t i = 0; i < static_cast<size_t>(size); i++) {
                        data.append(fdata.substr(i * bytesPerString, bytesPerString));
                        if(data.size() < fdata.size() - 1) data.append("\n");
                    }
                } else if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::string> and h5pp::type::sfinae::has_resize_v<DataType>) {
                    data.clear();
                    data.resize(static_cast<size_t>(size));
                    for(size_t i = 0; i < static_cast<size_t>(size); i++) data[i] = fdata.substr(i * bytesPerString, bytesPerString);
                } else {
                    throw std::runtime_error("To read text-data, please use std::string or a container of std::string like std::vector<std::string>");
                }
            }
            if constexpr(std::is_same_v<DataType, std::string>) {
                data.erase(std::find(data.begin(), data.end(), '\0'), data.end()); // Prune all but the last null terminator
            }
        } else
            retval = H5Aread(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), dataPtr);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to read from attribute \n\t {} \n into memory \n\t {}", attrInfo.string(), dataInfo.string()));
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
        hid_t file = H5Fcreate(filePath.string().c_str(), H5F_ACC_TRUNC, plists.fileCreate, plists.fileAccess);
        if(file < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to create file [{}]", filePath.string()));
        }
        H5Fclose(file);
        return fs::canonical(filePath);
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline void createTable(const h5x &loc, TableInfo &info, const PropertyLists &plists = PropertyLists()) {
        info.assertCreateReady();
        h5pp::logger::log->debug("Creating table [{}] | num fields {} | record size {} bytes", info.tablePath.value(), info.numFields.value(), info.recordBytes.value());
        createGroup(loc, info.tableGroupName.value(), std::nullopt, plists);

        if(checkIfLinkExists(loc, info.tablePath.value(), info.tableExists, plists.linkAccess)) {
            h5pp::logger::log->debug("Table [{}] already exists", info.tablePath.value());
            return;
        }

        // Copy member type data to a vector of hid_t for compatibility.
        // Note that there is no need to close thes hid_t since info will close them.
        std::vector<hid_t> fieldTypesHidT(info.fieldTypes.value().begin(), info.fieldTypes.value().end());

        // Copy member name data to a vector of const char * for compatibility
        std::vector<const char *> fieldNames;
        for(auto &name : info.fieldNames.value()) fieldNames.push_back(name.c_str());
        int compression = info.compressionLevel.value() == 0 ? 0 : 1; // Only true/false (1/0). Is set to level 6 in HDF5 sources
        H5TBmake_table(util::safe_str(info.tableTitle.value()).c_str(),
                       loc,
                       util::safe_str(info.tablePath.value()).c_str(),
                       info.numFields.value(),
                       info.numRecords.value(),
                       info.recordBytes.value(),
                       fieldNames.data(),
                       info.fieldOffsets.value().data(),
                       fieldTypesHidT.data(),
                       info.chunkSize.value(),
                       nullptr,
                       compression,
                       nullptr);

        h5pp::logger::log->trace("Successfully created table [{}]", info.tablePath.value());
        info.tableExists = true;
        if constexpr(std::is_same_v<h5x, hid::h5f>) info.tableFile = loc;
        if constexpr(std::is_same_v<h5x, hid::h5g>) info.tableGroup = loc;
    }

    template<typename DataType>
    inline void appendTableEntries(const DataType &data, TableInfo &info) {
        size_t numNewRecords = h5pp::util::getSize(data);
        h5pp::logger::log->debug("Appending {} records to table [{}] | current num records {} | record size {} bytes",
                                 numNewRecords,
                                 info.tablePath.value(),
                                 info.numRecords.value(),
                                 info.recordBytes.value());
        info.assertWriteReady();

        // Make sure the given container and the registered table entry have the same size.
        // If there is a mismatch here it can cause horrible bugs/segfaults
        if constexpr(h5pp::type::sfinae::has_value_type_v<DataType>) {
            if(sizeof(typename DataType::value_type) != info.recordBytes.value())
                throw std::runtime_error(h5pp::format("Size mismatch: Given container of type {} has elements of {} bytes, but the table entries on file are {} bytes each ",
                                                      h5pp::type::sfinae::type_name<DataType>(),
                                                      sizeof(typename DataType::value_type),
                                                      info.recordBytes.value()));
        } else if constexpr(h5pp::type::sfinae::has_data_v<DataType> and h5pp::type::sfinae::is_iterable_v<DataType>) {
            if(sizeof(&data.data()) != info.recordBytes.value())
                throw std::runtime_error(h5pp::format("Size mismatch: Given container of type {} has elements of {} bytes, but the table entries on file are {} bytes each ",
                                                      h5pp::type::sfinae::type_name<DataType>(),
                                                      sizeof(&data.data()),
                                                      info.recordBytes.value()));
        } else {
            if(sizeof(DataType) != info.recordBytes.value())
                throw std::runtime_error(h5pp::format("Size mismatch: Given data type {} is of {} bytes, but the table entries on file are {} bytes each ",
                                                      h5pp::type::sfinae::type_name<DataType>(),
                                                      sizeof(DataType),
                                                      info.recordBytes.value()));
        }

        if constexpr(h5pp::type::sfinae::has_data_v<DataType>) {
            H5TBappend_records(info.getTableLocId(),
                               util::safe_str(info.tablePath.value()).c_str(),
                               numNewRecords,
                               info.recordBytes.value(),
                               info.fieldOffsets.value().data(),
                               info.fieldSizes.value().data(),
                               data.data());
        } else {
            H5TBappend_records(info.getTableLocId(),
                               util::safe_str(info.tablePath.value()).c_str(),
                               numNewRecords,
                               info.recordBytes.value(),
                               info.fieldOffsets.value().data(),
                               info.fieldSizes.value().data(),
                               &data);
        }
        info.numRecords.value() += numNewRecords;
    }

    inline void addTableEntriesFrom(const h5pp::TableInfo &srcInfo, h5pp::TableInfo &tgtInfo, hsize_t srcStartIdx, hsize_t tgtStartIdx, hsize_t numNewRecords) {
        srcInfo.assertReadReady();
        tgtInfo.assertWriteReady();
        // Sanity checks for table types
        if(srcInfo.tableType.value() != tgtInfo.tableType.value()) throw std::runtime_error(h5pp::format("Failed to add table entries: table type mismatch"));
        if(srcInfo.recordBytes.value() != tgtInfo.recordBytes.value())
            throw std::runtime_error(
                h5pp::format("Failed to add table entries: table entry byte size mismatch src {} != tgt {}", srcInfo.recordBytes.value(), tgtInfo.recordBytes.value()));
        if(srcInfo.fieldSizes.value() != tgtInfo.fieldSizes.value())
            throw std::runtime_error(
                h5pp::format("Failed to add table entries: table field sizes mismatch src {} != tgt {}", srcInfo.fieldSizes.value(), tgtInfo.fieldSizes.value()));
        if(srcInfo.fieldOffsets.value() != tgtInfo.fieldOffsets.value())
            throw std::runtime_error(
                h5pp::format("Failed to add table entries: table field offsets mismatch src {} != tgt {}", srcInfo.fieldOffsets.value(), tgtInfo.fieldOffsets.value()));

        // Sanity check for entry ranges
        if(srcInfo.numRecords.value() < srcStartIdx + numNewRecords)
            throw std::runtime_error(h5pp::format("Failed to add table entries: Requeted entries out of bound: src table nrecords {} | src table start entry {} | num entries {}",
                                                  srcInfo.numRecords.value(),
                                                  srcStartIdx,
                                                  numNewRecords));

        if(srcInfo.tableFile.value() == tgtInfo.tableFile.value()) {
            h5pp::logger::log->debug("adding records to table [{}] from table [{}] | src start entry {} | tgt start entry {} | num entries {} | entry size {}",
                                     srcInfo.tablePath.value(),
                                     tgtInfo.tablePath.value(),
                                     srcStartIdx,
                                     tgtStartIdx,
                                     numNewRecords,
                                     tgtInfo.recordBytes.value());
            H5TBadd_records_from(srcInfo.getTableLocId(),
                                 util::safe_str(srcInfo.tablePath.value()).c_str(),
                                 srcStartIdx,
                                 numNewRecords,
                                 util::safe_str(tgtInfo.tablePath.value()).c_str(),
                                 tgtStartIdx);

        } else {
            // If the locations are on different files we need to make a temporary
            h5pp::logger::log->debug("adding records to table [{}] from table [{}] on different file | src start entry {} | tgt start entry {} | num entries {} | entry size {}",
                                     srcInfo.tablePath.value(),
                                     tgtInfo.tablePath.value(),
                                     srcStartIdx,
                                     tgtStartIdx,
                                     numNewRecords,
                                     tgtInfo.recordBytes.value());
            std::vector<std::byte> data(numNewRecords * tgtInfo.recordBytes.value());
            H5TBread_records(srcInfo.getTableLocId(),
                             util::safe_str(srcInfo.tablePath.value()).c_str(),
                             srcStartIdx,
                             numNewRecords,
                             srcInfo.recordBytes.value(),
                             srcInfo.fieldOffsets.value().data(),
                             srcInfo.fieldSizes.value().data(),
                             data.data());

            H5TBappend_records(tgtInfo.getTableLocId(),
                               util::safe_str(tgtInfo.tablePath.value()).c_str(),
                               numNewRecords,
                               tgtInfo.recordBytes.value(),
                               tgtInfo.fieldOffsets.value().data(),
                               tgtInfo.fieldSizes.value().data(),
                               data.data());
        }

        tgtInfo.numRecords.value() += numNewRecords;
    }

    template<typename DataType>
    inline void readTableEntries(DataType &data, const TableInfo &info, std::optional<size_t> startIdx = std::nullopt, std::optional<size_t> numReadRecords = std::nullopt) {
        // If none of startIdx or numReadRecords are given:
        //          If data resizeable: startIdx = 0, numReadRecords = totalRecords
        //          If data not resizeable: startIdx = last entry, numReadRecords = 1.
        // If startIdx given but numReadRecords is not:
        //          If data resizeable -> read from startEntries to the end
        //          If data not resizeable -> read from startEntries a single entry
        // If numReadRecords given but startEntries is not -> read the last numReadRecords records
        info.assertReadReady();
        hsize_t totalRecords = info.numRecords.value();
        if(not startIdx and not numReadRecords) {
            if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) {
                startIdx       = 0;
                numReadRecords = totalRecords;

            } else {
                startIdx       = totalRecords - 1;
                numReadRecords = 1;
            }
        } else if(startIdx and not numReadRecords) {
            if(startIdx.value() > totalRecords - 1)
                throw std::runtime_error(
                    h5pp::format("Invalid start record for table [{}] | nrecords [{}] | start entry [{}]", info.tablePath.value(), totalRecords, startIdx.value()));
            if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) {
                numReadRecords = totalRecords - startIdx.value();
            } else {
                numReadRecords = 1;
            }

        } else if(numReadRecords and not startIdx) {
            if(numReadRecords and numReadRecords.value() > totalRecords)
                throw std::runtime_error(h5pp::format(
                    "Asked for more records than available in table [{}] | nrecords [{}] | num_entries [{}]", info.tablePath.value(), totalRecords, numReadRecords.value()));
            startIdx = totalRecords - numReadRecords.value();
        }

        // Sanity check
        if(numReadRecords.value() > totalRecords)
            throw std::logic_error(h5pp::format("Asked for more records than available in table [{}] | nrecords [{}]", info.tablePath.value(), totalRecords));
        if(startIdx.value() + numReadRecords.value() > totalRecords)
            throw std::logic_error(h5pp::format("Asked for more records than available in table [{}] | nrecords [{}]", info.tablePath.value(), totalRecords));

        h5pp::logger::log->debug("Reading table [{}] | read from record {} | read num records {} | available records {} | record size {} bytes",
                                 info.tablePath.value(),
                                 startIdx.value(),
                                 numReadRecords.value(),
                                 info.numRecords.value(),
                                 info.recordBytes.value());

        // Make sure the given container and the registered table entry have the same size.
        // If there is a mismatch here it can cause horrible bugs/segfaults
        size_t dataSize = 0;
        if constexpr(h5pp::type::sfinae::has_value_type_v<DataType>)
            dataSize = sizeof(typename DataType::value_type);
        else if constexpr(h5pp::type::sfinae::has_data_v<DataType> and h5pp::type::sfinae::is_iterable_v<DataType>)
            dataSize = sizeof(&data.data());
        else
            dataSize = sizeof(DataType);

        if(dataSize != info.recordBytes.value())
            throw std::runtime_error(h5pp::format("Could not read from table [{}]: "
                                                  "Size mismatch: "
                                                  "Given data container size is {} bytes per element | "
                                                  "Table is {} bytes per entry",
                                                  info.tablePath.value(),
                                                  dataSize,
                                                  info.recordBytes.value()));

        h5pp::util::resizeData(data, {numReadRecords.value()});
        void *dataPtr = nullptr;
        if constexpr(h5pp::type::sfinae::has_data_v<DataType>)
            dataPtr = data.data();
        else if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>)
            dataPtr = data;
        else
            dataPtr = &data;
        H5TBread_records(info.getTableLocId(),
                         util::safe_str(info.tablePath.value()).c_str(),
                         startIdx.value(),
                         numReadRecords.value(),
                         info.recordBytes.value(),
                         info.fieldOffsets.value().data(),
                         info.fieldSizes.value().data(),
                         dataPtr);
    }

    template<typename DataType>
    inline void readTableField(DataType &            data,
                               const TableInfo &     info,
                               std::string_view      fieldName,
                               std::optional<size_t> startIdx       = std::nullopt,
                               std::optional<size_t> numReadRecords = std::nullopt) {
        // If none of startIdx or numReadRecords are given:
        //          If data resizeable: startIdx = 0, numReadRecords = totalRecords
        //          If data not resizeable: startIdx = last entry, numReadRecords = 1.
        // If startIdx given but numReadRecords is not:
        //          If data resizeable -> read from startEntries to the end
        //          If data not resizeable -> read from startEntries a single entry
        // If numReadRecords given but startEntries is not -> read the last numReadRecords records

        hsize_t totalRecords = info.numRecords.value();
        if(not startIdx and not numReadRecords) {
            if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) {
                startIdx       = 0;
                numReadRecords = totalRecords;

            } else {
                startIdx       = totalRecords - 1;
                numReadRecords = 1;
            }
        } else if(startIdx and not numReadRecords) {
            if(startIdx.value() > totalRecords - 1)
                throw std::runtime_error(
                    h5pp::format("Invalid start record for table [{}] | nrecords [{}] | start entry [{}]", info.tablePath.value(), totalRecords, startIdx.value()));
            if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) {
                numReadRecords = totalRecords - startIdx.value();
            } else {
                numReadRecords = 1;
            }

        } else if(numReadRecords and not startIdx) {
            if(numReadRecords and numReadRecords.value() > totalRecords)
                throw std::runtime_error(h5pp::format(
                    "Asked for more records than available in table [{}] | nrecords [{}] | num_entries [{}]", info.tablePath.value(), totalRecords, numReadRecords.value()));
            startIdx = totalRecords - numReadRecords.value();
        }

        // Sanity check
        if(numReadRecords.value() > totalRecords)
            throw std::logic_error(h5pp::format("Asked for more records than available in table [{}] | nrecords [{}]", info.tablePath.value(), totalRecords));
        if(startIdx.value() + numReadRecords.value() > totalRecords)
            throw std::logic_error(h5pp::format("Asked for more records than available in table [{}] | nrecords [{}]", info.tablePath.value(), totalRecords));

        // Compute the field index
        std::optional<size_t> optFieldIdx;
        for(size_t idx = 0; idx < info.fieldNames->size(); idx++) {
            if(fieldName == info.fieldNames.value()[idx]) {
                optFieldIdx = idx;
                break;
            }
        }
        if(not optFieldIdx)
            throw std::runtime_error(h5pp::format("Could not read field [{}] from table [{}]: "
                                                  "Could not find field name [{}] | "
                                                  "Available field names are {}",
                                                  fieldName,
                                                  info.tablePath.value(),
                                                  fieldName,
                                                  info.fieldNames.value()));
        size_t fieldIdx    = optFieldIdx.value();
        size_t fieldSize   = info.fieldSizes.value()[fieldIdx];
        size_t fieldOffset = info.fieldOffsets.value()[fieldIdx];

        // Make sure the given container and the registerd table entry have the same size.
        // If there is a mismatch here it can cause horrible bugs/segfaults
        size_t dataSize = 0;
        if constexpr(h5pp::type::sfinae::has_value_type_v<DataType>)
            dataSize = sizeof(typename DataType::value_type);
        else if constexpr(h5pp::type::sfinae::has_data_v<DataType> and h5pp::type::sfinae::is_iterable_v<DataType>)
            dataSize = sizeof(&data.data());
        else
            dataSize = sizeof(DataType);

        if(dataSize != fieldSize)
            throw std::runtime_error(h5pp::format("Could not read field [{}] from table [{}]: "
                                                  "Size mismatch: "
                                                  "Given data container size is {} bytes per element | "
                                                  "Field size is {} bytes per element",
                                                  fieldName,
                                                  info.tablePath.value(),
                                                  dataSize,
                                                  fieldSize));

        h5pp::util::resizeData(data, {numReadRecords.value()});
        h5pp::logger::log->debug("Reading table [{}] | field [{}] | field index {} | field offset {} | field size {} bytes | read from record {} | read num records {} | available "
                                 "records {} | record size {} bytes",
                                 info.tablePath.value(),
                                 fieldName,
                                 fieldIdx,
                                 fieldOffset,
                                 fieldSize,
                                 startIdx.value(),
                                 numReadRecords.value(),
                                 info.numRecords.value(),
                                 info.recordBytes.value());
        void *dataPtr = nullptr;
        if constexpr(h5pp::type::sfinae::has_data_v<DataType>)
            dataPtr = data.data();
        else if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>)
            dataPtr = data;
        else
            dataPtr = &data;
        H5TBread_fields_name(info.getTableLocId(),
                             util::safe_str(info.tablePath.value()).c_str(),
                             util::safe_str(fieldName).c_str(),
                             startIdx.value(),
                             numReadRecords.value(),
                             fieldSize,
                             info.fieldOffsets.value().data(),
                             info.fieldSizes.value().data(),
                             dataPtr);
    }

    template<typename h5x_src, typename h5x_tgt, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x_src>, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x_tgt>>
    inline void
        copyLink(const h5x_src &srcLocId, const std::string &srcLinkPath, const h5x_tgt &tgtLocId, const std::string &tgtLinkPath, const PropertyLists &plists = PropertyLists()) {
        h5pp::logger::log->trace("Copying link [{}] --> [{}]", srcLinkPath, tgtLinkPath);
        // Copy the link srcLinkPath to tgtLinkPath. Note that H5Ocopy does this recursively, so we don't need
        // to iterate links recursively here.
        auto retval = H5Ocopy(srcLocId, srcLinkPath.c_str(), tgtLocId, tgtLinkPath.c_str(), H5P_DEFAULT, plists.linkCreate);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Could not copy link [{}] --> [{}]", srcLinkPath, tgtLinkPath));
        }
    }

    inline void copyLink(const std::string &  srcFilePath,
                         const std::string &  srcLinkPath,
                         const std::string &  tgtFilePath,
                         const std::string &  tgtLinkPath,
                         FilePermission       targetFileCreatePermission = FilePermission::READWRITE,
                         const PropertyLists &plists                     = PropertyLists()) {
        h5pp::logger::log->trace("Copying link: source link [{}] | source file [{}]  -->  target link [{}] | target file [{}]", srcLinkPath, srcFilePath, tgtLinkPath, tgtFilePath);

        try {
            auto srcPath = fs::absolute(srcFilePath);
            if(not fs::exists(srcPath))
                throw std::runtime_error(h5pp::format("Could not copy link [{}] from file [{}]: source file does not exist [{}]", srcLinkPath, srcFilePath, srcPath.string()));
            auto tgtPath = h5pp::hdf5::createFile(tgtFilePath, targetFileCreatePermission, plists);

            hid_t hidSrc = H5Fopen(srcPath.string().c_str(), H5F_ACC_RDONLY, plists.fileAccess);
            hid_t hidTgt = H5Fopen(tgtPath.string().c_str(), H5F_ACC_RDWR, plists.fileAccess);
            if(hidSrc < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to open source file [{}] in read-only mode", srcPath.string()));
            }
            if(hidTgt < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to open target file [{}] in read-write mode", tgtPath.string()));
            }
            hid::h5f srcFile = hidSrc;
            hid::h5f tgtFile = hidTgt;
            copyLink(srcFile, srcLinkPath, tgtFile, tgtLinkPath);
        } catch(const std::exception &ex) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Could not copy link [{}] from file [{}]: {}", srcLinkPath, srcFilePath, ex.what()));
        }
    }

    inline fs::path
        copyFile(const std::string &src, const std::string &tgt, FilePermission permission = FilePermission::COLLISION_FAIL, const PropertyLists &plists = PropertyLists()) {
        h5pp::logger::log->trace("Copying file [{}] --> [{}]", src, tgt);
        auto tgtPath = h5pp::hdf5::createFile(tgt, permission, plists);
        auto srcPath = fs::absolute(src);
        try {
            if(not fs::exists(srcPath)) throw std::runtime_error(h5pp::format("Could not copy file [{}] --> [{}]: source file does not exist [{}]", src, tgt, srcPath.string()));
            if(tgtPath == srcPath) h5pp::logger::log->debug("Skipped copying file: source and target files have the same path [{}]", srcPath.string());

            hid_t hidSrc = H5Fopen(srcPath.string().c_str(), H5F_ACC_RDONLY, plists.fileAccess);
            hid_t hidTgt = H5Fopen(tgtPath.string().c_str(), H5F_ACC_RDWR, plists.fileAccess);
            if(hidSrc < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to open source file [{}] in read-only mode", srcPath.string()));
            }
            if(hidTgt < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to open target file [{}] in read-write mode", tgtPath.string()));
            }
            hid::h5f srcFile = hidSrc;
            hid::h5f tgtFile = hidTgt;

            // Copy all the groups in the file root recursively. Note that H5Ocopy does this recursively, so we don't need
            // to iterate links recursively here. Therefore maxDepth = 0
            long maxDepth = 0;
            for(const auto &link : getContentsOfLink<H5O_TYPE_UNKNOWN>(srcFile, "/", maxDepth, plists.linkAccess)) {
                if(link == ".") continue;
                h5pp::logger::log->trace("Copying recursively: [{}]", link);
                auto retval = H5Ocopy(srcFile, link.c_str(), tgtFile, link.c_str(), H5P_DEFAULT, plists.linkCreate);
                if(retval < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error(h5pp::format("Failed to copy file contents with H5Ocopy(srcFile,{},tgtFile,{},H5P_DEFAULT,link_create_propery_list)", link, link));
                }
            }
            // ... Find out how to copy attributes that are written on the root itself
            return tgtPath;
        } catch(const std::exception &ex) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Could not copy file [{}] --> [{}]: ", src, tgt, ex.what()));
        }
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
