#pragma once
#include "h5ppDebug.h"
#include "h5ppEigen.h"
#include "h5ppEnums.h"
#include "h5ppError.h"
#include "h5ppFilesystem.h"
#include "h5ppHyperslab.h"
#include "h5ppInfo.h"
#include "h5ppLogger.h"
#include "h5ppPropertyLists.h"
#include "h5ppTypeCast.h"
#include "h5ppTypeSfinae.h"
#include "h5ppUtils.h"
#include <cstddef>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <typeindex>
#include <utility>

#if !defined(H5PP_HAS_ZLIB_H)
    #if H5_HAVE_ZLIB_H == 1 && __has_include(<zlib.h>)
        #define H5PP_HAS_ZLIB_H 1
        #include <zlib.h>
    #else
        #define H5PP_HAS_ZLIB_H 0
    #endif
#endif

#ifndef H5PP_HAS_FILTER_DEFLATE
    #if H5PP_HAS_ZLIB_H == 1
        #define H5PP_HAS_FILTER_DEFLATE H5_HAVE_FILTER_DEFLATE
    #else
        #define H5PP_HAS_FILTER_DEFLATE 0
    #endif
#endif

#if H5_VERSION_GE(1, 10, 5)
    #define H5PP_HAS_DIRECT_CHUNK 1
#else
    #define H5PP_HAS_DIRECT_CHUNK 0
#endif

namespace h5pp {
    inline constexpr bool has_filter_deflate = H5PP_HAS_FILTER_DEFLATE;
    inline constexpr bool has_zlib_h         = H5PP_HAS_ZLIB_H;
    inline constexpr bool has_direct_chunk   = H5PP_HAS_DIRECT_CHUNK;
#if H5PP_HAS_DIRECT_CHUNK == 1
    inline bool use_direct_chunk = false;
#else
    inline constexpr bool use_direct_chunk = false;
#endif

}

/*!
 * \brief A collection of functions to create (or get information about) datasets and attributes in HDF5 files
 */
namespace h5pp::hdf5 {

    [[nodiscard]] inline std::vector<std::string_view> pathCumulativeSplit(std::string_view path, std::string_view delim) {
        // Here the resulting vector "output" will contain increasingly longer string_views, that are subsets of the path.
        // Note that no string data is allocated here, these are simply views into a string allocated elsewhere.
        // For example if path is "this/is/a/long/path, the vector will contain the following views
        // [0]: this
        // [1]: this/is
        // [2]: this/is/a
        // [3]: this/is/a/long
        // [4]: this/is/a/long/path

        // It is very important to note that the resulting views are not null terminated. Therefore, these vector elements
        // **must not** be used as c-style arrays using their .data() member functions.
        std::vector<std::string_view> output;
        size_t                        currentPosition = 0;
        while(currentPosition < path.size()) {
            const auto foundPosition = path.find_first_of(delim, currentPosition);
            if(currentPosition != foundPosition) output.emplace_back(path.substr(0, foundPosition));
            if(foundPosition == std::string_view::npos) break;
            currentPosition = foundPosition + 1;
        }
        return output;
    }

    template<typename h5x>
    [[nodiscard]] std::string getName(const h5x &object) {
        static_assert(type::sfinae::is_hdf5_id<h5x>);
        // Read about the buffer size inconsistency here
        // http://hdf-forum.184993.n3.nabble.com/H5Iget-name-inconsistency-td193143.html
        std::string buf;
        ssize_t     bufSize = H5Iget_name(object, nullptr, 0);                     // Size in bytes of the object name (NOT including \0)
        if(bufSize > 0) {
            buf.resize(type::safe_cast<size_t>(bufSize) + 1);                      // We allocate space for the null terminator with +1
            H5Iget_name(object, buf.data(), type::safe_cast<size_t>(bufSize + 1)); // Read name including \0 with +1
        }
        return buf.c_str(); // Use .c_str() to convert to a "standard" std::string, i.e. one where .size() does not include \0
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

    [[nodiscard]] inline hsize_t getSize(const hid::h5s &space) { return type::safe_cast<hsize_t>(H5Sget_simple_extent_npoints(space)); }

    [[nodiscard]] inline hsize_t getSize(const hid::h5d &dataset) {
        hid::h5s space = H5Dget_space(dataset);
        return getSize(space);
    }

    [[nodiscard]] inline hsize_t getSize(const hid::h5a &attribute) {
        hid::h5s space = H5Aget_space(attribute);
        return getSize(space);
    }

    [[nodiscard]] inline hsize_t getSizeSelected(const hid::h5s &space) {
        hssize_t size = H5Sget_select_npoints(space);
        if(size < 0) h5pp::runtime_error("getSizeSelected: H5Sget_select_npoints failed");
        return type::safe_cast<hsize_t>(size);
    }

    [[nodiscard]] inline std::vector<hsize_t> getDimensions(const hid::h5s &space) {
        int ndims = H5Sget_simple_extent_ndims(space);
        if(ndims < 0) throw h5pp::runtime_error("Failed to get dimensions");
        std::vector<hsize_t> dims(type::safe_cast<size_t>(ndims));
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

    [[nodiscard]] inline H5D_layout_t getLayout(const hid::h5p &dataset_creation_property_list) {
        return H5Pget_layout(dataset_creation_property_list);
    }

    [[nodiscard]] inline H5D_layout_t getLayout(const hid::h5d &dataset) {
        hid::h5p dcpl = H5Dget_create_plist(dataset);
        return H5Pget_layout(dcpl);
    }

    [[nodiscard]] inline std::optional<std::vector<hsize_t>> getChunkDimensions(const hid::h5p &dsetCreatePropertyList) {
        auto layout = H5Pget_layout(dsetCreatePropertyList);
        if(layout != H5D_CHUNKED) return std::nullopt;
        auto ndims = H5Pget_chunk(dsetCreatePropertyList, 0, nullptr);
        if(ndims < 0) throw h5pp::runtime_error("Failed to get chunk dimensions");
        if(ndims > 0) {
            std::vector<hsize_t> chunkDims(type::safe_cast<size_t>(ndims));
            H5Pget_chunk(dsetCreatePropertyList, ndims, chunkDims.data());
            return chunkDims;
        } else {
            return std::nullopt;
        }
    }

    [[nodiscard]] inline std::optional<std::vector<hsize_t>> getChunkDimensions(const hid::h5d &dataset) {
        hid::h5p dcpl = H5Dget_create_plist(dataset);
        return getChunkDimensions(dcpl);
    }

    [[nodiscard]] inline H5Z_filter_t getFilters(hid_t dcpl /* dataset creation property list */) {
        auto         nfilter = H5Pget_nfilters(dcpl);
        H5Z_filter_t filter  = H5Z_FILTER_NONE;
        for(int idx = 0; idx < nfilter; idx++) filter |= H5Pget_filter(dcpl, idx, nullptr, nullptr, nullptr, 0, nullptr, nullptr);
        return filter;
    }

    [[nodiscard]] inline int getDeflateLevel(hid_t dcpl /* dataset creation property list */) {
        auto filters = getFilters(dcpl);
        if((filters & H5Z_FILTER_DEFLATE) == H5Z_FILTER_DEFLATE) {
            std::array<unsigned int, 1> cd_values = {0};
            size_t                      cd_nelmts = cd_values.size();
            H5Pget_filter_by_id(dcpl, H5Z_FILTER_DEFLATE, nullptr, &cd_nelmts, cd_values.data(), 0, nullptr, nullptr);
            return type::safe_cast<int>(cd_values[0]);
        } else {
            return -1; // No deflate
        }
    }

    [[nodiscard]] inline std::optional<std::vector<hsize_t>> getMaxDimensions(const hid::h5s &space, H5D_layout_t layout) {
        if(layout != H5D_CHUNKED) return std::nullopt;
        if(H5Sget_simple_extent_type(space) != H5S_SIMPLE) return std::nullopt;
        int rank = H5Sget_simple_extent_ndims(space);
        if(rank < 0) throw h5pp::runtime_error("Failed to get dimensions");
        std::vector<hsize_t> dimsMax(type::safe_cast<size_t>(rank));
        H5Sget_simple_extent_dims(space, nullptr, dimsMax.data());
        return dimsMax;
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
        std::vector<const char *> vdata{type::safe_cast<size_t>(size)}; // Allocate for pointers for "size" number of strings
        // HDF5 allocates space for each string
        herr_t                    retval = H5Dread(dset, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, vdata.data());
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            return 0;
        }
        // Sum up the number of bytes
        size_t maxLen = h5pp::constants::maxSizeCompact;
        for(auto elem : vdata) {
            if(elem == nullptr) continue;
            *vlen += type::safe_cast<hsize_t>(std::min(std::string_view(elem).size(), maxLen) + 1); // Add null-terminator
        }
        H5Dvlen_reclaim(type, space, H5P_DEFAULT, vdata.data());
        return 1;
    }

    inline herr_t H5Avlen_get_buf_size_safe(const hid::h5a &attr, const hid::h5t &type, const hid::h5s &space, hsize_t *vlen) {
        *vlen = 0;
        if(H5Tis_variable_str(type) <= 0) return -1;
        if(H5Aget_storage_size(attr) <= 0) return 0;

        auto                      size = H5Sget_simple_extent_npoints(space);
        std::vector<const char *> vdata{type::safe_cast<size_t>(size)}; // Allocate pointers for "size" number of strings
        // HDF5 allocates space for each string
        herr_t                    retval = H5Aread(attr, type, vdata.data());
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            return 0;
        }
        // Sum up the number of bytes
        size_t maxLen = h5pp::constants::maxSizeCompact;
        for(auto elem : vdata) {
            if(elem == nullptr) continue;
            *vlen += type::safe_cast<hsize_t>(std::min(std::string_view(elem).size(), maxLen) + 1); // Add null-terminator
        }
        H5Dvlen_reclaim(type, space, H5P_DEFAULT, vdata.data());
        return 1;
    }

    [[nodiscard]] inline size_t getBytesPerElem(const hid::h5t &h5Type) { return H5Tget_size(h5Type); }

    [[nodiscard]] inline size_t getBytesTotal(const hid::h5s &space, const hid::h5t &type) {
        return getBytesPerElem(type) * getSize(space);
    }

    [[nodiscard]] inline size_t getBytesTotal(const hid::h5d &dset, std::optional<hid::h5s> space, std::optional<hid::h5t> type) {
        if(not type) type = H5Dget_type(dset);
        if(not space) space = H5Dget_space(dset);
        if(H5Tis_variable_str(type.value()) > 0) {
            hsize_t vlen = 0;
            herr_t  err  = H5Dvlen_get_buf_size_safe(dset, type.value(), space.value(), &vlen);
            if(err >= 0) return vlen; // Returns the total number of bytes required to store the dataset
            else return getBytesTotal(space.value(), type.value());
        }
        return getBytesTotal(space.value(), type.value());
    }

    [[nodiscard]] inline size_t getBytesTotal(const hid::h5a &attr, std::optional<hid::h5s> space, std::optional<hid::h5t> type) {
        if(not type) type = H5Aget_type(attr);
        if(not space) space = H5Aget_space(attr);
        if(H5Tis_variable_str(type.value()) > 0) {
            hsize_t vlen = 0;
            herr_t  err  = H5Avlen_get_buf_size_safe(attr, type.value(), space.value(), &vlen);
            if(err >= 0) return vlen; // Returns the total number of bytes required to store the dataset
            else return getBytesTotal(space.value(), type.value());
        }
        return getBytesTotal(space.value(), type.value());
    }

    [[nodiscard]] inline size_t getBytesSelected(const hid::h5s &space, const hid::h5t &type) {
        return getBytesPerElem(type) * getSizeSelected(space);
    }

    inline H5TInfo getH5TInfo(const hid::h5t &type) {
        H5TInfo info;
        info.h5Type   = type;
        hid_t h5Type  = type.value(); // Used a lot, store here to avoid validity checks
        info.typeSize = H5Tget_size(h5Type);
        info.h5Class  = H5Tget_class(h5Type);
        if(info.h5Class != H5T_class_t::H5T_COMPOUND) return info; // Can't do further analysis on its members

        info.numMembers   = H5Tget_nmembers(h5Type);
        auto nmemb        = type::safe_cast<size_t>(info.numMembers.value());
        info.memberNames  = std::vector<std::string>(nmemb);
        info.memberTypes  = std::vector<hid::h5t>(nmemb);
        info.memberSizes  = std::vector<size_t>(nmemb);
        info.memberIndex  = std::vector<int>(nmemb);
        info.memberOffset = std::vector<size_t>(nmemb);
        info.memberClass  = std::vector<H5T_class_t>(nmemb);
        if(info.h5Class.value() == H5T_COMPOUND) {
            for(size_t idx = 0; idx < nmemb; idx++) {
                char *name                         = H5Tget_member_name(h5Type, type::safe_cast<unsigned int>(idx));
                info.memberNames->operator[](idx)  = name;
                info.memberTypes->operator[](idx)  = H5Tget_member_type(h5Type, type::safe_cast<unsigned int>(idx));
                info.memberSizes->operator[](idx)  = H5Tget_size(info.memberTypes->operator[](idx));
                info.memberIndex->operator[](idx)  = H5Tget_member_index(h5Type, name);
                info.memberOffset->operator[](idx) = H5Tget_member_offset(h5Type, type::safe_cast<unsigned int>(idx));
                info.memberClass->operator[](idx)  = H5Tget_member_class(h5Type, type::safe_cast<unsigned int>(idx));
                H5free_memory(name);
            }
        }

        return info;
    }

    template<typename DataType>
    void assertWriteBufferIsLargeEnough(const DataType &data, const hid::h5s &space, const hid::h5t &type) {
        if(H5Tget_class(type) == H5T_STRING) {
            if(H5Tis_variable_str(type)) return;   // This transfers the string from memory until finding a null terminator
            if constexpr(type::sfinae::is_text_v<DataType>) {
                auto hdf5Byte = H5Tget_size(type); // Chars including null-terminator. The memory buffer must fit this size. Also, these
                                                   // many bytes will participate in IO
                auto hdf5Size = getSizeSelected(space);
                auto dataByte = h5pp::util::getCharArraySize(data, false); // Allocated chars including null terminator
                auto dataSize = h5pp::util::getSize(data);
                if(dataByte < hdf5Byte) {
                    throw h5pp::runtime_error(
                        "The text buffer given for this write operation is smaller than the selected space in memory.\n"
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
                        type::sfinae::type_name<DataType>());
                }
            }
        } else {
            if constexpr(std::is_pointer_v<DataType>) return;
            if constexpr(not type::sfinae::has_size_v<DataType>) return;
            auto hdf5Size = getSizeSelected(space);
            auto hdf5Byte = h5pp::util::getBytesPerElem<DataType>() * hdf5Size;
            auto dataByte = h5pp::util::getBytesTotal(data);
            auto dataSize = h5pp::util::getSize(data);
            if(dataByte < hdf5Byte) {
                throw h5pp::runtime_error("The buffer given for this write operation is smaller than the selected space in memory.\n"
                                          "\t Data transfer would read from memory out of bounds\n"
                                          "\t given   : size {} | bytes {}\n"
                                          "\t selected: size {} | bytes {}\n"
                                          "\t type     : [{}]",
                                          dataSize,
                                          dataByte,
                                          hdf5Size,
                                          hdf5Byte,
                                          type::sfinae::type_name<DataType>());
            }
        }
    }

    template<typename DataType>
    void assertReadSpaceIsLargeEnough(const DataType &data, const hid::h5s &space, const hid::h5t &type) {
        if(H5Tget_class(type) == H5T_STRING) {
            if(H5Tis_variable_str(type)) return; // These are resized on the fly
            if constexpr(type::sfinae::is_text_v<DataType>) {
                // The memory buffer must fit hdf5Byte: that's how many bytes will participate in IO
                auto hdf5Byte = H5Tget_size(type);                      // Chars including null-terminator.
                auto hdf5Size = getSizeSelected(space);
                auto dataByte = h5pp::util::getCharArraySize(data) + 1; // Chars including null terminator
                auto dataSize = h5pp::util::getSize(data);
                if(dataByte < hdf5Byte) {
                    throw h5pp::runtime_error(
                        "The text buffer allocated for this read operation is smaller than the string buffer found on the dataset.\n"
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
                        type::sfinae::type_name<DataType>());
                }
            }
        } else {
            if constexpr(std::is_pointer_v<DataType>) return;
            if constexpr(not type::sfinae::has_size_v<DataType>) return;
            auto hdf5Size = getSizeSelected(space);
            auto hdf5Byte = h5pp::util::getBytesPerElem<DataType>() * hdf5Size;
            auto dataByte = h5pp::util::getBytesTotal(data);
            auto dataSize = h5pp::util::getSize(data);
            if(dataByte < hdf5Byte) {
                throw h5pp::runtime_error("The buffer allocated for this read operation is smaller than the selected space in memory.\n"
                                          "\t Data transfer would write into memory out of bounds\n"
                                          "\t allocated : size {} | bytes {}\n"
                                          "\t selected  : size {} | bytes {}\n"
                                          "\t type     : [{}]",
                                          dataSize,
                                          dataByte,
                                          hdf5Size,
                                          hdf5Byte,
                                          type::sfinae::type_name<DataType>());
            }
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
            if(dataTypeSize != dsetTypeSize) {
                h5pp::logger::log->debug("Type size mismatch: dataset type {} bytes | given type {} bytes", dsetTypeSize, dataTypeSize);
            } else {
                h5pp::logger::log->warn(
                    "Detected packed HDF5 type: packed size {} bytes | native size {} bytes. This is not supported by h5pp yet!",
                    packedTypesize,
                    dataTypeSize);
            }
        }
        return dataTypeSize == dsetTypeSize;
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    void assertBytesPerElemMatch(const hid::h5t &h5Type) {
        if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::byte>) return;
        size_t dsetTypeSize = 0;
        size_t dataTypeSize = h5pp::util::getBytesPerElem<DataType>();
        if(H5Tget_class(h5Type) == H5T_STRING) dsetTypeSize = H5Tget_size(H5T_C_S1);
        else dsetTypeSize = h5pp::hdf5::getBytesPerElem(h5Type);
        if(dataTypeSize != dsetTypeSize) {
            // The dsetType may have been generated by H5Tpack, in which case we should check against the native type
            size_t   packedTypesize = dsetTypeSize;
            hid::h5t nativetype     = H5Tget_native_type(h5Type, H5T_DIR_ASCEND);
            dsetTypeSize            = h5pp::hdf5::getBytesPerElem(nativetype);
            if(dataTypeSize != dsetTypeSize) {
                throw h5pp::runtime_error("Type size mismatch: dataset type is [{}] bytes | Type of given data is [{}] bytes",
                                          dsetTypeSize,
                                          dataTypeSize);
            }

            else {
                h5pp::logger::log->warn(
                    "Detected packed HDF5 type: packed size {} bytes | native size {} bytes. This is not supported by h5pp yet!",
                    packedTypesize,
                    dataTypeSize);
            }
        }
    }

    template<typename DataType, typename = std::enable_if_t<not type::sfinae::is_h5pp_id<DataType>>>
    void assertReadTypeIsLargeEnough(const hid::h5t &h5Type) {
        if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::byte>) return;
        size_t dsetTypeSize = h5pp::hdf5::getBytesPerElem(h5Type);
        size_t dataTypeSize = h5pp::util::getBytesPerElem<DataType>();
        if(H5Tget_class(h5Type) == H5T_STRING) dsetTypeSize = H5Tget_size(H5T_C_S1);
        if(dataTypeSize != dsetTypeSize) {
            // The dsetType may have been generated by H5Tpack, in which case we should check against the native type
            size_t   packedTypesize = dsetTypeSize;
            hid::h5t nativetype     = H5Tget_native_type(h5Type, H5T_DIR_ASCEND);
            dsetTypeSize            = h5pp::hdf5::getBytesPerElem(nativetype);
            if(dataTypeSize > dsetTypeSize) {
                h5pp::logger::log->debug(
                    "Given data-type is too large: elements of type [{}] are [{}] bytes (each) | target HDF5 type is [{}] bytes",
                    type::sfinae::type_name<DataType>(),
                    dataTypeSize,
                    dsetTypeSize);
            } else if(dataTypeSize < dsetTypeSize) {
                throw h5pp::runtime_error(
                    "Given data-type is too small: elements of type [{}] are [{}] bytes (each) | target HDF5 type is [{}] bytes",
                    type::sfinae::type_name<DataType>(),
                    dataTypeSize,
                    dsetTypeSize);
            } else {
                h5pp::logger::log->warn(
                    "Detected packed HDF5 type: packed size {} bytes | native size {} bytes. This is not supported by h5pp yet!",
                    packedTypesize,
                    dataTypeSize);
            }
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
            if constexpr(type::sfinae::is_text_v<DataType>) {
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
                    // Furthermore,
                    // "The size set for a string datatype should include space for
                    // the null-terminator character, otherwise it will not be
                    // stored on (or retrieved from) disk"
                    retval              = H5Tset_size(type, desiredSize); // Desired size should be num chars + null terminator
                    dims                = {};
                    size                = 1;
                    bytes               = desiredSize;
                }
            } else if(type::sfinae::has_text_v<DataType>) {
                // Case 3
                retval = H5Tset_size(type, H5T_VARIABLE);
                bytes  = h5pp::util::getBytesTotal(data);
            }

            if(retval < 0) throw h5pp::runtime_error("Failed to set size on string");

            // The following makes sure there is a single "\0" at the end of the string when written to file.
            // Note however that bytes here is supposed to be the number of characters including null terminator.
            retval = H5Tset_strpad(type, H5T_STR_NULLTERM);
            if(retval < 0) throw h5pp::runtime_error("Failed to set strpad");

            // Sets the character set to UTF-8
            retval = H5Tset_cset(type, H5T_cset_t::H5T_CSET_UTF8);
            if(retval < 0) throw h5pp::runtime_error("Failed to set char-set UTF-8");
        }
    }

    template<typename h5x>
    [[nodiscard]] inline bool checkIfLinkExists(const h5x &loc, std::string_view linkPath, const hid::h5p &linkAccess = H5P_DEFAULT) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x>,
                      "Template function [h5pp::hdf5::checkIfLinkExists<h5x>(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        for(const auto &subPath : pathCumulativeSplit(linkPath, "/")) {
            int exists = H5Lexists(loc, util::safe_str(subPath).c_str(), linkAccess);
            if(exists == 0) {
                h5pp::logger::log->trace("Checking if link exists [{}] ... false", linkPath);
                return false;
            }
            if(exists < 0) throw h5pp::runtime_error("Failed to check if link exists [{}]", linkPath);
        }
        h5pp::logger::log->trace("Checking if link exists [{}] ... true", linkPath);
        return true;
    }

    template<typename h5x, typename h5x_loc>
    [[nodiscard]] h5x openLink(const h5x_loc      &loc,
                               std::string_view    linkPath,
                               std::optional<bool> linkExists = std::nullopt,
                               const hid::h5p     &access     = H5P_DEFAULT) {
        static_assert(type::sfinae::is_h5pp_link_id<h5x>,
                      "Template function [h5pp::hdf5::openLink<h5x>(...)] requires type h5x to be: "
                      "[h5pp::hid::h5d], [h5pp::hid::h5g] or [h5pp::hid::h5o]");
        static_assert(type::sfinae::is_hdf5_loc_id<h5x_loc>,
                      "Template function [h5pp::hdf5::openLink<h5x>(const h5x_loc & loc, ...)] requires type h5x_loc to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        if constexpr(not h5pp::ndebug) {
            if(not linkExists) linkExists = checkIfLinkExists(loc, linkPath, access);
            if(not linkExists.value()) throw h5pp::runtime_error("Cannot open link [{}]: it does not exist", linkPath);
        }
        if constexpr(std::is_same_v<h5x, hid::h5d>) h5pp::logger::log->trace("Opening dataset [{}]", linkPath);
        if constexpr(std::is_same_v<h5x, hid::h5g>) h5pp::logger::log->trace("Opening group [{}]", linkPath);
        if constexpr(std::is_same_v<h5x, hid::h5o>) h5pp::logger::log->trace("Opening object [{}]", linkPath);

        hid_t link;
        if constexpr(std::is_same_v<h5x, hid::h5d>) link = H5Dopen(loc, util::safe_str(linkPath).c_str(), access);
        if constexpr(std::is_same_v<h5x, hid::h5g>) link = H5Gopen(loc, util::safe_str(linkPath).c_str(), access);
        if constexpr(std::is_same_v<h5x, hid::h5o>) link = H5Oopen(loc, util::safe_str(linkPath).c_str(), access);

        if(link < 0) {
            if constexpr(std::is_same_v<h5x, hid::h5d>) throw h5pp::runtime_error("Failed to open dataset [{}]", linkPath);
            if constexpr(std::is_same_v<h5x, hid::h5g>) throw h5pp::runtime_error("Failed to open group [{}]", linkPath);
            if constexpr(std::is_same_v<h5x, hid::h5o>) throw h5pp::runtime_error("Failed to open object [{}]", linkPath);
        }
        return link;
    }

    template<typename h5x>
    inline void createGroup(const h5x           &loc,
                            std::string_view     relGroupPath,
                            std::optional<bool>  groupExists = std::nullopt,
                            const PropertyLists &plists      = PropertyLists()) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x>,
                      "Template function [h5pp::hdf5::createGroup(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        // Check if group exists already
        if(relGroupPath.empty()) return;
        if(not groupExists) groupExists = checkIfLinkExists(loc, relGroupPath, plists.linkAccess);
        if(not groupExists.value()) {
            h5pp::logger::log->trace("Creating group link [{}]", relGroupPath);
            // Setup property to create intermediate links
            hid::h5p lcpl = H5Pcopy(plists.linkCreate);
            herr_t   lerr = H5Pset_create_intermediate_group(lcpl, 1); // Create intermediate groups
            if(lerr < 0) throw h5pp::runtime_error("H5Pset_create_intermediate_group failed when creating group [{}]", relGroupPath);
            hid_t gid = H5Gcreate(loc, util::safe_str(relGroupPath).c_str(), lcpl, plists.groupCreate, plists.groupAccess);
            if(gid < 0) throw h5pp::runtime_error("H5Gcreate: Failed to create group link [{}]", relGroupPath);
            herr_t gerr = H5Gclose(gid);
            if(gerr < 0) throw h5pp::runtime_error("H5Gclose: Failed to close id for group [{}]", relGroupPath);
        } else {
            h5pp::logger::log->trace("Group exists already: [{}]", relGroupPath);
        }
    }

    template<typename h5x>
    [[nodiscard]] inline bool checkIfAttrExists(const h5x &link, std::string_view attrName, const hid::h5p &linkAccess = H5P_DEFAULT) {
        static_assert(type::sfinae::is_hdf5_link_id<h5x>,
                      "Template function [h5pp::hdf5::checkIfAttrExists<h5x>(const h5x & link, ...) requires type h5x to be: "
                      "[h5pp::hid::h5d], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        //        if(attrExists and attrExists.value()) return true;
        h5pp::logger::log->trace("Checking if attribute [{}] exitst in link ...", attrName);
        bool exists = H5Aexists_by_name(link, std::string(".").c_str(), util::safe_str(attrName).c_str(), linkAccess) > 0;
        h5pp::logger::log->trace("Checking if attribute [{}] exitst in link ... {}", attrName, exists);
        return exists;
    }

    template<typename h5x>
    [[nodiscard]] inline bool checkIfAttrExists(const h5x          &loc,
                                                std::string_view    linkPath,
                                                std::string_view    attrName,
                                                std::optional<bool> linkExists = std::nullopt,
                                                const hid::h5p     &linkAccess = H5P_DEFAULT) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x>,
                      "Template function [h5pp::hdf5::checkIfAttrExists<h5x>(const h5x & loc, ..., ...)] requires type h5x to be: "
                      "[h5pp::hid::h5d], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        if(not linkExists) linkExists = checkIfLinkExists(loc, linkPath, linkAccess);
        // If the link does not exist the attribute doesn't exist either
        if(not linkExists.value()) return false;
        // Otherwise, we open the link and check
        auto link = openLink<hid::h5o>(loc, linkPath, linkExists, linkAccess);
        h5pp::logger::log->trace("Checking if attribute [{}] exitst in link [{}] ...", attrName, linkPath);
        bool exists = H5Aexists_by_name(link, std::string(".").c_str(), util::safe_str(attrName).c_str(), linkAccess) > 0;
        h5pp::logger::log->trace("Checking if attribute [{}] exitst in link [{}] ... {}", attrName, linkPath, exists);
        return exists;
    }

    [[nodiscard]] inline bool isCompressionAvaliable() {
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

    [[nodiscard]] inline int getValidCompressionLevel(std::optional<int> compressionLevel = std::nullopt) {
        if(isCompressionAvaliable()) {
            if(compressionLevel) {
                if(compressionLevel.value() < 10) {
                    return compressionLevel.value();
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

    [[nodiscard]] inline std::string getAttributeName(const hid::h5a &attribute) {
        std::string buf;
        ssize_t     bufSize = H5Aget_name(attribute, 0ul, nullptr); // Returns number of chars excluding \0
        if(bufSize >= 0) {
            buf.resize(type::safe_cast<size_t>(bufSize));
            H5Aget_name(attribute, type::safe_cast<size_t>(bufSize) + 1, buf.data()); // buf is guaranteed to have \0 at the end
        } else {
            H5Eprint(H5E_DEFAULT, stderr);
            h5pp::logger::log->debug("Failed to get attribute names");
        }
        return buf.c_str();
    }

    template<typename h5x>
    [[nodiscard]] inline std::vector<std::string> getAttributeNames(const h5x &link) {
        static_assert(type::sfinae::is_hdf5_link_id<h5x>,
                      "Template function [h5pp::hdf5::getAttributeNames(const h5x & link, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5d], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        auto                     numAttrs = H5Aget_num_attrs(link);
        std::vector<std::string> attrNames;
        std::string              buf;
        for(auto i = 0; i < numAttrs; i++) {
            hid::h5a attrId = H5Aopen_idx(link, type::safe_cast<unsigned int>(i));
            attrNames.emplace_back(getAttributeName(attrId));
        }
        return attrNames;
    }

    template<typename h5x>
    [[nodiscard]] inline std::vector<std::string> getAttributeNames(const h5x          &loc,
                                                                    std::string_view    linkPath,
                                                                    std::optional<bool> linkExists = std::nullopt,
                                                                    const hid::h5p     &linkAccess = H5P_DEFAULT) {
        static_assert(
            type::sfinae::is_hdf5_loc_id<h5x>,
            "Template function [h5pp::hdf5::getAttributeNames(const h5x & link, std::string_view linkPath)] requires type h5x to be: "
            "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        auto link = openLink<hid::h5o>(loc, linkPath, linkExists, linkAccess);
        return getAttributeNames(link);
    }

    [[nodiscard]] inline TypeInfo getTypeInfo(std::optional<std::string> objectPath,
                                              std::optional<std::string> objectName,
                                              const hid::h5s            &h5Space,
                                              const hid::h5t            &h5Type) {
        TypeInfo typeInfo;
        typeInfo.h5Name = std::move(objectName);
        typeInfo.h5Path = std::move(objectPath);
        typeInfo.h5Type = h5Type;
        typeInfo.h5Rank = h5pp::hdf5::getRank(h5Space);
        typeInfo.h5Size = h5pp::hdf5::getSize(h5Space);
        typeInfo.h5Dims = h5pp::hdf5::getDimensions(h5Space);

        std::tie(typeInfo.cppTypeIndex, typeInfo.cppTypeName, typeInfo.cppTypeBytes) = h5pp::type::getCppType(typeInfo.h5Type.value());
        return typeInfo;
    }

    [[nodiscard]] inline TypeInfo getTypeInfo(const hid::h5d &dataset) {
        auto dsetPath = h5pp::hdf5::getName(dataset);
        h5pp::logger::log->trace("Collecting type info about dataset [{}]", dsetPath);
        return getTypeInfo(dsetPath, std::nullopt, H5Dget_space(dataset), H5Dget_type(dataset));
    }

    template<typename h5x, typename = std::enable_if_t<type::sfinae::is_hdf5_loc_id<h5x>>>
    // enable_if so the compiler doesn't think it can use overload with std::string on first argument
    [[nodiscard]] inline TypeInfo getTypeInfo(const h5x          &loc,
                                              std::string_view    dsetName,
                                              std::optional<bool> dsetExists = std::nullopt,
                                              const hid::h5p     &dsetAccess = H5P_DEFAULT) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x>);
        auto dataset = openLink<hid::h5d>(loc, dsetName, dsetExists, dsetAccess);
        return getTypeInfo(dataset);
    }

    [[nodiscard]] inline TypeInfo getTypeInfo(const hid::h5a &attribute) {
        auto attrName = getAttributeName(attribute);
        auto linkPath = getName(attribute); // Returns the name of the link which has the attribute
        h5pp::logger::log->trace("Collecting type info about attribute [{}] in link [{}]", attrName, linkPath);
        return getTypeInfo(linkPath, attrName, H5Aget_space(attribute), H5Aget_type(attribute));
    }

    template<typename h5x, typename = std::enable_if_t<type::sfinae::is_hdf5_loc_id<h5x>>>
    [[nodiscard]] inline TypeInfo getTypeInfo(const h5x          &loc,
                                              std::string_view    linkPath,
                                              std::string_view    attrName,
                                              std::optional<bool> linkExists = std::nullopt,
                                              std::optional<bool> attrExists = std::nullopt,
                                              const hid::h5p     &linkAccess = H5P_DEFAULT) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x>);
        if(not linkExists) linkExists = checkIfLinkExists(loc, linkPath, linkAccess);
        if(not linkExists.value())
            throw h5pp::runtime_error("Attribute [{}] does not exist because its link does not exist: [{}]", attrName, linkPath);

        auto link = openLink<hid::h5o>(loc, linkPath, linkExists, linkAccess);
        if(not attrExists) attrExists = checkIfAttrExists(link, attrName, linkAccess);
        if(attrExists.value()) {
            hid::h5a attribute = H5Aopen_name(link, util::safe_str(attrName).c_str());
            return getTypeInfo(attribute);
        } else {
            throw h5pp::runtime_error("Attribute [{}] does not exist in link [{}]", attrName, linkPath);
        }
    }

    template<typename h5x>
    [[nodiscard]] std::vector<TypeInfo> getTypeInfo_allAttributes(const h5x &link) {
        static_assert(type::sfinae::is_hdf5_link_id<h5x>,
                      "Template function [h5pp::hdf5::getTypeInfo_allAttributes(const h5x & link)] requires type h5x to be: "
                      "[h5pp::hid::h5d], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        std::vector<TypeInfo> allAttrInfo;
        int                   num_attrs = H5Aget_num_attrs(link);
        if(num_attrs < 0) throw h5pp::runtime_error("Failed to get number of attributes in link");

        for(int idx = 0; idx < num_attrs; idx++) {
            hid::h5a attribute = H5Aopen_idx(link, type::safe_cast<unsigned int>(idx));
            allAttrInfo.emplace_back(getTypeInfo(attribute));
        }
        return allAttrInfo;
    }

    template<typename h5x>
    [[nodiscard]] inline std::vector<TypeInfo> getTypeInfo_allAttributes(const h5x          &loc,
                                                                         std::string_view    linkPath,
                                                                         std::optional<bool> linkExists = std::nullopt,
                                                                         const hid::h5p     &linkAccess = H5P_DEFAULT) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x>,
                      "Template function [h5pp::hdf5::getTypeInfo_allAttributes(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        auto link = openLink<hid::h5o>(loc, linkPath, linkExists, linkAccess);
        return getTypeInfo_allAttributes(link);
    }

    template<typename h5x>
    inline void deleteLink(const h5x &loc, std::string_view linkPath, const hid::h5p &linkAccess = H5P_DEFAULT) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x>,
                      "Template function [h5pp::hdf5::deleteLink(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        if(checkIfLinkExists(loc, linkPath, linkAccess)) {
            auto retval = H5Ldelete(loc, util::safe_str(linkPath).c_str(), linkAccess);
            if(retval < 0) throw h5pp::runtime_error("Failed to delete link [{}]", linkPath);
        }
    }

    template<typename h5x>
    inline void
        deleteAttribute(const h5x &loc, std::string_view linkPath, std::string_view attrName, const hid::h5p &linkAccess = H5P_DEFAULT) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x>,
                      "Template function [h5pp::hdf5::deleteAttribute(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        if(checkIfLinkExists(loc, linkPath, linkAccess)) {
            auto link = openLink<hid::h5o>(loc, linkPath, true, linkAccess);
            if(checkIfAttrExists(link, attrName, linkAccess)) {
                auto retval = H5Adelete(link, util::safe_str(attrName).c_str());
                if(retval < 0) throw h5pp::runtime_error("Failed to delete attribute [{}] in link [{}]", attrName, linkPath);
            }
        }
    }

    template<typename h5x>
    inline void createSoftLink(std::string_view     targetLinkPath,
                               const h5x           &loc,
                               std::string_view     softLinkPath,
                               const PropertyLists &plists = PropertyLists()) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x>,
                      "Template function [h5pp::hdf5::createSoftLink(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        if constexpr(not h5pp::ndebug) {
            if(not checkIfLinkExists(loc, targetLinkPath, plists.linkAccess))
                throw h5pp::runtime_error("Tried to create soft link to a path that does not exist [{}]", targetLinkPath);
        }

        h5pp::logger::log->trace("Creating soft link [{}] --> [{}]", targetLinkPath, softLinkPath);
        hid::h5p lcpl = H5Pcopy(plists.linkCreate);
        if(H5Pset_create_intermediate_group(lcpl, 1) < 0) // Set to create intermediate groups
            throw h5pp::runtime_error("H5Pset_create_intermediate_group failed");
        herr_t retval =
            H5Lcreate_soft(util::safe_str(targetLinkPath).c_str(), loc, util::safe_str(softLinkPath).c_str(), lcpl, plists.linkAccess);
        if(retval < 0) throw h5pp::runtime_error("Failed to create soft link [{}]  ", targetLinkPath);
    }

    template<typename h5x>
    inline void createHardLink(const h5x           &targetLinkLoc,
                               std::string_view     targetLinkPath,
                               const h5x           &hardLinkLoc,
                               std::string_view     hardLinkPath,
                               const PropertyLists &plists = PropertyLists()) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x>,
                      "Template function [h5pp::hdf5::createHardLink(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        if constexpr(not h5pp::ndebug) {
            if(not checkIfLinkExists(targetLinkLoc, targetLinkPath, plists.linkAccess))
                throw h5pp::runtime_error("Tried to create a hard link to a path that does not exist [{}]", targetLinkPath);
        }
        h5pp::logger::log->trace("Creating hard link [{}] --> [{}]", targetLinkPath, hardLinkPath);
        hid::h5p lcpl = H5Pcopy(plists.linkCreate);
        if(H5Pset_create_intermediate_group(lcpl, 1) < 0) // Set to create intermediate groups
            throw h5pp::runtime_error("H5Pset_create_intermediate_group failed");
        herr_t retval = H5Lcreate_hard(targetLinkLoc,
                                       util::safe_str(targetLinkPath).c_str(),
                                       hardLinkLoc,
                                       util::safe_str(hardLinkPath).c_str(),
                                       lcpl,
                                       plists.linkAccess);
        if(retval < 0) throw h5pp::runtime_error("Failed to create hard link [{}] -> [{}] ", targetLinkPath, hardLinkPath);
    }

    template<typename h5x>
    void createExternalLink(std::string_view     targetFilePath,
                            std::string_view     targetLinkPath,
                            const h5x           &loc,
                            std::string_view     softLinkPath,
                            const PropertyLists &plists = PropertyLists()) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x>,
                      "Template function [h5pp::hdf5::createExternalLink(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        h5pp::logger::log->trace("Creating external link [{}] from file [{}] : [{}]", softLinkPath, targetFilePath, targetLinkPath);
        hid::h5p lcpl = H5Pcopy(plists.linkCreate);
        if(H5Pset_create_intermediate_group(lcpl, 1) < 0) // Set to create intermediate groups
            throw h5pp::runtime_error("H5Pset_create_intermediate_group failed");
        herr_t retval = H5Lcreate_external(util::safe_str(targetFilePath).c_str(),
                                           util::safe_str(targetLinkPath).c_str(),
                                           loc,
                                           util::safe_str(softLinkPath).c_str(),
                                           lcpl,
                                           plists.linkAccess);

        if(retval < 0) throw h5pp::runtime_error("Failed to create external link [{}] --> [{}]", targetLinkPath, softLinkPath);
    }

    inline void setProperty_layout(DsetInfo &dsetInfo) {
        if(not dsetInfo.h5DsetCreate)
            throw h5pp::logic_error("Could not configure the H5D layout: the dataset creation property list has not been initialized");
        if(not dsetInfo.h5Layout)
            throw h5pp::logic_error("Could not configure the H5D layout: the H5D layout parameter has not been initialized");
        switch(dsetInfo.h5Layout.value()) {
            case H5D_CHUNKED: h5pp::logger::log->trace("Setting layout H5D_CHUNKED"); break;
            case H5D_COMPACT: h5pp::logger::log->trace("Setting layout H5D_COMPACT"); break;
            case H5D_CONTIGUOUS: h5pp::logger::log->trace("Setting layout H5D_CONTIGUOUS"); break;
            default:
                throw h5pp::runtime_error(
                    "Given invalid layout when creating dataset property list. Choose one of H5D_COMPACT,H5D_CONTIGUOUS,H5D_CHUNKED");
        }
        herr_t err = H5Pset_layout(dsetInfo.h5DsetCreate.value(), dsetInfo.h5Layout.value());
        if(err < 0) throw h5pp::runtime_error("Could not set layout");
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
            dsetInfo.dsetChunk    = std::nullopt;
            dsetInfo.dsetDimsMax  = std::nullopt;
            dsetInfo.h5Layout     = H5D_CONTIGUOUS; // In case it's a big text
            dsetInfo.resizePolicy = h5pp::ResizePolicy::OFF;
            setProperty_layout(dsetInfo);
            return;
        }

        if(not dsetInfo.h5DsetCreate)
            throw h5pp::logic_error("Could not configure chunk dimensions: the dataset creation property list has not been initialized");
        if(not dsetInfo.dsetRank)
            throw h5pp::logic_error("Could not configure chunk dimensions: the dataset rank (n dims) has not been initialized");
        if(not dsetInfo.dsetDims)
            throw h5pp::logic_error("Could not configure chunk dimensions: the dataset dimensions have not been initialized");
        if(dsetInfo.dsetRank.value() != type::safe_cast<int>(dsetInfo.dsetDims->size())) {
            throw h5pp::logic_error("Could not set chunk dimensions properties: Rank mismatch: dataset dimensions {} has "
                                    "different number of elements than reported rank {}",
                                    dsetInfo.dsetDims.value(),
                                    dsetInfo.dsetRank.value());
        }
        if(dsetInfo.dsetDims->size() != dsetInfo.dsetChunk->size()) {
            throw h5pp::logic_error("Could not configure chunk dimensions: Rank mismatch: dataset dimensions {} and chunk "
                                    "dimensions {} do not have the same number of elements",
                                    dsetInfo.dsetDims->size(),
                                    dsetInfo.dsetChunk->size());
        }

        h5pp::logger::log->trace("Setting chunk dimensions {}", dsetInfo.dsetChunk.value());
        herr_t err =
            H5Pset_chunk(dsetInfo.h5DsetCreate.value(), type::safe_cast<int>(dsetInfo.dsetChunk->size()), dsetInfo.dsetChunk->data());
        if(err < 0) throw h5pp::runtime_error("Could not set chunk dimensions");
    }

    inline void setProperty_compression(DsetInfo &dsetInfo) {
        if(not dsetInfo.compression) return;
        if(not isCompressionAvaliable()) return;
        if(not dsetInfo.h5DsetCreate)
            throw h5pp::runtime_error("Could not configure compression: field h5_plist_dset_create has not been initialized");
        if(not dsetInfo.h5Layout) throw h5pp::logic_error("Could not configure compression: field h5_layout has not been initialized");

        if(dsetInfo.h5Layout.value() != H5D_CHUNKED) {
            h5pp::logger::log->trace("Compression ignored: Layout is not H5D_CHUNKED");
            dsetInfo.compression = std::nullopt;
            return;
        }
        // Negative value means to skip zlib altogether, even though it could be enabled.
        // This is different from 0, which means "no compression".
        if(dsetInfo.compression.value() < 0) return;
        if(dsetInfo.compression.value() > 9) {
            h5pp::logger::log->warn("Compression level too high: [{}]. Reducing to [9]", dsetInfo.compression.value());
            dsetInfo.compression = 9;
        }
        h5pp::logger::log->trace("Setting compression level {}", dsetInfo.compression.value());
        herr_t err = H5Pset_deflate(dsetInfo.h5DsetCreate.value(), type::safe_cast<unsigned int>(dsetInfo.compression.value()));
        if(err < 0) throw h5pp::runtime_error("Failed to set compression level. Check that your HDF5 version has zlib enabled.");
    }

    inline void
        selectHyperslab(const hid::h5s &space, const Hyperslab &hyperSlab, std::optional<H5S_seloper_t> select_op_override = std::nullopt) {
        if(hyperSlab.empty()) return;
        hid_t h5space = space.value(); // We will be using this identifier a lot here
        int   rank    = H5Sget_simple_extent_ndims(h5space);
        if(rank < 0) throw h5pp::runtime_error("Failed to read space rank");
        std::vector<hsize_t> dims(type::safe_cast<size_t>(rank));
        H5Sget_simple_extent_dims(h5space, dims.data(), nullptr);
        // If one of slabOffset or slabExtent is given, then the other must also be given
        if(hyperSlab.offset and not hyperSlab.extent)
            throw h5pp::logic_error("Could not setup hyperslab metadata: Given hyperslab offset but not extent");
        if(not hyperSlab.offset and hyperSlab.extent)
            throw h5pp::logic_error("Could not setup hyperslab metadata: Given hyperslab extent but not offset");

        // If given, ranks of slabOffset and slabExtent must be identical to each other and to the rank of the existing dataset
        if(hyperSlab.offset and hyperSlab.extent and (hyperSlab.offset.value().size() != hyperSlab.extent.value().size())) {
            throw h5pp::logic_error(
                h5pp::format("Could not setup hyperslab metadata: Size mismatch in given hyperslab arrays: offset {} | extent {}",
                             hyperSlab.offset.value(),
                             hyperSlab.extent.value()));
        }

        if(hyperSlab.offset and hyperSlab.offset.value().size() != type::safe_cast<size_t>(rank)) {
            throw h5pp::logic_error(
                h5pp::format("Could not setup hyperslab metadata: Hyperslab arrays have different rank compared to the given space: "
                             "offset {} | extent {} | space dims {}",
                             hyperSlab.offset.value(),
                             hyperSlab.extent.value(),
                             dims));
        }

        // If given, slabStride must have the same rank as the dataset
        if(hyperSlab.stride and hyperSlab.stride.value().size() != type::safe_cast<size_t>(rank)) {
            throw h5pp::logic_error(
                h5pp::format("Could not setup hyperslab metadata: Hyperslab stride has a different rank compared to the dataset: "
                             "stride {} | dataset dims {}",
                             hyperSlab.stride.value(),
                             dims));
        }
        // If given, slabBlock must have the same rank as the dataset
        if(hyperSlab.blocks and hyperSlab.blocks.value().size() != type::safe_cast<size_t>(rank)) {
            throw h5pp::logic_error(
                h5pp::format("Could not setup hyperslab metadata: Hyperslab blocks has a different rank compared to the dataset: "
                             "blocks {} | dataset dims {}",
                             hyperSlab.blocks.value(),
                             dims));
        }

        if(not select_op_override) select_op_override = hyperSlab.select_oper;
        if(H5Sget_select_type(h5space) != H5S_SEL_HYPERSLABS and select_op_override != H5S_SELECT_SET)
            select_op_override = H5S_SELECT_SET; // First hyperslab selection must be H5S_SELECT_SET

        const hsize_t *offptr = hyperSlab.offset ? hyperSlab.offset->data() : nullptr;
        const hsize_t *extptr = hyperSlab.extent ? hyperSlab.extent->data() : nullptr;
        const hsize_t *strptr = hyperSlab.stride ? hyperSlab.stride->data() : nullptr;
        const hsize_t *blkptr = hyperSlab.blocks ? hyperSlab.blocks->data() : nullptr;
        herr_t         retval = H5Sselect_hyperslab(h5space, select_op_override.value(), offptr, strptr, extptr, blkptr);
        if(retval < 0) throw h5pp::runtime_error("Failed to select hyperslab");

#if H5_VERSION_GE(1, 10, 0)
        htri_t is_regular = H5Sis_regular_hyperslab(h5space);
        if(is_regular < 0) throw h5pp::runtime_error("Failed to check if Hyperslab selection is regular (non-rectangular)");
        else if(is_regular == 0)
            throw h5pp::runtime_error("Hyperslab selection is irregular (non-rectangular).\nThis is not yet supported by h5pp");

#endif
        htri_t valid = H5Sselect_valid(h5space);
        if(valid < 0) {
            throw h5pp::runtime_error("Hyperslab selection is invalid. space: {} | hyperslab: {}", dims, Hyperslab(space).string());
        } else if(valid == 0) {
            throw h5pp::runtime_error(h5pp::format("Hyperslab selection is not contained in the given space. space: {} | hyperslab: {}",
                                                   dims,
                                                   Hyperslab(space).string()));
        }
    }

    inline void selectHyperslabs(hid::h5s                                 &space,
                                 const std::vector<Hyperslab>             &hyperSlabs,
                                 std::optional<std::vector<H5S_seloper_t>> hyperSlabSelectOps = std::nullopt) {
        if(hyperSlabSelectOps and not hyperSlabSelectOps->empty()) {
            if(hyperSlabs.size() != hyperSlabSelectOps->size())
                for(const auto &slab : hyperSlabs) selectHyperslab(space, slab, hyperSlabSelectOps->at(0));
            else
                for(size_t num = 0; num < hyperSlabs.size(); num++) selectHyperslab(space, hyperSlabs[num], hyperSlabSelectOps->at(num));

        } else {
            for(const auto &slab : hyperSlabs) selectHyperslab(space, slab, H5S_seloper_t::H5S_SELECT_OR);
        }
    }

    inline void setSlabOverlap(const h5pp::Hyperslab &slab1, const h5pp::Hyperslab &slab2, h5pp::Hyperslab &olap) {
        if(not slab1.offset or not slab1.extent) throw h5pp::runtime_error("slab1 is not fully defined");
        if(not slab2.offset or not slab2.extent) throw h5pp::runtime_error("slab2 is not fully defined");
        if(not olap.offset or not olap.extent) throw h5pp::runtime_error("overlap slab is not fully defined");
        if(not h5pp::util::all_equal(slab1.offset->size(),
                                     slab1.extent->size(),
                                     slab2.offset->size(),
                                     slab2.extent->size(),
                                     olap.offset->size(),
                                     olap.extent->size())) {
            throw h5pp::runtime_error("Hyperslabs have incompatible ranks:\nslab1 {}\nslab2{}\nolap{}",
                                      slab1.string(),
                                      slab2.string(),
                                      olap.string());
        }

        auto rank = slab1.offset->size();
        for(size_t i = 0; i < rank; i++) {
            olap.offset.value()[i] = std::max(slab1.offset.value()[i], slab2.offset.value()[i]);
            hsize_t pos1           = slab1.offset.value()[i] + slab1.extent.value()[i];
            hsize_t pos2           = slab2.offset.value()[i] + slab2.extent.value()[i];
            auto    pos            = std::min<hsize_t>(pos1, pos2);
            if(pos >= olap.offset.value()[i]) olap.extent.value()[i] = pos - olap.offset.value()[i];
            else olap.extent.value()[i] = 0;

            if constexpr(not h5pp::ndebug) {
                if(olap.extent.value()[i] > (std::numeric_limits<hsize_t>::max() - (1ull << 32))) {
                    h5pp::logger::log->warn("olap extent in dim {} is {}: this is likely error to do with overflow/unsigned wrap",
                                            i,
                                            olap.extent.value()[i]);

                    if(olap.offset.value()[i] > (std::numeric_limits<hsize_t>::max() - (1ull << 32))) {
                        h5pp::logger::log->warn("olap offset in dim {} is {}: this is likely error to do with overflow/unsigned wrap",
                                                i,
                                                olap.offset.value()[i]);
                    }
                }
            }
        }
    }

    [[nodiscard]] inline h5pp::Hyperslab getSlabOverlap(const h5pp::Hyperslab &slab1, const h5pp::Hyperslab &slab2) {
        auto            rank = slab1.offset->size();
        h5pp::Hyperslab olap;
        olap.offset = std::vector<hsize_t>(rank, 0);
        olap.extent = std::vector<hsize_t>(rank, 0);
        setSlabOverlap(slab1, slab2, olap);
        return olap;
    }

    inline void setSpaceExtent(const hid::h5s                     &h5Space,
                               const std::vector<hsize_t>         &dims,
                               std::optional<std::vector<hsize_t>> dimsMax = std::nullopt) {
        if(H5Sget_simple_extent_type(h5Space) == H5S_SCALAR) return;
        if(dims.empty()) return;
        herr_t err;
        if(dimsMax) {
            // Here dimsMax was given by the user, and we have to do some sanity checks
            // Check that the ranks match
            if(dims.size() != dimsMax->size()) {
                throw h5pp::runtime_error("Number of dimensions (rank) mismatch: dims {} | max dims {}\n"
                                          "\t Hint: Dimension lists must have the same number of elements",
                                          dims,
                                          dimsMax.value());
            }

            std::vector<long> dimsMaxPretty;
            for(auto &dim : dimsMax.value())
                if(dim == H5S_UNLIMITED) dimsMaxPretty.emplace_back(-1);
                else dimsMaxPretty.emplace_back(type::safe_cast<long>(dim));
            h5pp::logger::log->trace("Setting dataspace extents: dims {} | max dims {}", dims, dimsMaxPretty);
            err = H5Sset_extent_simple(h5Space, type::safe_cast<int>(dims.size()), dims.data(), dimsMax->data());
            if(err < 0) throw h5pp::runtime_error("Failed to set extents on space: dims {} | max dims {}", dims, dimsMax.value());

        } else {
            h5pp::logger::log->trace("Setting dataspace extents: dims {}", dims);
            err = H5Sset_extent_simple(h5Space, type::safe_cast<int>(dims.size()), dims.data(), nullptr);
            if(err < 0) throw h5pp::runtime_error("Failed to set extents on space. Dims {}", dims);
        }
    }

    inline void setSpaceExtent(DsetInfo &dsetInfo) {
        if(not dsetInfo.h5Space) throw h5pp::logic_error("Could not set space extent: the space is not initialized");
        if(not dsetInfo.h5Space->valid()) throw h5pp::runtime_error("Could not set space extent. Space is not valid");
        if(H5Sget_simple_extent_type(dsetInfo.h5Space.value()) == H5S_SCALAR) return;
        if(not dsetInfo.dsetDims) throw h5pp::runtime_error("Could not set space extent: dataset dimensions are not defined");

        if(dsetInfo.h5Layout and dsetInfo.h5Layout.value() == H5D_CHUNKED and not dsetInfo.dsetDimsMax) {
            // Chunked datasets are unlimited unless told explicitly otherwise
            dsetInfo.dsetDimsMax = std::vector<hsize_t>(type::safe_cast<size_t>(dsetInfo.dsetRank.value()), 0);
            std::fill_n(dsetInfo.dsetDimsMax->begin(), dsetInfo.dsetDimsMax->size(), H5S_UNLIMITED);
        }
        try {
            setSpaceExtent(dsetInfo.h5Space.value(), dsetInfo.dsetDims.value(), dsetInfo.dsetDimsMax);
        } catch(const std::exception &err) {
            throw h5pp::runtime_error("Failed to set extent on dataset {} \n Reason {}", dsetInfo.string(), err.what());
        }
    }

    inline void extendSpace(const hid::h5s &space, const int dim, const hsize_t extent) {
        h5pp::logger::log->trace("Extending space dimension [{}] to extent [{}]", dim, extent);
        // Retrieve the current extent of this space
        const int            oldRank = H5Sget_simple_extent_ndims(space);
        std::vector<hsize_t> oldDims(type::safe_cast<size_t>(oldRank));
        H5Sget_simple_extent_dims(space, oldDims.data(), nullptr);

        // We may need to change the rank, for instance, if we are appending a new column
        // to a vector of size n, so it becomes an (n x 2) "matrix".
        const int            newRank = std::max(dim + 1, oldRank);
        std::vector<hsize_t> newDims(type::safe_cast<size_t>(newRank), 1);
        std::copy(oldDims.begin(), oldDims.end(), newDims.begin());
        newDims[type::safe_cast<size_t>(dim)] = extent;
        setSpaceExtent(space, newDims);
        //        H5Sset_extent_simple(space,newRank,newDims.data(),nullptr);
    }

    inline void extendDataset(const hid::h5d &dataset, const int dim, const hsize_t extent) {
        // Retrieve the current size of the memSpace (act as if you don't know its size and want to append)
        hid::h5s space = H5Dget_space(dataset);
        extendSpace(space, dim, extent);
    }

    inline void setDatasetDims(const hid::h5d &dataset, const std::vector<hsize_t> &dims) {
        h5pp::logger::log->trace("Extending dataset to dimensions {}", dims);
        if(H5Dset_extent(dataset, dims.data()) < 0) throw h5pp::runtime_error("Failed to set extent on dataset");
    }

    inline void setDatasetDims(DsetInfo &info, const std::vector<hsize_t> &dims) {
        info.assertResizeReady();
        setDatasetDims(info.h5Dset.value(), dims);
        info.h5Space  = H5Dget_space(info.h5Space.value());
        info.dsetDims = dims;
    }
    inline void setTableSize(h5pp::TableInfo &info, hsize_t size) {
        info.assertWriteReady();
        setDatasetDims(info.h5Dset.value(), {size});
        info.numRecords = size;
    }

    template<typename h5x>
    inline void extendDataset(const h5x          &loc,
                              std::string_view    dsetPath,
                              const int           dim,
                              const hsize_t       extent,
                              std::optional<bool> linkExists = std::nullopt,
                              const hid::h5p     &dsetAccess = H5P_DEFAULT) {
        auto dataset = openLink<hid::h5d>(loc, dsetPath, linkExists, dsetAccess);
        extendDataset(dataset, dim, extent);
    }

    inline void extendDataset(DsetInfo &info, const std::vector<hsize_t> &dims, size_t axis) {
        // We use this function to EXTEND the dataset to APPEND given data
        // We add dims to the current dimensions of the dataset.
        info.assertResizeReady();
        int appRank = type::safe_cast<int>(dims.size());
        if(H5Tis_variable_str(info.h5Type.value()) > 0) {
            // These are resized on the fly
            return;
        } else {
            // Sanity checks
            if(info.dsetRank.value() <= type::safe_cast<int>(axis)) {
                throw h5pp::runtime_error(
                    "Could not append to dataset [{}] along axis {}: Dataset rank ({}) must be strictly larger than the given axis ({})",
                    info.dsetPath.value(),
                    axis,
                    info.dsetRank.value(),
                    axis);
            }
            if(info.dsetRank.value() < appRank) {
                throw h5pp::runtime_error("Cannot append to dataset [{}] along axis {}: Dataset rank {} < appended rank {}",
                                          info.dsetPath.value(),
                                          axis,
                                          info.dsetRank.value(),
                                          appRank);
            }

            // If we have a dataset with dimensions ijkl, and we want to append along j, say, then the remaining
            // ikl should be at least as large as the corresponding dimensions on the given data.
            for(size_t idx = 0; idx < dims.size(); idx++) {
                if(idx != axis and dims[idx] > info.dsetDims.value()[idx]) {
                    throw h5pp::runtime_error(
                        "Could not append to dataset [{}] along axis {}: Dimension {} size mismatch: data {} | dset {}",
                        info.dsetPath.value(),
                        axis,
                        idx,
                        dims,
                        info.dsetDims.value());
                }
            }

            // Compute the new dset dimension. Note that dataRank <= dsetRank,
            // For instance when we add a column to a matrix, the column may be an nx1 vector.
            // Therefore, we embed the data dimensions in a (possibly) higher-dimensional space
            auto embeddedDims = std::vector<hsize_t>(type::safe_cast<size_t>(info.dsetRank.value()), 1);
            std::copy(dims.begin(), dims.end(), embeddedDims.begin()); // In the example above, we get nx1
            auto oldAxisSize  = info.dsetDims.value()[axis];           // Will need this later when drawing the hyperspace
            auto newAxisSize  = embeddedDims[axis];                    // Will need this later when drawing the hyperspace
            auto newDsetDims  = info.dsetDims.value();
            newDsetDims[axis] = oldAxisSize + newAxisSize;

            // Set the new dimensions
            std::string oldInfoStr = info.string(h5pp::logger::logIf(LogLevel::debug));
            herr_t      err        = H5Dset_extent(info.h5Dset.value(), newDsetDims.data());
            if(err < 0) throw h5pp::runtime_error("Failed to set extent {} on dataset [{}]", newDsetDims, info.dsetPath.value());

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
            slab.offset               = std::vector<hsize_t>(type::safe_cast<size_t>(info.dsetRank.value()), 0);
            slab.offset.value()[axis] = oldAxisSize;
            h5pp::hdf5::selectHyperslab(info.h5Space.value(), slab);
            h5pp::logger::log->debug("Extended dataset \n \t old: {} \n \t new: {}",
                                     oldInfoStr,
                                     info.string(h5pp::logger::logIf(LogLevel::debug)));
        }
    }

    inline void extendDataset(DsetInfo &dsetInfo, const DataInfo &dataInfo, size_t axis) {
        // We use this function to EXTEND the dataset to APPEND given data
        dataInfo.assertWriteReady();
        extendDataset(dsetInfo, dataInfo.dataDims.value(), axis);
    }

    inline void
        resizeDataset(DsetInfo &info, const std::vector<hsize_t> &newDimensions, std::optional<h5pp::ResizePolicy> policy = std::nullopt) {
        if(info.resizePolicy == h5pp::ResizePolicy::OFF) return;
        if(not policy) policy = info.resizePolicy;
        if(not policy and info.dsetSlab)
            policy = h5pp::ResizePolicy::GROW; // A hyperslab selection on the dataset has been made. Let's not shrink!
        if(not policy) policy = h5pp::ResizePolicy::FIT;
        if(policy == h5pp::ResizePolicy::OFF) return;
        if(policy == h5pp::ResizePolicy::FIT and info.dsetSlab) {
            bool outofbounds = false;
            for(size_t idx = 0; idx < newDimensions.size(); idx++) {
                if(info.dsetSlab->extent and newDimensions[idx] < info.dsetSlab->extent->at(idx)) {
                    outofbounds = true;
                    break;
                }
                if(info.dsetSlab->offset and newDimensions[idx] <= info.dsetSlab->offset->at(idx)) {
                    outofbounds = true;
                    break;
                }
            }
            if(outofbounds) {
                h5pp::logger::log->warn("A hyperslab selection was made on the dataset [{}{}]. "
                                        "However, resize policy [FIT] will resize this dataset to dimensions {}. "
                                        "This is likely an error.",
                                        info.dsetPath.value(),
                                        info.dsetSlab->string(),
                                        newDimensions);
            }
        }

        if(info.h5Layout and info.h5Layout.value() != H5D_CHUNKED) {
            switch(info.h5Layout.value()) {
                case H5D_COMPACT: throw h5pp::runtime_error("Datasets with H5D_COMPACT layout cannot be resized");
                case H5D_CONTIGUOUS: throw h5pp::runtime_error("Datasets with H5D_CONTIGUOUS layout cannot be resized");
                default: break;
            }
        }
        if(not info.dsetPath) throw h5pp::runtime_error("Could not resize dataset: Path undefined");
        if(not info.h5Space) throw h5pp::runtime_error("Could not resize dataset [{}]: info.h5Space undefined", info.dsetPath.value());
        if(not info.h5Type) throw h5pp::runtime_error("Could not resize dataset [{}]: info.h5Type undefined", info.dsetPath.value());
        if(H5Sget_simple_extent_type(info.h5Space.value()) == H5S_SCALAR) return; // These are not supposed to be resized. Typically strings
        if(H5Tis_variable_str(info.h5Type.value()) > 0) return;                   // These are resized on the fly
        info.assertResizeReady();

        // Return if there is no change compared to the current dimensions
        if(info.dsetDims.value() == newDimensions) return;
        // Compare ranks
        if(info.dsetDims->size() != newDimensions.size()) {
            throw h5pp::runtime_error("Could not resize dataset [{}]: "
                                      "Rank mismatch: "
                                      "The dataset dimensions {} must have the same number of elements as the new dimensions {}",
                                      info.dsetPath.value(),
                                      info.dsetDims.value(),
                                      newDimensions);
        }

        // If the dataset is already larger than newDimensions, and we only allow it to grow, there is nothing to do.
        if(policy == h5pp::ResizePolicy::GROW) {
            bool allDimsAreSmaller = true;
            for(size_t idx = 0; idx < newDimensions.size(); idx++)
                if(info.dsetDims.value()[idx] < newDimensions[idx]) allDimsAreSmaller = false;
            if(allDimsAreSmaller) return;
        }
        std::string oldInfoStr = info.string(h5pp::logger::logIf(LogLevel::debug));
        // Chunked datasets can shrink and grow in any direction
        // Non-chunked datasets can't be resized at all

        for(size_t idx = 0; idx < newDimensions.size(); idx++) {
            if(newDimensions[idx] > info.dsetDimsMax.value()[idx]) {
                throw h5pp::runtime_error(
                    "Could not resize dataset [{}]: "
                    "Dimension size error: "
                    "The target dimensions {} are larger than the maximum dimensions {} for this dataset. "
                    "Consider creating the dataset with larger maximum dimensions or use H5D_CHUNKED layout to enable unlimited resizing",
                    info.dsetPath.value(),
                    newDimensions,
                    info.dsetDimsMax.value());
            }
        }

        std::vector<hsize_t> finalDimensions = newDimensions;
        if(policy == h5pp::ResizePolicy::GROW) {
            // Make sure we only grow when the policy is to grow
            for(size_t idx = 0; idx < finalDimensions.size(); idx++)
                finalDimensions[idx] = std::max(newDimensions[idx], info.dsetDims.value()[idx]);
        }
        herr_t err = H5Dset_extent(info.h5Dset.value(), finalDimensions.data());
        if(err < 0) {
            throw h5pp::runtime_error("Failed to resize dataset [{}] from dimensions {} to {}",
                                      info.dsetPath.value(),
                                      info.dsetDims.value(),
                                      finalDimensions);
        }

        // By default, all the space (old and new) is selected
        info.dsetDims = finalDimensions;
        info.h5Space  = H5Dget_space(info.h5Dset->value()); // Needs to be refreshed after H5Dset_extent
        info.dsetByte = h5pp::hdf5::getBytesTotal(info.h5Dset.value(), info.h5Space, info.h5Type);
        info.dsetSize = h5pp::hdf5::getSize(info.h5Space.value());
        h5pp::logger::log->debug("Resized dataset \n \t old: {} \n \t new: {}",
                                 oldInfoStr,
                                 info.string(h5pp::logger::logIf(LogLevel::debug)));
    }

    inline void resizeDataset(DsetInfo &dsetInfo, const DataInfo &dataInfo) {
        /*! Use this function before writing to a dataset on file.
         * Requirements
         *      - dsetInfo.resizePolicy == FIT | GROW
         *      - dsetInfo.h5Layout == H5D_CHUNKED
         * If the requirements are fullfilled, this function makes sure the dataset is large enough to receive data.
         * This also takes hyperslabs into account:
         *      - If there is a hyperslab in dataInfo, the new dimensions should be at least this big
         *      - If there is a hyperslab in dsetInfo, the new dimensions should be increased to include it.
         */
        try {
            dataInfo.assertWriteReady();
            auto newDimensions = dataInfo.dataDims.value();
            if(dataInfo.dataSlab and dataInfo.dataSlab->extent) {
                if(dataInfo.dataDims->size() != dataInfo.dataSlab->extent->size()) {
                    h5pp::runtime_error("rank mismatch: \n data dims {}\n data slab {}",
                                        dataInfo.dataDims.value(),
                                        dataInfo.dataSlab->string());
                }
                newDimensions = dataInfo.dataSlab->extent.value();
            }
            if(dsetInfo.dsetSlab and dsetInfo.dsetSlab->offset and dsetInfo.dsetSlab->extent) {
                const auto &offset = dsetInfo.dsetSlab->offset.value();
                const auto &extent = dsetInfo.dsetSlab->extent.value();
                if(newDimensions.size() != offset.size() or newDimensions.size() != extent.size())
                    h5pp::runtime_error("rank mismatch: \n data dims {}\n dset slab {}", newDimensions, dsetInfo.dsetSlab->string());
                for(size_t idx = 0; idx < newDimensions.size(); idx++)
                    newDimensions[idx] = std::max(newDimensions[idx], offset[idx] + extent[idx]);
            }
            resizeDataset(dsetInfo, newDimensions);
        } catch(const std::exception &e) { throw h5pp::runtime_error("Failed to resize dataset: {}", e.what()); }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    inline void resizeData(DataType &data, const hid::h5s &space, const hid::h5t &type, size_t bytes) {
        // This function is used when reading data from file into memory.
        // It resizes the data so the space in memory can fit the data read from file.
        // Note that this resizes the data to fit the bounding box of the data selected in the fileSpace.
        // A selection of elements in memory space must occurr after calling this function.
        if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>) return; // h5pp never uses malloc
        if(bytes == 0) return;
        if(H5Tget_class(type) == H5T_STRING) {
            if constexpr(type::sfinae::is_text_v<DataType>) {
                // Minus one: String resize allocates the null-terminator automatically, and bytes is the number of characters including
                // null-terminator
                h5pp::util::resizeData(data, {type::safe_cast<hsize_t>(bytes) - 1});
            } else if constexpr(type::sfinae::has_text_v<DataType> and type::sfinae::is_iterable_v<DataType>) {
                // We have a container such as std::vector<std::string> here, and the dataset may have multiple string elements
                auto size = getSizeSelected(space);
                h5pp::util::resizeData(data, {type::safe_cast<hsize_t>(size)});
                // In variable length arrays each string element is dynamically resized when read.
                // For fixed-size we can resize already.
                if(not H5Tis_variable_str(type)) {
                    auto fixedStringSize = H5Tget_size(type) - 1; // Subtract null terminator
                    for(auto &str : data) h5pp::util::resizeData(str, {type::safe_cast<hsize_t>(fixedStringSize)});
                }
            } else {
                throw h5pp::runtime_error("Could not resize given container for text data: Unrecognized type for text [{}]",
                                          type::sfinae::type_name<DataType>());
            }
        } else if(H5Sget_simple_extent_type(space) == H5S_SCALAR) {
            h5pp::util::resizeData(data, {type::safe_cast<hsize_t>(1)});
        } else {
            int                  rank = H5Sget_simple_extent_ndims(space);
            std::vector<hsize_t> extent(type::safe_cast<size_t>(rank),
                                        0); // This will have the bounding box containing the current selection
            H5S_sel_type         select_type = H5Sget_select_type(space);
            if(select_type == H5S_sel_type::H5S_SEL_HYPERSLABS) {
                std::vector<hsize_t> start(type::safe_cast<size_t>(rank), 0);
                std::vector<hsize_t> end(type::safe_cast<size_t>(rank), 0);
                H5Sget_select_bounds(space, start.data(), end.data());
                for(size_t idx = 0; idx < extent.size(); idx++) extent[idx] = std::max<hsize_t>(0, 1 + end[idx] - start[idx]);
            } else {
                H5Sget_simple_extent_dims(space, extent.data(), nullptr);
            }
            if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::byte> and h5pp::type::sfinae::has_resize_v<DataType>) {
                // This adds support for reading arbitrary data into std::vector<std::byte>
                auto numBytes = getBytesPerElem(type) * util::getSizeFromDimensions(extent);
                data.resize(numBytes);
            } else {
                h5pp::util::resizeData(data, extent);
            }
            if(bytes != h5pp::util::getBytesTotal(data)) {
                h5pp::logger::log->warn("Size mismatch after resizing container [{}]: data [{}] bytes | dset [{}] bytes ",
                                        h5pp::type::sfinae::type_name<DataType>(),
                                        h5pp::util::getBytesTotal(data),
                                        bytes);
            }
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    inline void resizeData(DataType &data, DataInfo &dataInfo, const DsetInfo &info) {
        if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>) return; // h5pp never uses malloc
        if(not info.h5Space) throw h5pp::runtime_error("Could not resize given data container: DsetInfo field [h5Space] is not defined");
        if(not info.h5Type) throw h5pp::runtime_error("Could not resize given data container: DsetInfo field [h5Type] is not defined");
        if(not info.dsetByte) throw h5pp::runtime_error("Could not resize given data container: DsetInfo field [dsetByte] is not defined");
        auto oldDims = h5pp::util::getDimensions(data);                                     // Store the old dimensions
        resizeData(data, info.h5Space.value(), info.h5Type.value(), info.dsetByte.value()); // Resize the container
        auto newDims = h5pp::util::getDimensions(data);
        if(oldDims != newDims) {
            // Update the metadata
            if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::byte>)
                dataInfo.dataDims = getDimensions(info.h5Space.value());
            else dataInfo.dataDims = h5pp::util::getDimensions(data); // Will fail if no dataDims passed on a pointer
            dataInfo.dataSize = h5pp::util::getSizeFromDimensions(dataInfo.dataDims.value());
            dataInfo.dataRank = h5pp::util::getRankFromDimensions(dataInfo.dataDims.value());
            dataInfo.dataByte = h5pp::util::getBytesTotal(data);
            dataInfo.h5Space  = h5pp::util::getMemSpace(dataInfo.dataSize.value(), dataInfo.dataDims.value());
            // Apply hyperslab selection if there is any
            if(dataInfo.dataSlab) h5pp::hdf5::selectHyperslab(dataInfo.h5Space.value(), dataInfo.dataSlab.value());
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    inline void resizeData(DataType &data, DataInfo &dataInfo, const AttrInfo &attrInfo) {
        if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>) return; // h5pp never uses malloc
        if(not attrInfo.h5Space)
            throw h5pp::runtime_error("Could not resize given data container: AttrInfo field [h5Space] is not defined");
        if(not attrInfo.h5Type) throw h5pp::runtime_error("Could not resize given data container: AttrInfo field [h5Type] is not defined");
        if(not attrInfo.attrByte)
            throw h5pp::runtime_error("Could not resize given data container: AttrInfo field [attrByte] is not defined");

        auto oldDims = h5pp::util::getDimensions(data);                                                 // Store the old dimensions
        resizeData(data, attrInfo.h5Space.value(), attrInfo.h5Type.value(), attrInfo.attrByte.value()); // Resize the container
        auto newDims = h5pp::util::getDimensions(data);
        if(oldDims != newDims) {
            // Update the metadata
            dataInfo.dataDims = h5pp::util::getDimensions(data); // Will fail if no dataDims passed on a pointer
            dataInfo.dataSize = h5pp::util::getSizeFromDimensions(dataInfo.dataDims.value());
            dataInfo.dataRank = h5pp::util::getRankFromDimensions(dataInfo.dataDims.value());
            dataInfo.dataByte = dataInfo.dataSize.value() * h5pp::util::getBytesPerElem<DataType>();
            dataInfo.h5Space  = h5pp::util::getMemSpace(dataInfo.dataSize.value(), dataInfo.dataDims.value());
            // Apply hyperslab selection if there is any
            if(dataInfo.dataSlab) h5pp::hdf5::selectHyperslab(dataInfo.h5Space.value(), dataInfo.dataSlab.value());
        }
    }

    inline std::string getSpaceString(const hid::h5s &space, bool enable = true) {
        if(not enable) return {};
        std::string msg;
        msg.append(h5pp::format(" | size {}", H5Sget_simple_extent_npoints(space)));
        int                  rank = H5Sget_simple_extent_ndims(space);
        std::vector<hsize_t> dims(type::safe_cast<size_t>(rank), 0);
        H5Sget_simple_extent_dims(space, dims.data(), nullptr);
        msg.append(h5pp::format(" | rank {}", rank));
        msg.append(h5pp::format(" | dims {}", dims));
        if(H5Sget_select_type(space) == H5S_SEL_HYPERSLABS) {
            Hyperslab slab(space);
            msg.append(slab.string());
        }
        return msg;
    }

    template<typename DataType>
    void assertSpacesEqual(const hid::h5s &dataSpace, const hid::h5s &dsetSpace, const hid::h5t &dsetType) {
        if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::byte>)
            return; // The spaces will be mismatched, but the buffer size is checked elsewhere

        if(H5Tis_variable_str(dsetType) or H5Tget_class(dsetType) == H5T_STRING) {
            // Strings are a special case, e.g. we can write multiple string elements into just one.
            // Also space is allocated on the fly during read by HDF5. so size comparisons are useless here.
            return;
        }

        htri_t equal = H5Sextent_equal(dataSpace, dsetSpace);
        if(equal > 0) return;
        if(equal < 0) throw h5pp::runtime_error("Failed to compare space extents");
        auto dataDimensions   = getDimensions(dataSpace);
        auto dsetDimensions   = getDimensions(dataSpace);
        auto dataSelectedSize = getSizeSelected(dataSpace);
        auto dsetSelectedSize = getSizeSelected(dsetSpace);

        if(dataDimensions != dsetDimensions) {
            h5pp::logger::log->debug("Spaces have mismatching dimensions:\n"
                                     "\tdata space: {} | {} bytes/elem | {} bytes total\n"
                                     "\tdset space: {} | {} bytes/elem | {} bytes total\n",
                                     getSpaceString(dataSpace, h5pp::logger::logIf(LogLevel::debug)),
                                     getSpaceString(dsetSpace, h5pp::logger::logIf(LogLevel::debug)));
        }

        if(dataSelectedSize != dsetSelectedSize) {
            h5pp::logger::log->debug("Spaces have mismatching size:\n"
                                     "\tdata space: {} | {} bytes/elem | {} bytes total\n"
                                     "\tdset space: {} | {} bytes/elem | {} bytes total\n",
                                     getSpaceString(dataSpace, h5pp::logger::logIf(LogLevel::debug)),
                                     getSpaceString(dsetSpace, h5pp::logger::logIf(LogLevel::debug)));
        }
    }
    namespace internal {
        inline long        maxHits  = -1;
        inline long        maxDepth = -1;
        inline bool        symlinks = false;
        inline std::string searchKey;
        template<H5O_type_t ObjType>
        /* clang-format off */
        [[nodiscard]] inline constexpr std::string_view getObjTypeName() {
            if constexpr     (ObjType == H5O_type_t::H5O_TYPE_DATASET) return "dataset";
            else if constexpr(ObjType == H5O_type_t::H5O_TYPE_GROUP) return "group";
            else if constexpr(ObjType == H5O_type_t::H5O_TYPE_UNKNOWN) return "unknown";
            else if constexpr(ObjType == H5O_type_t::H5O_TYPE_NAMED_DATATYPE) return "named datatype";
            else if constexpr(ObjType == H5O_type_t::H5O_TYPE_NTYPES) return "ntypes";
            else return "map"; // Only in HDF5 v 1.12
        }
        [[nodiscard]] inline std::string_view getObjTypeName(H5O_type_t type) {
            if     (type == H5O_type_t::H5O_TYPE_DATASET) return "dataset";
            else if(type == H5O_type_t::H5O_TYPE_GROUP) return "group";
            else if(type == H5O_type_t::H5O_TYPE_UNKNOWN) return "unknown";
            else if(type == H5O_type_t::H5O_TYPE_NAMED_DATATYPE) return "named datatype";
            else if(type == H5O_type_t::H5O_TYPE_NTYPES) return "ntypes";
            return "map"; // Only in HDF5 v 1.12
        }
        template<H5L_type_t LinkType>
        [[nodiscard]] inline constexpr std::string_view getLinkTypeName() {
            if constexpr(LinkType == H5L_type_t::H5L_TYPE_HARD) return "hard";
            else if constexpr(LinkType == H5L_type_t::H5L_TYPE_SOFT) return "soft";
            else if constexpr(LinkType == H5L_type_t::H5L_TYPE_EXTERNAL) return "external";
            else if constexpr(LinkType == H5L_type_t::H5L_TYPE_MAX) return "max";
            else return "error";
        }
        [[nodiscard]] inline std::string_view getObjTypeName(H5L_type_t type) {
            if(type == H5L_type_t::H5L_TYPE_HARD) return "hard";
            else if(type == H5L_type_t::H5L_TYPE_SOFT) return "soft";
            else if(type == H5L_type_t::H5L_TYPE_EXTERNAL) return "external";
            else if(type == H5L_type_t::H5L_TYPE_MAX) return "max";
            else return "error";

        }
        /* clang-format on */

        template<H5O_type_t ObjType, typename InfoType>
        inline herr_t matcher([[maybe_unused]] hid_t id, const char *name, [[maybe_unused]] const InfoType *info, void *opdata) {
            // If object type is the one requested, and name matches the search key, then add it to the match list (a vector<string>)
            // If the search depth is passed the depth specified, return immediately
            // Return 0 to continue searching
            // Return 1 to finish the search. Normally when we've reached max search hits.

            // Skip symlinks if not asked for
            if(not symlinks and (info->type == H5L_TYPE_SOFT or info->type == H5L_TYPE_EXTERNAL)) return 0;
            std::string_view linkPath(name); // <-- This is the full path to the object that we are currently visiting.

            // If this group is deeper than maxDepth, just return
            auto depth = std::count(linkPath.begin(), linkPath.end(), '/');
            if(maxDepth >= 0 and depth > maxDepth) return 0;

            // Get the name of the object without the full path, to match the searchKey
            auto slashpos = linkPath.rfind('/');
            if(slashpos == std::string_view::npos) slashpos = 0;
            std::string_view linkName = linkPath.substr(slashpos);

            auto matchList = reinterpret_cast<std::vector<std::string> *>(opdata);
            try {
                if constexpr(std::is_same_v<InfoType, H5O_info_t>) {
                    if(info->type == ObjType or ObjType == H5O_TYPE_UNKNOWN) {
                        if(searchKey.empty() or linkName.find(searchKey) != std::string::npos) {
                            if(depth >= 1 and not symlinks) { // H5Lget_info fails on the root "." or "/"
                                // Make sure not to include symbolic links if not asked for
                                H5L_info_t lInfo;
                                herr_t     lerr = H5Lget_info(id, name, &lInfo, H5P_DEFAULT);
                                if(lerr < 0) throw h5pp::runtime_error("H5Lget_info failed on link [{}]", name);
                                if(lInfo.type == H5L_type_t::H5L_TYPE_SOFT or lInfo.type == H5L_TYPE_EXTERNAL) return 0;
                            }
                            matchList->push_back(name);
                        }
                    }
                }

                else if constexpr(std::is_same_v<InfoType, H5L_info_t>) {
                    /* clang-format off */
                    // It is expensive to use H5Oopen to peek H5O_info_t::type.
                    // It is faster to populate H5O_info_t using
                    H5O_info_t oInfo;
                    #if defined(H5Oget_info_vers) && H5Oget_info_vers >= 2
                        H5Oget_info_by_name(id, name, &oInfo, H5O_INFO_BASIC, H5P_DEFAULT);
                    #else
                        H5Oget_info_by_name(id,name, &oInfo,H5P_DEFAULT);
                    #endif
                    /* clang-format on */
                    if(oInfo.type == ObjType or ObjType == H5O_TYPE_UNKNOWN) {
                        if(searchKey.empty() or linkName.find(searchKey) != std::string::npos) {
                            // Otherwise append the match
                            matchList->push_back(name);
                        }
                    }
                } else {
                    if(searchKey.empty() or linkName.find(searchKey) != std::string::npos) matchList->push_back(name);
                }

                if(maxHits > 0 and type::safe_cast<long>(matchList->size()) >= maxHits) return 1;
                else return 0;
            } catch(const std::exception &ex) {
                throw h5pp::logic_error("Could not match object [{}] | loc_id [{}] | reason: {}", name, id, ex.what());
            }
        }

        template<H5O_type_t ObjType, typename h5x>
        inline herr_t visit_by_name(const h5x                &loc,
                                    std::string_view          root,
                                    std::vector<std::string> &matchList,
                                    const PropertyLists      &plists = PropertyLists()) {
            static_assert(type::sfinae::is_hdf5_loc_id<h5x>,
                          "Template function [h5pp::hdf5::visit_by_name(const h5x & loc, ...)] requires type h5x to be: "
                          "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
            auto safe_root = util::safe_str(root);

            if constexpr(h5pp::type::sfinae::is_any_v<h5x, hid::h5f, hid::h5g>) {
                if(internal::maxDepth == 0) {
                    // Faster when we don't need to iterate recursively
                    hsize_t idx = 0;
                    auto    gid = openLink<hid::h5g>(loc, safe_root, std::nullopt, plists.groupAccess);
                    return H5Literate(gid,
                                      H5_index_t::H5_INDEX_NAME,
                                      H5_iter_order_t::H5_ITER_NATIVE,
                                      &idx,
                                      internal::matcher<ObjType>,
                                      &matchList);
                }
            }
            return H5Lvisit_by_name(loc,
                                    safe_root.c_str(),
                                    H5_index_t::H5_INDEX_NAME,
                                    H5_iter_order_t::H5_ITER_NATIVE,
                                    internal::matcher<ObjType>,
                                    &matchList,
                                    plists.linkAccess);
        }

    }

    template<H5O_type_t ObjType, typename h5x>
    [[nodiscard]] inline std::vector<std::string> findLinks(const h5x           &loc,
                                                            std::string_view     searchKey      = "",
                                                            std::string_view     searchRoot     = "/",
                                                            long                 maxHits        = -1,
                                                            long                 maxDepth       = -1,
                                                            bool                 followSymlinks = false,
                                                            const PropertyLists &plists         = PropertyLists()) {
        h5pp::logger::log->trace("search key: {} | root: {} | type: {} | max hits {} | max depth {}",
                                 searchKey,
                                 searchRoot,
                                 internal::getObjTypeName<ObjType>(),
                                 maxHits,
                                 maxDepth);

        if(not checkIfLinkExists(loc, searchRoot, plists.linkAccess))
            throw h5pp::runtime_error("Cannot find links inside group [{}]: it does not exist", searchRoot);

        std::vector<std::string> matchList;
        internal::maxHits   = maxHits;
        internal::maxDepth  = maxDepth;
        internal::symlinks  = followSymlinks;
        internal::searchKey = searchKey;
        herr_t err          = internal::visit_by_name<ObjType>(loc, searchRoot, matchList, plists);
        if(err < 0) {
            throw h5pp::runtime_error(
                "Error occurred when trying to find links of type [{}] containing [{}] while iterating from root [{}]",
                internal::getObjTypeName<ObjType>(),
                searchKey,
                searchRoot);
        }

        return matchList;
    }

    template<H5O_type_t ObjType, typename h5x>
    [[nodiscard]] inline std::vector<std::string>
        getContentsOfLink(const h5x &loc, std::string_view linkPath, long maxDepth = 1, const PropertyLists &plists = PropertyLists()) {
        std::vector<std::string> contents;
        internal::maxHits  = -1;
        internal::maxDepth = maxDepth;
        internal::searchKey.clear();
        herr_t err = internal::visit_by_name<ObjType>(loc, linkPath, contents, plists);
        if(err < 0) throw h5pp::runtime_error("Failed to iterate link [{}] of type [{}]", linkPath, internal::getObjTypeName<ObjType>());

        return contents;
    }

    inline void createDataset(DsetInfo &dsetInfo, const PropertyLists &plists = PropertyLists()) {
        // Here we create, the dataset id and set its properties before writing data to it.
        dsetInfo.assertCreateReady();
        if(dsetInfo.dsetExists and dsetInfo.dsetExists.value()) {
            h5pp::logger::log->trace("No need to create dataset [{}]: exists already", dsetInfo.dsetPath.value());
            return;
        }
        // Create intermediate groups
        createGroup(dsetInfo.getLocId(), util::getParentPath(dsetInfo.dsetPath.value()), std::nullopt, plists);
        h5pp::logger::log->debug("Creating dataset {}", dsetInfo.string(h5pp::logger::logIf(LogLevel::debug)));
        hid_t dsetId = H5Dcreate(dsetInfo.getLocId(),
                                 util::safe_str(dsetInfo.dsetPath.value()).c_str(),
                                 dsetInfo.h5Type.value(),
                                 dsetInfo.h5Space.value(),
                                 plists.linkCreate,
                                 dsetInfo.h5DsetCreate.value(),
                                 dsetInfo.h5DsetAccess.value());

        if(dsetId <= 0) throw h5pp::runtime_error("Failed to create dataset {}", dsetInfo.string());

        dsetInfo.h5Dset     = dsetId;
        dsetInfo.dsetExists = true;
    }

    inline void createAttribute(AttrInfo &attrInfo) {
        // Here we create, or register, the attribute id and set its properties before writing data to it.
        if(attrInfo.linkExists.has_value() and not attrInfo.linkExists.value()) {
            throw h5pp::runtime_error("Could not create attribute [{}] in link [{}]: "
                                      "Link does not exist.",
                                      attrInfo.attrName.value(),
                                      attrInfo.linkPath.value());
        }
        attrInfo.assertCreateReady();
        if(attrInfo.attrExists and attrInfo.attrExists.value()) {
            h5pp::logger::log->trace("No need to create attribute [{}] in link [{}]: exists already",
                                     attrInfo.attrName.value(),
                                     attrInfo.linkPath.value());
            return;
        }
        h5pp::logger::log->debug("Creating attribute {}", attrInfo.string(h5pp::logger::logIf(LogLevel::debug)));
        hid_t attrId = H5Acreate(attrInfo.h5Link.value(),
                                 util::safe_str(attrInfo.attrName.value()).c_str(),
                                 attrInfo.h5Type.value(),
                                 attrInfo.h5Space.value(),
                                 attrInfo.h5PlistAttrCreate.value(),
                                 attrInfo.h5PlistAttrAccess.value());
        if(attrId <= 0) {
            throw h5pp::runtime_error("Failed to create attribute [{}] for link [{}]",
                                      attrInfo.attrName.value(),
                                      attrInfo.linkPath.value());
        }

        attrInfo.h5Attr     = attrId;
        attrInfo.attrExists = true;
    }

    template<typename DataType>
    [[nodiscard]] std::vector<const char *> getCharPtrVector(const DataType &data) {
        std::vector<const char *> sv;
        if constexpr(type::sfinae::is_text_v<DataType> and type::sfinae::has_data_v<DataType>) { // Takes care of std::string
            sv.push_back(data.data());
        } else if constexpr(type::sfinae::is_text_v<DataType>) {                                 // Takes care of char pointers and arrays
            sv.push_back(data);
        } else if constexpr(type::sfinae::is_iterable_v<DataType>) {                             // Takes care of containers with text
            for(auto &elem : data) {
                if constexpr(type::sfinae::is_text_v<decltype(elem)> and
                             type::sfinae::has_data_v<decltype(elem)>) {       // Takes care of containers with std::string
                    sv.push_back(elem.data());
                } else if constexpr(type::sfinae::is_text_v<decltype(elem)>) { // Takes care of containers  of char pointers and arrays
                    sv.push_back(elem);
                } else {
                    sv.push_back(&elem); // Takes care of other things?
                }
            }
        } else {
            throw h5pp::runtime_error("Failed to get char pointer of datatype [{}]", type::sfinae::type_name<DataType>());
        }
        return sv;
    }

    template<bool compile = h5pp::has_direct_chunk>
    inline void H5Dwrite_single_chunk([[maybe_unused]] const hid_t                  &h5dset,
                                      [[maybe_unused]] const hid_t                  &h5dxpl, // Dataset transfer property list
                                      [[maybe_unused]] H5Z_filter_t                 &filters,
                                      [[maybe_unused]] uint32_t                     &mask,
                                      [[maybe_unused]] int                          &deflate,
                                      [[maybe_unused]] const std::vector<hsize_t>   &chunkOffset,
                                      [[maybe_unused]] const std::vector<std::byte> &chunkBuffer) {
        if constexpr(compile) {
#if H5PP_HAS_DIRECT_CHUNK == 1
            size_t chunkByte   = chunkBuffer.size();
            bool   skipDeflate = (mask & H5Z_FILTER_DEFLATE) == H5Z_FILTER_DEFLATE;
            bool   isOnDeflate = (filters & H5Z_FILTER_DEFLATE) == H5Z_FILTER_DEFLATE;

            if constexpr(not has_filter_deflate) {
                if(isOnDeflate and not skipDeflate) {
                    throw h5pp::runtime_error(
                        "H5Dread_single_chunk: deflate filter is not available in this HDF5 library. Failed to read chunk "
                        "with enabled filter H5Z_FILTER_DEFLATE");
                }
            }

    #if H5PP_HAS_FILTER_DEFLATE == 1 && H5PP_HAS_ZLIB_H == 1
            if(deflate >= 0 and isOnDeflate and not skipDeflate) {
                auto deflate_size_adjust = [](auto &s) { // This is in the documentation, but I have no idea why it's needed
                    return std::ceil(static_cast<double>(s) * 1.001) + 12;
                };

                auto z_dst_nbytes = static_cast<uLongf>(deflate_size_adjust(chunkByte));
                auto z_src_nbytes = static_cast<uLong>(chunkByte);

                // Allocate space for the compressed buffer
                std::vector<std::byte> chunkZBuffer(z_dst_nbytes);
                auto                   z_dst = reinterpret_cast<Bytef *>(chunkZBuffer.data());

                // Get a pointer to the source buffer for compression
                auto z_src = static_cast<const Bytef *>(h5pp::util::getVoidPointer<const void *>(chunkBuffer));

                // Perform compression of the data into the destination chunkZbuffer
                int z_erw = compress2(z_dst, &z_dst_nbytes, z_src, z_src_nbytes, deflate);
                /* Check for various zlib errors */
                if(Z_BUF_ERROR == z_erw) throw h5pp::runtime_error("overflow");
                else if(Z_MEM_ERROR == z_erw) throw h5pp::runtime_error("deflate memory error");
                else if(Z_OK != z_erw) throw h5pp::runtime_error("other deflate error");

                /* Write the compressed chunk data */
                herr_t erw = H5Dwrite_chunk(h5dset, h5dxpl, mask, chunkOffset.data(), z_dst_nbytes, z_dst);
                if(erw < 0) throw h5pp::runtime_error("Failed to write compressed chunk at offset {}", chunkOffset);
            } else
    #endif
            {
                /* Write the raw chunk data */
                herr_t erw = H5Dwrite_chunk(h5dset, h5dxpl, mask, chunkOffset.data(), chunkByte, chunkBuffer.data());
                if(erw < 0) throw h5pp::runtime_error("Failed to write raw chunk at offset {}", chunkOffset);
            }
#endif
        } else {
            static_assert(compile, "This " H5_VERS_INFO " does not support direct chunk writes");
        }
    }

    template<bool compile = h5pp::has_direct_chunk>
    inline void H5Dread_single_chunk([[maybe_unused]] const hid_t                &h5dset,
                                     [[maybe_unused]] const hid_t                &h5dxpl, // Dataset transfer property list
                                     [[maybe_unused]] H5Z_filter_t               &filters,
                                     [[maybe_unused]] uint32_t                   &mask,
                                     [[maybe_unused]] const std::vector<hsize_t> &chunkOffset,
                                     [[maybe_unused]] std::vector<std::byte>     &chunkBuffer) {
        if constexpr(compile) {
#if H5PP_HAS_DIRECT_CHUNK == 1
            haddr_t chaddr = 0;
            hsize_t chsize = 0;
            herr_t  eci    = H5Dget_chunk_info_by_coord(h5dset, chunkOffset.data(), &mask, &chaddr, &chsize);
            if(eci < 0) h5pp::runtime_error("Failed to get chunk info for offset {}", chunkOffset);

            if(chsize == 0 or chaddr == HADDR_UNDEF) {
                h5pp::logger::log->trace(
                    h5pp::format("H5Dread_single_chunk: chunk at offset {} is not yet allocated. Clearing", chunkOffset));
                std::fill(chunkBuffer.begin(), chunkBuffer.end(), static_cast<std::byte>(0));
                return;
            }

            size_t chunkByte = h5pp::util::getBytesTotal(chunkBuffer);
            h5pp::logger::log->trace("H5Dread_single_chunk: offset {}", chunkOffset);

            hsize_t chunkByteStorage = 0; // Size of the chunk on disk
            herr_t  erc              = H5Dget_chunk_storage_size(h5dset, chunkOffset.data(), &chunkByteStorage);
            if(erc < 0)
                throw h5pp::runtime_error("H5Dread_single_chunk: failed to get chunk storage size for chunk offset {}", chunkOffset);
            if(chunkByteStorage == 0) return; // There is probably no chunk yet

            bool skipDeflate = (mask & H5Z_FILTER_DEFLATE) == H5Z_FILTER_DEFLATE;
            bool isOnDeflate = (filters & H5Z_FILTER_DEFLATE) == H5Z_FILTER_DEFLATE;

            if constexpr(not h5pp::ndebug) {
                h5pp::logger::log->trace(
                    "H5Dread_single_chunk: chunk buffer size {} | {} bytes | offset {} | storage {} bytes | chaddr {} | "
                    "chsize {} | mask {:b} | skipDeflate {} | isOnDeflate {}",
                    chunkBuffer.size(),
                    chunkByte,
                    chunkOffset,
                    chunkByteStorage,
                    chaddr,
                    chsize,
                    mask,
                    skipDeflate,
                    isOnDeflate);
            }
            if constexpr(not has_filter_deflate) {
                if(isOnDeflate and not skipDeflate) {
                    throw h5pp::runtime_error(
                        "H5Dread_single_chunk: deflate filter is not available in this HDF5 library. Failed to read chunk "
                        "with enabled filter H5Z_FILTER_DEFLATE");
                }
            }

    #if H5PP_HAS_FILTER_DEFLATE && H5PP_HAS_ZLIB_H
            if(isOnDeflate and not skipDeflate) {
                std::vector<std::byte> chunkZBuffer(chunkByteStorage);
                herr_t                 err = H5Dread_chunk(h5dset, h5dxpl, chunkOffset.data(), &mask, chunkZBuffer.data());
                if(err < 0) throw h5pp::runtime_error("Failed to read compressed chunk at offset {}", chunkOffset);

                int z_err = uncompress(reinterpret_cast<Bytef *>(chunkBuffer.data()),
                                       reinterpret_cast<uLongf *>(&chunkByte),
                                       reinterpret_cast<const Bytef *>(chunkZBuffer.data()),
                                       static_cast<uLong>(chunkByteStorage));
                /* Check for various zlib errors */
                if(Z_BUF_ERROR == z_err) throw h5pp::runtime_error("error: not enough room in output buffer");
                else if(Z_MEM_ERROR == z_err) throw h5pp::runtime_error("error: not enough memory");
                else if(Z_OK != z_err) throw h5pp::runtime_error("error: corrupted input data");
            } else
    #endif
            {
                if(chunkByte != chunkByteStorage) {
                    h5pp::logger::log->warn("H5Dread_single_chunk: Size mismatch: "
                                            "given chunk buffer and chunk on file have different sizes: "
                                            "buffer {} bytes | disk {} bytes | mask {:b}",
                                            chunkByte,
                                            chunkByteStorage,
                                            mask);
                    //                chunkBuffer.resize(read_chunk_nbytes);
                }

                herr_t err = H5Dread_chunk(h5dset, h5dxpl, chunkOffset.data(), &mask, chunkBuffer.data());
                if(err < 0) throw h5pp::runtime_error("Failed to read uncompressed chunk at offset {}", chunkOffset);
            }
#endif
        } else {
            static_assert(compile, "This " H5_VERS_INFO " does not support direct chunk writes");
        }
    }

    template<typename DataType, bool compile = h5pp::has_direct_chunk>
    void H5Dwrite_chunkwise([[maybe_unused]] const DataType             &data,
                            [[maybe_unused]] const h5pp::hid::h5d       &dataset,
                            [[maybe_unused]] const h5pp::hid::h5t       &datatype,
                            [[maybe_unused]] const h5pp::hid::h5p       &dsetCreate, // Dataset creation property list
                            [[maybe_unused]] const h5pp::hid::h5p       &dsetXfer,   // Dataset transfer property list
                            [[maybe_unused]] const std::vector<hsize_t> &dims,
                            [[maybe_unused]] const std::vector<hsize_t> &chunkDims,
                            [[maybe_unused]] const h5pp::Hyperslab      &dsetSlab,
                            [[maybe_unused]] const h5pp::Hyperslab      &dataSlab) {
        if constexpr(compile) {
#if H5PP_HAS_DIRECT_CHUNK == 1

            size_t     typeSize  = h5pp::hdf5::getBytesPerElem(datatype);
            size_t     chunkSize = h5pp::util::getSizeFromDimensions(chunkDims);
            hsize_t    chunkByte = chunkSize * typeSize;
            hid_t      h5dset    = dataset.value();    // Repeated calls to .value() takes time because validity is always checked
            hid_t      h5dcpl    = dsetCreate.value(); // Repeated calls to .value() takes time because validity is always checked
            hid_t      h5dxpl    = dsetXfer.value();   // Repeated calls to .value() takes time because validity is always checked
            auto       filters   = getFilters(h5dcpl);
            auto       deflate   = getDeflateLevel(h5dcpl);
            const auto rank      = dims.size();

            // Compute the total number of chunks currently in the dataset
            hsize_t chunkCount = 0;                    // The total number of chunks currently in the dataset
            hid_t   h5space    = H5Dget_space(h5dset); // Must be created because H5Dget_num_chunks can't take H5S_SPACE_ALL yet
            herr_t  ers        = H5Sselect_all(h5space);
            if(ers < 0) throw h5pp::runtime_error("writeDataset_chunkwise: failed to select all elements in space");
            herr_t ern = H5Dget_num_chunks(h5dset, h5space, &chunkCount);
            if(ern < 0) throw h5pp::runtime_error("writeDataset_chunkwise: failed to get number of chunks in dataset");

            // Compute the total number of chunks that this dataset has room for
            std::vector<hsize_t> chunkRoom(rank);                                // counts how many chunks fit in each direction
            for(size_t i = 0; i < chunkRoom.size(); i++)
                chunkRoom[i] = (dims[i] + chunkDims[i] - 1) / chunkDims[i];      // Integral ceil on division
            size_t chunkCapacity = h5pp::util::getSizeFromDimensions(chunkRoom); // The total number of chunks that can fit

            if constexpr(not h5pp::ndebug) {
                if(h5pp::logger::log->level() == 0) {
                    h5pp::logger::log->info(
                        "writeDataset_chunkwise: data [type {} | size {} | {} bytes/item | {} bytes{}] dset [size {} | {} "
                        "bytes/item | storage {} bytes | deflate {} | dims {}{}]  "
                        "chunk [size {} | {} bytes | dims {} | count {} | capacity {} | room {}]",
                        type::sfinae::type_name<DataType>(),
                        h5pp::util::getSize(data),
                        h5pp::util::getBytesPerElem<DataType>(),
                        h5pp::util::getBytesTotal(data),
                        dataSlab.string(),
                        h5pp::hdf5::getSize(dataset),
                        typeSize,
                        H5Dget_storage_size(dataset),
                        deflate,
                        dims,
                        dsetSlab.string(),
                        chunkSize,
                        chunkByte,
                        chunkDims,
                        chunkCount,
                        chunkCapacity,
                        chunkRoom);
                }
            }

            uint32_t read_mask  = 0; // Tells which filters to skip on read
            uint32_t write_mask = 0; // Tells which filters to skip on write

            /* Allocate a reusable chunk buffers */
            std::vector<std::byte> chunkBuffer(chunkByte); // Takes existing chunks and modifies

            /* Allocate a reusable hyperslabs */
            h5pp::Hyperslab chunkSlab, olapSlab;
            chunkSlab.offset = std::vector<hsize_t>(rank);
            chunkSlab.extent = chunkDims;
            olapSlab.offset  = std::vector<hsize_t>(rank);
            olapSlab.extent  = std::vector<hsize_t>(rank);

            // Allocate coordinate vectors
            auto chunkCoord = std::vector<hsize_t>(rank);
            auto dsetCoord  = std::vector<hsize_t>(rank);
            auto dataCoord  = std::vector<hsize_t>(rank);
            auto olapCoord  = std::vector<hsize_t>(rank);
            // We iterate through all the chunks in the dataset
            for(size_t chunkIndex = 0; chunkIndex < chunkCapacity; chunkIndex++) {
                // Step 1, convert chunkIndex to coordinates
                h5pp::util::ind2sub(chunkRoom, chunkIndex, chunkCoord);
                for(size_t i = 0; i < rank; i++) chunkSlab.offset.value()[i] = chunkCoord[i] * chunkDims[i];

                // Step 3 Check if the current chunk would receive any data. If not, go to the next iteration
                h5pp::hdf5::setSlabOverlap(chunkSlab, dsetSlab, olapSlab);
                auto olapSize = h5pp::util::getSizeFromDimensions(olapSlab.extent.value());

                if(olapSize == 0) continue;
                // Now we know there are some overlapping points. These points are copied into the chunk buffer

                // Load a chunk buffer from file so that we can modify it later
                h5pp::hdf5::H5Dread_single_chunk(h5dset, h5dxpl, filters, read_mask, chunkSlab.offset.value(), chunkBuffer);

                // Step 4 Copy the part of the given data that overlaps with this chunk
                for(size_t i = 0; i < olapSize; i++) {
                    // 'i' is the linear index of the overlap slab.
                    h5pp::util::ind2sub(olapSlab.extent.value(), i, olapCoord);
                    // olapCoord are the coordinates in the overlap basis,
                    // but we need them in chunk basis, so we transform.
                    // First to the dataset basis, and then to chunk basis
                    for(size_t j = 0; j < rank; j++) {
                        dsetCoord[j]  = olapSlab.offset.value()[j] + olapCoord[j];
                        chunkCoord[j] = dsetCoord[j] - chunkSlab.offset.value()[j];
                        dataCoord[j]  = dsetCoord[j] - dsetSlab.offset.value()[j];
                    }

                    //                h5pp::logger::log->trace("dset  slab {} | coord {}", dsetSlab.string(), dsetCoord);
                    //                h5pp::logger::log->trace("olap  slab {} | coord {}", olapSlab.string(), olapCoord);
                    //                h5pp::logger::log->trace("data  slab {} | coord {}", dataSlab.string(), dataCoord);
                    //                h5pp::logger::log->trace("chunk  slab {} | coord {}", chunkSlab.string(), chunkCoord);

                    // Copy the value
                    auto   dataIdx         = h5pp::util::sub2ind(dataSlab.extent.value(), dataCoord);
                    auto   chunkIdx        = h5pp::util::sub2ind(chunkSlab.extent.value(), chunkCoord);
                    size_t dataByteOffset  = dataIdx;
                    size_t chunkByteOffset = chunkIdx * typeSize;
                    if constexpr(std::is_same_v<DataType, std::vector<std::byte>>) dataByteOffset *= typeSize;
                    std::memcpy(util::getVoidPointer<void *>(chunkBuffer, chunkByteOffset),
                                util::getVoidPointer<const void *>(data, dataByteOffset),
                                typeSize);
                }
                // Step 5 Now all the data is in the chunk buffer. Write to file
                if(deflate < 0) write_mask = 1;
                h5pp::hdf5::H5Dwrite_single_chunk(h5dset, h5dxpl, filters, write_mask, deflate, chunkSlab.offset.value(), chunkBuffer);
            }
#endif
        } else {
            static_assert(compile, "This " H5_VERS_INFO " does not support direct chunk writes");
        }
    }

    template<typename DataType>
    const void *
        getTextPtrForH5Dwrite(const DataType &data, const hid::h5t &h5Type, std::string &tempBuf, std::vector<const char *> &vlenBuf) {
        static_assert(not type::sfinae::is_h5pp_id<DataType>);
        auto dataPtr = h5pp::util::getVoidPointer<const void *>(data);
        if constexpr(type::sfinae::is_text_v<DataType> or type::sfinae::has_text_v<DataType>) {
            vlenBuf = getCharPtrVector(data);

            if(H5Tis_variable_str(h5Type) > 0) {
                // When H5T_VARIABLE, H5Dwrite function expects [const char **], which is what we get from vlenBuf.data()
                dataPtr = reinterpret_cast<const void **>(vlenBuf.data());
            } else if(vlenBuf.size() == 1) {
                dataPtr = static_cast<const void *>(*vlenBuf.data());
            } else if constexpr(type::sfinae::has_text_v<DataType> and type::sfinae::is_iterable_v<DataType>) {
                // We have a fixed-size string array now. We have to copy the strings to a contiguous array.
                // vlenBuf already contains the pointer to each string, and bytesPerStr should be the size of each string
                // including null terminators
                size_t bytesPerStr = H5Tget_size(h5Type); // This is the fixed-size of a string, not a char! Includes null term
                tempBuf.resize(bytesPerStr * vlenBuf.size());
                for(size_t i = 0; i < vlenBuf.size(); i++) {
                    auto offset = tempBuf.data() + type::safe_cast<long>(i * bytesPerStr);
                    // Construct a view of the null-terminated character string, not including the null character.
                    auto view   = std::string_view(vlenBuf[i]); // view.size() will not include null term here!
                    std::copy_n(std::begin(view), std::min(view.size(), bytesPerStr - 1), offset); // Do not copy null character
                }
                dataPtr = static_cast<const void *>(tempBuf.data());
            }
        }
        return dataPtr;
    }

    template<typename DataType>
    void writeDataset(const DataType      &data,
                      const DataInfo      &dataInfo,
                      const DsetInfo      &dsetInfo,
                      const PropertyLists &plists = PropertyLists()) {
        static_assert(not type::sfinae::is_h5pp_id<DataType>);
#ifdef H5PP_USE_EIGEN3
        if constexpr(type::sfinae::is_eigen_colmajor_v<DataType> and not type::sfinae::is_eigen_1d_v<DataType>) {
            h5pp::logger::log->debug("Converting data to row-major storage order");
            const auto tempRowm = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::writeDataset(tempRowm, dataInfo, dsetInfo, plists);
            return;
        }
#endif
        try {
            dsetInfo.assertWriteReady();
            dataInfo.assertWriteReady();
            h5pp::logger::log->trace("Writing from memory  {}", dataInfo.string(h5pp::logger::logIf(LogLevel::trace)));
            h5pp::logger::log->trace("Writing into dataset {}", dsetInfo.string(h5pp::logger::logIf(LogLevel::trace)));
            if(dsetInfo.dsetSlab) selectHyperslab(dsetInfo.h5Space.value(), dsetInfo.dsetSlab.value());
            if(dataInfo.dataSlab) selectHyperslab(dataInfo.h5Space.value(), dataInfo.dataSlab.value());
            h5pp::hdf5::assertWriteBufferIsLargeEnough(data, dataInfo.h5Space.value(), dsetInfo.h5Type.value());
            h5pp::hdf5::assertBytesPerElemMatch<DataType>(dsetInfo.h5Type.value());
            h5pp::hdf5::assertSpacesEqual<DataType>(dataInfo.h5Space.value(), dsetInfo.h5Space.value(), dsetInfo.h5Type.value());
        } catch(const std::exception &ex) {
            throw h5pp::runtime_error("Error writing to dataset [{}]:\n{}", dsetInfo.dsetPath.value(), ex.what());
        }

        // Get the memory address to the data buffer
        [[maybe_unused]] auto                      dataPtr = h5pp::util::getVoidPointer<const void *>(data);
        [[maybe_unused]] std::string               tempBuf; // A buffer in case we need to make text data contiguous
        [[maybe_unused]] std::vector<const char *> vlenBuf;
        if constexpr(type::sfinae::is_text_v<DataType> or type::sfinae::has_text_v<DataType>) {
            /* This will reseat dataPtr automatically, for the different ways that text data can be represented
             *    tempBuf: if H5Tis_variable_str == false:
             *             Holds a contiguous block of memory, into which we copy each fixed-length string in the given data.
             *             dataPtr points to tempBuf.data() gives a const char *
             *    vlenBuf: if H5Tis_variable_str == true:
             *             Holds pointers to each const char* when data is a container of strings.
             *             dataPtr points to vlenBuf.data() which is const char **,
             */
            dataPtr = getTextPtrForH5Dwrite(data, dsetInfo.h5Type.value(), tempBuf, vlenBuf);
        }

        // Write to file

        herr_t retval = H5Dwrite(dsetInfo.h5Dset.value(),
                                 dsetInfo.h5Type.value(),
                                 dataInfo.h5Space.value(),
                                 dsetInfo.h5Space.value(),
                                 plists.dsetXfer,
                                 dataPtr);
        if(retval < 0)
            throw h5pp::runtime_error("Failed to write into dataset \n\t {} \n from memory \n\t {}", dsetInfo.string(), dataInfo.string());
    }

    template<typename DataType, bool compile = h5pp::has_direct_chunk>
    void writeDataset_chunkwise([[maybe_unused]] const DataType            &data,
                                [[maybe_unused]] DataInfo                  &dataInfo,
                                [[maybe_unused]] DsetInfo                  &dsetInfo,
                                [[maybe_unused]] const h5pp::PropertyLists &plists = PropertyLists()) {
        if constexpr(type::sfinae::is_text_v<DataType> or type::sfinae::has_text_v<DataType>) {
            h5pp::logger::log->warn("writeDataset_chunkwise: text data is not supported, defaulting to normal writeDataset");
            writeDataset(data, dataInfo, dsetInfo, plists);
            return;
        } else if constexpr(not compile) {
            h5pp::logger::log->warn("writeDataset_chunkwise is not available in " H5_VERS_INFO ": defaulting to writeDataset");
            writeDataset(data, dataInfo, dsetInfo, plists);
            return;
        } else {
#ifdef H5PP_USE_EIGEN3
            if constexpr(type::sfinae::is_eigen_colmajor_v<DataType> and not type::sfinae::is_eigen_1d_v<DataType>) {
                h5pp::logger::log->debug("Converting data to row-major storage order");
                const auto tempRowm = eigen::to_RowMajor(data); // Convert to Row Major first;
                h5pp::hdf5::writeDataset_chunkwise(tempRowm, dataInfo, dsetInfo, plists);
                return;
            }
#endif

            dsetInfo.assertWriteReady();
            dataInfo.assertWriteReady();
            try {
                h5pp::logger::log->trace("Writing from memory  {}", dataInfo.string(h5pp::logger::logIf(LogLevel::trace)));
                h5pp::logger::log->trace("Writing into dataset {}", dsetInfo.string(h5pp::logger::logIf(LogLevel::trace)));
                h5pp::hdf5::assertWriteBufferIsLargeEnough(data, dataInfo.h5Space.value(), dsetInfo.h5Type.value());
                h5pp::hdf5::assertBytesPerElemMatch<DataType>(dsetInfo.h5Type.value());
                h5pp::hdf5::assertSpacesEqual<DataType>(dataInfo.h5Space.value(), dsetInfo.h5Space.value(), dsetInfo.h5Type.value());
            } catch(const std::exception &ex) {
                throw h5pp::runtime_error("Error writing to dataset [{}]:\n{}", dsetInfo.dsetPath.value(), ex.what());
            }

            const auto rank = dsetInfo.dsetDims->size();

            // Define a hyperslab with the shape of the given data
            h5pp::Hyperslab dataSlab;
            if(dataInfo.dataSlab) {
                dataSlab = dataInfo.dataSlab.value();
            } else {
                dataSlab.offset = std::vector<hsize_t>(rank, 0);
                if(rank == dataInfo.dataDims->size()) {
                    dataSlab.extent = dataInfo.dataDims.value();
                } else {
                    dataSlab.extent = std::vector<hsize_t>(rank, 1);
                    std::copy(dataInfo.dataDims->begin(), dataInfo.dataDims->end(), dataSlab.extent->rbegin());
                }
            }
            //  Define a hyperslab which selects the points in the dataset that will be written into.
            h5pp::Hyperslab dsetSlab;
            if(dsetInfo.dsetSlab) {
                dsetSlab = dsetInfo.dsetSlab.value();
            } else {
                dsetSlab.offset = std::vector<hsize_t>(rank, 0);
                dsetSlab.extent = dataSlab.extent;
            }
            H5Dwrite_chunkwise(data,
                               dsetInfo.h5Dset.value(),
                               dsetInfo.h5Type.value(),
                               dsetInfo.h5DsetCreate.value(),
                               plists.dsetXfer,
                               dsetInfo.dsetDims.value(),
                               dsetInfo.dsetChunk.value(),
                               dsetSlab,
                               dataSlab);
        }
    }

    template<typename DataType>
    void readDataset(DataType &data, const DataInfo &dataInfo, const DsetInfo &dsetInfo, const PropertyLists &plists = PropertyLists()) {
        static_assert(not std::is_const_v<DataType>);
        static_assert(not type::sfinae::is_h5pp_id<DataType>);
        // Transpose the data container before reading
#ifdef H5PP_USE_EIGEN3
        if constexpr(type::sfinae::is_eigen_colmajor_v<DataType> and not type::sfinae::is_eigen_1d_v<DataType>) {
            h5pp::logger::log->debug("Converting data to row-major storage order");
            auto tempRowMajor = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::readDataset(tempRowMajor, dataInfo, dsetInfo, plists);
            data = eigen::to_ColMajor(tempRowMajor);
            return;
        }
#endif
        try {
            dsetInfo.assertReadReady();
            dataInfo.assertReadReady();
            h5pp::logger::log->trace("Reading into memory  {}", dataInfo.string(h5pp::logger::logIf(LogLevel::trace)));
            h5pp::logger::log->trace("Reading from dataset {}", dsetInfo.string(h5pp::logger::logIf(LogLevel::trace)));
            if(dsetInfo.dsetSlab) selectHyperslab(dsetInfo.h5Space.value(), dsetInfo.dsetSlab.value());
            if(dataInfo.dataSlab) selectHyperslab(dataInfo.h5Space.value(), dataInfo.dataSlab.value());
            h5pp::hdf5::assertReadTypeIsLargeEnough<DataType>(dsetInfo.h5Type.value());
            h5pp::hdf5::assertReadSpaceIsLargeEnough(data, dataInfo.h5Space.value(), dsetInfo.h5Type.value());
            h5pp::hdf5::assertSpacesEqual<DataType>(dataInfo.h5Space.value(), dsetInfo.h5Space.value(), dsetInfo.h5Type.value());
        } catch(const std::exception &ex) {
            throw h5pp::runtime_error("Error reading dataset [{}]:\n{}", dsetInfo.dsetPath.value(), ex.what());
        }
        //        h5pp::hdf5::assertBytesPerElemMatch<DataType>(dsetInfo.h5Type.value());
        herr_t retval = 0;

        // Get the memory address to the data buffer
        [[maybe_unused]] auto dataPtr = h5pp::util::getVoidPointer<void *>(data);

        // Read the data
        if constexpr(type::sfinae::is_text_v<DataType> or type::sfinae::has_text_v<DataType>) {
            // When H5T_VARIABLE,
            //      1) H5Dread expects [const char **], which is what we get from vdata.data().
            //      2) H5Dread allocates memory on each const char * which has to be reclaimed later.
            // Otherwise,
            //      1) H5Dread expects [char *], i.e. *vdata.data()
            //      2) Allocation on char * must be done before reading.

            if(H5Tis_variable_str(dsetInfo.h5Type.value())) {
                hssize_t size = H5Sget_select_npoints(dsetInfo.h5Space.value());
                if(size < 0) throw h5pp::runtime_error("H5S_select_npoints: failed on dataset [{}]", dsetInfo.dsetPath.value());
                std::vector<h5pp::vstr_t> vdata(type::safe_cast<size_t>(size));
                // HDF5 allocates space for each string in vdata
                retval = H5Dread(dsetInfo.h5Dset.value(),
                                 dsetInfo.h5Type.value(),
                                 H5S_ALL,
                                 dsetInfo.h5Space.value(),
                                 plists.dsetXfer,
                                 vdata.data());
                // Now vdata contains the whole dataset, and we need to put the data into the user-given container.
                if constexpr(std::is_same_v<DataType, std::string> or type::sfinae::is_vstr_v<DataType>) {
                    // A vector of strings (vdata) can be put into a single string (data) with entries separated by new-lines
                    data.clear();
                    for(size_t i = 0; i < vdata.size(); i++) {
                        if(!vdata.empty() and vdata[i].data() != nullptr) data.append(vdata[i].c_str());
                        if(i < vdata.size() - 1) data.append("\n");
                    }
                } else if constexpr(type::sfinae::has_resize_v<DataType> and (type::sfinae::is_container_of_v<DataType, h5pp::vstr_t> or
                                                                              type::sfinae::is_container_of_v<DataType, std::string>)) {
                    data.clear();
                    data.resize(vdata.size());
                    for(size_t i = 0; i < data.size(); i++) data[i] = vdata[i];
                } else {
                    static_assert(type::sfinae::unrecognized_type_v<DataType> and
                                  "To read text-data, please use h5pp::vstr_t, std::string or a container of them such as std::vector");
                }
            } else {
                // All the elements in the dataset have the same string size
                // The whole dataset is read into a contiguous block of memory.
                size_t      bytesPerString = H5Tget_size(dsetInfo.h5Type.value()); // Includes null terminator
                auto        size           = H5Sget_select_npoints(dsetInfo.h5Space.value());
                std::string fdata;
                fdata.resize(type::safe_cast<size_t>(size) * bytesPerString);
                retval = H5Dread(dsetInfo.h5Dset.value(),
                                 dsetInfo.h5Type.value(),
                                 dataInfo.h5Space.value(),
                                 dsetInfo.h5Space.value(),
                                 plists.dsetXfer,
                                 fdata.data());
                // Now fdata contains the whole dataset, and we need to put the data into the user-given container.
                if constexpr(std::is_same_v<DataType, std::string> or type::sfinae::is_vstr_v<DataType>) {
                    // A vector of strings (fdata) can be put into a single string (data) with entries separated by new-lines
                    data.clear();
                    for(size_t i = 0; i < type::safe_cast<size_t>(size); i++) {
                        data.append(fdata.substr(i * bytesPerString, bytesPerString));
                        if(data.size() < fdata.size() - 1) data.append("\n");
                    }
                    data.erase(std::find(data.begin(), data.end(), '\0'), data.end()); // Prune all but the last null terminator
                } else if constexpr(type::sfinae::has_resize_v<DataType> and (type::sfinae::is_container_of_v<DataType, h5pp::vstr_t> or
                                                                              type::sfinae::is_container_of_v<DataType, std::string>)) {
                    if(data.size() != type::safe_cast<size_t>(size)) {
                        throw h5pp::runtime_error("Given container of strings has the wrong size: dset size {} | container size {}",
                                                  size,
                                                  data.size());
                    }
                    for(size_t i = 0; i < type::safe_cast<size_t>(size); i++) {
                        // Each data[i] has type std::string, so we can use the std::string constructor to copy data
                        data[i] = std::string(fdata.data() + i * bytesPerString, bytesPerString);
                        // Prune away all null terminators except the last one
                        data[i].erase(std::find(data[i].begin(), data[i].end(), '\0'), data[i].end());
                    }
                } else {
                    static_assert(type::sfinae::unrecognized_type_v<DataType> and
                                  "To read text-data, please use h5pp::vstr_t, std::string or a container of them such as std::vector");
                }
            }
        } else {
            retval = H5Dread(dsetInfo.h5Dset.value(),
                             dsetInfo.h5Type.value(),
                             dataInfo.h5Space.value(),
                             dsetInfo.h5Space.value(),
                             plists.dsetXfer,
                             dataPtr);

            if(retval < 0) {
                throw h5pp::runtime_error("Failed to read from dataset \n\t {} \n into memory \n\t {}",
                                          dsetInfo.string(),
                                          dataInfo.string());
            }
            /* Detect if any VLEN arrays were read, that would have to be reclaimed/free'd later */
            if(util::should_track_vlen_reclaims<DataType>(dsetInfo.h5Type.value(), plists)) {
                dsetInfo.reclaimInfo = h5pp::ReclaimInfo::Reclaim(dsetInfo.h5Type.value(),
                                                                  dataInfo.h5Space.value(),
                                                                  plists.dsetXfer,
                                                                  dataPtr,
                                                                  dsetInfo.dsetPath.value());
            }
        }
    }

    template<typename DataType>
    void writeAttribute(const DataType &data, const DataInfo &dataInfo, const AttrInfo &attrInfo) {
        static_assert(not type::sfinae::is_h5pp_id<DataType>);
#ifdef H5PP_USE_EIGEN3
        if constexpr(type::sfinae::is_eigen_colmajor_v<DataType> and not type::sfinae::is_eigen_1d_v<DataType>) {
            h5pp::logger::log->debug("Converting attribute data to row-major storage order");
            const auto tempRowm = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::writeAttribute(tempRowm, dataInfo, attrInfo);
            return;
        }
#endif
        try {
            dataInfo.assertWriteReady();
            attrInfo.assertWriteReady();
            h5pp::logger::log->trace("Writing from memory    {}", dataInfo.string(h5pp::logger::logIf(LogLevel::trace)));
            h5pp::logger::log->trace("Writing into attribute {}", attrInfo.string(h5pp::logger::logIf(LogLevel::trace)));
            if(attrInfo.attrSlab) selectHyperslab(attrInfo.h5Space.value(), attrInfo.attrSlab.value());
            if(dataInfo.dataSlab) selectHyperslab(dataInfo.h5Space.value(), dataInfo.dataSlab.value());
            h5pp::hdf5::assertWriteBufferIsLargeEnough(data, dataInfo.h5Space.value(), attrInfo.h5Type.value());
            h5pp::hdf5::assertBytesPerElemMatch<DataType>(attrInfo.h5Type.value());
            h5pp::hdf5::assertSpacesEqual<DataType>(dataInfo.h5Space.value(), attrInfo.h5Space.value(), attrInfo.h5Type.value());
        } catch(const std::exception &ex) {
            throw h5pp::runtime_error("Error writing to attribute [{}] in link [{}]:\n{}",
                                      attrInfo.attrName.value(),
                                      attrInfo.linkPath.value(),
                                      ex.what());
        }

        herr_t retval = 0;

        // Get the memory address to the data buffer
        [[maybe_unused]] auto dataPtr = h5pp::util::getVoidPointer<const void *>(data);

        if constexpr(type::sfinae::is_text_v<DataType> or type::sfinae::has_text_v<DataType>) {
            auto vec = getCharPtrVector(data);
            if(H5Tis_variable_str(attrInfo.h5Type->value()) > 0)
                retval = H5Awrite(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), vec.data());
            else retval = H5Awrite(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), *vec.data());
        } else {
            retval = H5Awrite(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), dataPtr);
        }

        if(retval < 0) {
            throw h5pp::runtime_error("Failed to write into attribute \n\t {} \n from memory \n\t {}",
                                      attrInfo.string(),
                                      dataInfo.string());
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    void readAttribute(DataType &data, const DataInfo &dataInfo, const AttrInfo &attrInfo, const PropertyLists &plists = PropertyLists()) {
        // Transpose the data container before reading
#ifdef H5PP_USE_EIGEN3
        if constexpr(type::sfinae::is_eigen_colmajor_v<DataType> and not type::sfinae::is_eigen_1d_v<DataType>) {
            h5pp::logger::log->debug("Converting data to row-major storage order");
            auto tempRowMajor = eigen::to_RowMajor(data); // Convert to Row Major first;
            h5pp::hdf5::readAttribute(tempRowMajor, dataInfo, attrInfo);
            data = eigen::to_ColMajor(tempRowMajor);
            return;
        }
#endif
        try {
            dataInfo.assertReadReady();
            attrInfo.assertReadReady();
            h5pp::logger::log->trace("Reading into memory {}", dataInfo.string(h5pp::logger::logIf(LogLevel::trace)));
            h5pp::logger::log->trace("Reading from file   {}", attrInfo.string(h5pp::logger::logIf(LogLevel::trace)));
            if(attrInfo.attrSlab) selectHyperslab(attrInfo.h5Space.value(), attrInfo.attrSlab.value());
            if(dataInfo.dataSlab) selectHyperslab(dataInfo.h5Space.value(), dataInfo.dataSlab.value());
            h5pp::hdf5::assertReadSpaceIsLargeEnough(data, dataInfo.h5Space.value(), attrInfo.h5Type.value());
            h5pp::hdf5::assertBytesPerElemMatch<DataType>(attrInfo.h5Type.value());
            h5pp::hdf5::assertSpacesEqual<DataType>(dataInfo.h5Space.value(), attrInfo.h5Space.value(), attrInfo.h5Type.value());
        } catch(const std::exception &ex) {
            throw h5pp::runtime_error("Error reading attribute [{}] from link [{}]:\n{}",
                                      attrInfo.attrName.value(),
                                      attrInfo.linkPath.value(),
                                      ex.what());
        }

        herr_t                retval  = 0;
        // Get the memory address to the data buffer
        [[maybe_unused]] auto dataPtr = h5pp::util::getVoidPointer<void *>(data);

        // Read the data
        if constexpr(type::sfinae::is_text_v<DataType> or type::sfinae::has_text_v<DataType>) {
            // When H5T_VARIABLE,
            //      1) H5Aread expects [const char **], which is what we get from vdata.data().
            //      2) H5Aread allocates memory on each const char * which has to be reclaimed later.
            // Otherwise,
            //      1) H5Aread expects [char *], i.e. *vdata.data()
            //      2) Allocation on char * must be done before reading.
            if(H5Tis_variable_str(attrInfo.h5Type.value()) > 0) {
                auto                size = H5Sget_select_npoints(attrInfo.h5Space.value());
                std::vector<char *> vdata(type::safe_cast<size_t>(size)); // Allocate pointers for "size" number of strings
                // HDF5 allocates space for each string
                retval = H5Aread(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), vdata.data());
                // Now vdata contains the whole dataset, and we need to put the data into the user-given container.
                if constexpr(std::is_same_v<DataType, std::string>) {
                    // A vector of strings (vdata) can be put into a single string (data) with entries separated by new-lines
                    data.clear();
                    for(size_t i = 0; i < vdata.size(); i++) {
                        if(!vdata.empty() and vdata[i] != nullptr) data.append(vdata[i]);
                        if(i < vdata.size() - 1) data.append("\n");
                    }
                } else if constexpr(type::sfinae::is_container_of_v<DataType, std::string> and type::sfinae::has_resize_v<DataType>) {
                    data.clear();
                    data.resize(vdata.size());
                    for(size_t i = 0; i < data.size(); i++) data[i] = std::string(vdata[i]);
                } else {
                    throw h5pp::runtime_error(
                        "To read text-data, please use std::string or a container of std::string like std::vector<std::string>");
                }
                // Free memory allocated by HDF5
#if H5_VERSION_GE(1, 12, 0)
                herr_t reclaim_err = H5Treclaim(attrInfo.h5Type.value(), attrInfo.h5Space.value(), H5P_DEFAULT, vdata.data());
#else
                herr_t reclaim_err = H5Dvlen_reclaim(attrInfo.h5Type.value(), attrInfo.h5Space.value(), H5P_DEFAULT, vdata.data());
#endif
                if(reclaim_err < 0) h5pp::runtime_error("readAttribute: failed to reclaim variable-length array buffer");
            } else {
                // All the elements in the dataset have the same string size
                // The whole dataset is read into a contiguous block of memory.
                size_t      bytesPerString = H5Tget_size(attrInfo.h5Type.value()); // Includes null terminator
                auto        size           = H5Sget_select_npoints(attrInfo.h5Space.value());
                std::string fdata;
                fdata.resize(type::safe_cast<size_t>(size) * bytesPerString);
                retval = H5Aread(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), fdata.data());
                // Now fdata contains the whole dataset, and we need to put the data into the user-given container.
                if constexpr(std::is_same_v<DataType, std::string>) {
                    // A vector of strings (fdata) can be put into a single string (data) with entries separated by new-lines
                    data.clear();
                    for(size_t i = 0; i < type::safe_cast<size_t>(size); i++) {
                        data.append(fdata.substr(i * bytesPerString, bytesPerString));
                        if(data.size() < fdata.size() - 1) data.append("\n");
                    }
                } else if constexpr(type::sfinae::is_container_of_v<DataType, std::string> and type::sfinae::has_resize_v<DataType>) {
                    data.clear();
                    data.resize(type::safe_cast<size_t>(size));
                    for(size_t i = 0; i < type::safe_cast<size_t>(size); i++) data[i] = fdata.substr(i * bytesPerString, bytesPerString);
                } else {
                    throw h5pp::runtime_error(
                        "To read text-data, please use std::string or a container of std::string like std::vector<std::string>");
                }
            }
            if constexpr(std::is_same_v<DataType, std::string>)
                data.erase(std::find(data.begin(), data.end(), '\0'), data.end()); // Prune all but the last null terminator
        } else {
            retval = H5Aread(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), dataPtr);
            /* Detect if any VLEN arrays were read, that would have to be reclaimed/free'd later */
            if(util::should_track_vlen_reclaims<DataType>(attrInfo.h5Type.value(), plists)) {
                attrInfo.reclaimInfo =
                    h5pp::ReclaimInfo::Reclaim(attrInfo.h5Type.value(),
                                               attrInfo.h5Space.value(),
                                               plists.dsetXfer,
                                               dataPtr,
                                               h5pp::format("{}|{}", attrInfo.linkPath.value(), attrInfo.attrName.value()));
            }
        }
        if(retval < 0)
            throw h5pp::runtime_error("Failed to read from attribute \n\t {} \n into memory \n\t {}", attrInfo.string(), dataInfo.string());
    }

    [[nodiscard]] inline bool fileIsValid(const fs::path &filePath) {
        return fs::exists(filePath) and H5Fis_hdf5(filePath.string().c_str()) > 0;
    }

    [[nodiscard]] inline fs::path getAvailableFileName(const fs::path &filePath) {
        int      i           = 1;
        fs::path newFileName = filePath;
        while(fs::exists(newFileName))
            newFileName.replace_filename(filePath.stem().string() + "-" + std::to_string(i++) + filePath.extension().string());
        return newFileName;
    }

    [[nodiscard]] inline fs::path getBackupFileName(const fs::path &filePath) {
        int      i           = 1;
        fs::path newFilePath = filePath;
        while(fs::exists(newFilePath)) newFilePath.replace_extension(filePath.extension().string() + ".bak_" + std::to_string(i++));
        return newFilePath;
    }

    [[nodiscard]] inline h5pp::FileAccess convertFileAccessFlags(unsigned int H5F_ACC_FLAGS) {
        h5pp::FileAccess access = h5pp::FileAccess::RENAME;
        if((H5F_ACC_FLAGS & (H5F_ACC_TRUNC | H5F_ACC_EXCL)) == (H5F_ACC_TRUNC | H5F_ACC_EXCL))
            throw h5pp::runtime_error("File access modes H5F_ACC_EXCL and H5F_ACC_TRUNC are mutually exclusive");
        if((H5F_ACC_FLAGS & H5F_ACC_RDONLY) == H5F_ACC_RDONLY) access = h5pp::FileAccess::READONLY;
        if((H5F_ACC_FLAGS & H5F_ACC_RDWR) == H5F_ACC_RDWR) access = h5pp::FileAccess::READWRITE;
        if((H5F_ACC_FLAGS & H5F_ACC_EXCL) == H5F_ACC_EXCL) access = h5pp::FileAccess::COLLISION_FAIL;
        if((H5F_ACC_FLAGS & H5F_ACC_TRUNC) == H5F_ACC_TRUNC) access = h5pp::FileAccess::REPLACE;
        return access;
    }

    [[nodiscard]] inline unsigned int convertFileAccessFlags(h5pp::FileAccess access) {
        unsigned int H5F_ACC_MODE = H5F_ACC_RDONLY;
        if(access == h5pp::FileAccess::COLLISION_FAIL) H5F_ACC_MODE |= H5F_ACC_EXCL;
        if(access == h5pp::FileAccess::REPLACE) H5F_ACC_MODE |= H5F_ACC_TRUNC;
        if(access == h5pp::FileAccess::RENAME) H5F_ACC_MODE |= H5F_ACC_TRUNC;
        if(access == h5pp::FileAccess::READONLY) H5F_ACC_MODE |= H5F_ACC_RDONLY;
        if(access == h5pp::FileAccess::READWRITE) H5F_ACC_MODE |= H5F_ACC_RDWR;
        return H5F_ACC_MODE;
    }

    [[nodiscard]] inline fs::path
        createFile(const h5pp::fs::path &filePath_, const h5pp::FileAccess &access, const PropertyLists &plists = PropertyLists()) {
        fs::path filePath = fs::absolute(filePath_);
        fs::path fileName = filePath_.filename();
        if(fs::exists(filePath)) {
            if(not fileIsValid(filePath)) h5pp::logger::log->debug("Pre-existing file may be corrupted [{}]", filePath.string());
            if(access == h5pp::FileAccess::READONLY) return filePath;
            if(access == h5pp::FileAccess::COLLISION_FAIL)
                throw h5pp::runtime_error("[COLLISION_FAIL]: Previous file exists with the same name [{}]", filePath.string());
            if(access == h5pp::FileAccess::RENAME) {
                auto newFilePath = getAvailableFileName(filePath);
                h5pp::logger::log->info("[RENAME]: Previous file exists. Choosing a new file name [{}] --> [{}]",
                                        filePath.filename().string(),
                                        newFilePath.filename().string());
                filePath = newFilePath;
                fileName = filePath.filename();
            }
            if(access == h5pp::FileAccess::READWRITE) return filePath;
            if(access == h5pp::FileAccess::BACKUP) {
                auto backupPath = getBackupFileName(filePath);
                h5pp::logger::log->info("[BACKUP]: Backing up existing file [{}] --> [{}]",
                                        filePath.filename().string(),
                                        backupPath.filename().string());
                fs::rename(filePath, backupPath);
            }
            if(access == h5pp::FileAccess::REPLACE) {} // Do nothing
        } else {
            if(access == h5pp::FileAccess::READONLY) throw h5pp::runtime_error("[READONLY]: File does not exist [{}]", filePath.string());
            if(access == h5pp::FileAccess::COLLISION_FAIL) {} // Do nothing
            if(access == h5pp::FileAccess::RENAME) {}         // Do nothing
            if(access == h5pp::FileAccess::READWRITE) {}      // Do nothing
            if(access == h5pp::FileAccess::BACKUP) {}         // Do nothing
            if(access == h5pp::FileAccess::REPLACE) {}        // Do nothing
            try {
                if(fs::create_directories(filePath.parent_path()))
                    h5pp::logger::log->trace("Created directory: {}", filePath.parent_path().string());
                else h5pp::logger::log->trace("Directory already exists: {}", filePath.parent_path().string());
            } catch(std::exception &ex) { throw h5pp::runtime_error("Failed to create directory: {}", ex.what()); }
        }

        // One last sanity check
        if(access == h5pp::FileAccess::READONLY)
            throw h5pp::logic_error("About to create/truncate a file even though READONLY was specified. This is a programming error!");

        // Go ahead
        hid_t file = H5Fcreate(filePath.string().c_str(), H5F_ACC_TRUNC, plists.fileCreate, plists.fileAccess);
        if(file < 0) {
            throw h5pp::runtime_error("Failed to create file [{}]\n\t\t Check that you have the right file access permissions and that the "
                                      "file is not locked by another program",
                                      filePath.string());
        }

        H5Fclose(file);
        return fs::canonical(filePath);
    }

    inline void createTable(TableInfo &info, const PropertyLists &plists = PropertyLists()) {
        info.assertCreateReady();
        h5pp::logger::log->debug("Creating table [{}] | num fields {} | record size {} bytes | compression {}",
                                 info.tablePath.value(),
                                 info.numFields.value(),
                                 info.recordBytes.value(),
                                 info.compression.value());

        if(not info.tableExists) info.tableExists = checkIfLinkExists(info.getLocId(), info.tablePath.value(), plists.linkAccess);
        if(info.tableExists.value()) {
            h5pp::logger::log->debug("Table [{}] already exists", info.tablePath.value());
            return;
        }
        createGroup(info.getLocId(),
                    info.tableGroupName.value(),
                    std::nullopt,
                    plists); // The existence of the group has to be checked, unfortunately

        // Copy member type data to a vector of hid_t for compatibility.
        // Note that there is no need to close thes hid_t since info will close them.
        std::vector<hid_t> fieldTypesHidT(info.fieldTypes.value().begin(), info.fieldTypes.value().end());

        // Copy member name data to a vector of const char * for compatibility
        std::vector<const char *> fieldNames;
        for(auto &name : info.fieldNames.value()) fieldNames.push_back(name.c_str());
        int    compression = info.compression.value() == 0 ? 0 : 1; // Only true/false (1/0). Is set to level 6 in HDF5 sources
        herr_t retval      = H5TBmake_table(util::safe_str(info.tableTitle.value()).c_str(),
                                       info.getLocId(),
                                       util::safe_str(info.tablePath.value()).c_str(),
                                       info.numFields.value(),
                                       info.numRecords.value(),
                                       info.recordBytes.value(),
                                       fieldNames.data(),
                                       info.fieldOffsets.value().data(),
                                       fieldTypesHidT.data(),
                                       info.chunkDims.value()[0],
                                       nullptr,
                                       compression,
                                       nullptr);
        if(retval < 0) throw h5pp::runtime_error("Could not create table {}\n{}", info.string(), info.string_fields());

        // Setup fields so that this TableInfo can be reused for read/write after
        info.compression  = compression == 0 ? 0 : 6; // Set to 0/6 so that users can read this field later
        info.tableExists  = true;
        info.h5Dset       = hdf5::openLink<hid::h5d>(info.getLocId(), info.tablePath.value(), info.tableExists, plists.dsetAccess);
        info.h5Type       = H5Dget_type(info.h5Dset.value());
        info.recordBytes  = H5Tget_size(info.h5Type.value());
        info.h5DsetCreate = H5Dget_create_plist(info.h5Dset.value());
        info.h5DsetAccess = H5Dget_access_plist(info.h5Dset.value());
        info.assertReadReady();
        info.assertWriteReady();
        h5pp::logger::log->trace("Successfully created table [{}]", info.tablePath.value());
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    inline void readTableRecords(DataType             &data,
                                 const TableInfo      &info,
                                 std::optional<size_t> offset = std::nullopt,
                                 std::optional<size_t> extent = std::nullopt,
                                 const PropertyLists  &plists = PropertyLists()) {
        /*
         *  This function replaces H5TBread_records() and avoids creating expensive temporaries for the dataset id and type id for the
         * compound table type.
         *
         */

        // If none of offset or extent are given:
        //          If data resizeable: offset = 0, extent = totalRecords
        //          If data not resizeable: offset = last record, extent = 1.
        // If offset given but extent is not:
        //          If data resizeable -> read from offset to the end
        //          If data not resizeable -> read a single record starting from offset
        // If extent given but offset is not -> read the last extent records

        info.assertReadReady();
        if constexpr(std::is_same_v<DataType, std::vector<std::byte>>) {
            if(not extent)
                throw h5pp::runtime_error("Optional argument [extent] is required when reading std::vector<std::byte> from table");
        }

        if(not offset and not extent) {
            if constexpr(type::sfinae::has_resize_v<DataType>) {
                offset = 0;
                extent = info.numRecords.value();
            } else {
                offset = info.numRecords.value() - 1;
                extent = 1;
            }
        } else if(offset and not extent) {
            if constexpr(type::sfinae::has_resize_v<DataType>) extent = info.numRecords.value() - offset.value();
            else extent = 1;
        } else if(extent and not offset) {
            offset = info.numRecords.value() - extent.value();
        }

        // Sanity check
        if(offset.value() + extent.value() > info.numRecords.value()) {
            throw h5pp::logic_error("readTableRecords: requested offset {} and extent {} is out of bounds for table with {} records: [{}]",
                                    offset.value(),
                                    extent.value(),
                                    info.numRecords.value(),
                                    info.tablePath.value());
        }

        if constexpr(std::is_same_v<DataType, std::vector<std::byte>>) {
            auto newsize = info.recordBytes.value() * extent.value();
            if(data.size() != newsize){
                h5pp::logger::log->trace("Resizing std::vector<std::byte> to size {}", newsize);
                data.resize(newsize);
            }
        } else {
            size_t dtypeSize = util::getBytesPerElem<DataType>();
            if(dtypeSize != info.recordBytes.value()) {
                throw h5pp::runtime_error("readTableRecords: Type size mismatch:\n"
                                          "Buffer [{}] has {} bytes/element\n"
                                          "Table [{}] has {} bytes/record",
                                          type::sfinae::type_name<DataType>(),
                                          dtypeSize,
                                          info.tablePath.value(),
                                          info.recordBytes.value());
            }
            h5pp::util::resizeData(data, {extent.value()});
        }
        if(h5pp::logger::log->level() <= 1) {
            h5pp::logger::log->debug("readTableRecords: offset {} | extent {} | records {} | {} bytes/record | buffer [{}] | table [{}] ",
                                     offset.value(),
                                     extent.value(),
                                     info.numRecords.value(),
                                     info.recordBytes.value(),
                                     type::sfinae::type_name<DataType>(),
                                     info.tablePath.value());
        }

        // Last sanity check. If there are no records to read, just return;
        if(extent.value() == 0) return;
        if(info.numRecords.value() == 0) return;

        /* Step 1: Get the dataset and memory spaces */
        std::array<hsize_t, 1> dataDims  = {extent.value()};                  /* create a simple memory data space */
        hid::h5s               dsetSpace = H5Dget_space(info.h5Dset.value()); /* get a copy of the new file data space for writing */
        hid::h5s               dataSpace = H5Screate_simple(dataDims.size(), dataDims.data(), nullptr);

        /* Step 2: draw a region in the dataset */
        std::array<hsize_t, 1> dsetOffset = {offset.value()};
        std::array<hsize_t, 1> dsetExtent = dataDims;
        // Select the region in the dataset space
        H5Sselect_hyperslab(dsetSpace, H5S_SELECT_SET, dsetOffset.data(), nullptr, dsetExtent.data(), nullptr);

        /* Step 3: read the records */
        // Get the memory address to the data buffer
        auto   dataPtr = h5pp::util::getVoidPointer<void *>(data);
        herr_t retval  = H5Dread(info.h5Dset.value(), info.h5Type.value(), dataSpace, dsetSpace, plists.dsetXfer, dataPtr);
        if(retval < 0) throw h5pp::runtime_error("Failed to read data from table [{}]", info.tablePath.value());

        /* Step 4: Detect if any VLEN arrays were read, that would have to be reclaimed/free'd later */
        if(util::should_track_vlen_reclaims<DataType>(info.h5Type.value(), plists))
            info.reclaimInfo = h5pp::ReclaimInfo::Reclaim(info.h5Type.value(), dataSpace, plists.dsetXfer, dataPtr, info.tablePath.value());
    }

    template<typename DataType>
    inline void writeTableRecords(const DataType        &data,
                                  TableInfo             &info,
                                  hsize_t                offset = 0,
                                  std::optional<hsize_t> extent = std::nullopt,
                                  const PropertyLists   &plists = PropertyLists()) {
        /*
         *  This function replaces H5TBwrite_records() and avoids creating expensive temporaries for the dataset id and type id for the
         * compound table type. In addition, it has the ability to extend the existing the dataset if the incoming data larger than the
         * current bound
         */
        info.assertWriteReady();
        offset = util::wrapUnsigned(offset, info.numRecords.value()); // Allows python style negative indexing
        if(not extent) {
            if constexpr(std::is_same_v<DataType, std::vector<std::byte>>) {
                std::string errmsg;
                if(data.size() < info.recordBytes.value()) errmsg.append("is smaller than");
                else if(data.size() % info.recordBytes.value() != 0) errmsg.append("is not a multiple of");
                if(not errmsg.empty()) {
                    throw h5pp::logic_error("The data buffer size [{} : {} bytes] {} the record size of table [{} : {} bytes]",
                                            h5pp::type::sfinae::type_name<DataType>(),
                                            data.size(),
                                            errmsg,
                                            info.tablePath.value(),
                                            info.recordBytes.value());
                }
                extent = data.size() / info.recordBytes.value();
            } else {
                extent = h5pp::util::getSize(data);
            }
        }

        if(extent.value() == 0)
            h5pp::logger::log->warn("Given 0 records to write to table [{}]. This is likely an error.", info.tablePath.value());

        /* Step 0: define clear quantities */
        const hsize_t numRecordsWrt = extent.value();
        const hsize_t numRecordsOld = info.numRecords.value();
        const hsize_t numRecordsNew = std::max<size_t>(offset + numRecordsWrt, numRecordsOld);

        if constexpr(std::is_same_v<DataType, std::vector<std::byte>>) {
            size_t bufferRecords = data.size() / info.recordBytes.value();
            if(bufferRecords != extent.value()) {
                throw h5pp::runtime_error("writeTableRecords: Buffer size mismatch:\n"
                                          "    table         : [{}]\n"
                                          "    table record  : {} bytes\n"
                                          "    buffer type   : {}\n"
                                          "    buffer size   : {}\n"
                                          "    buffer has    : {} records\n"
                                          "    write extent  : {} records",
                                          info.tablePath.value(),
                                          info.recordBytes.value(),
                                          type::sfinae::type_name<DataType>(),
                                          data.size(),
                                          bufferRecords,
                                          extent.value());
            }

        } else {
            // Make sure the given data type size matches the table record type size.
            // If there is a mismatch here it can cause horrible bugs/segfaults
            size_t dtypeSize = util::getBytesPerElem<DataType>();
            if(dtypeSize != info.recordBytes.value()) {
                throw h5pp::runtime_error("writeTableRecords: Type size mismatch:\n"
                                          "Buffer [{}] has {} bytes/element\n"
                                          "Table [{}] has {} bytes/record",
                                          type::sfinae::type_name<DataType>(),
                                          dtypeSize,
                                          info.tablePath.value(),
                                          info.recordBytes.value());
            }
        }

        /* Step 1: set the new table size if necessary */
        if(numRecordsNew > numRecordsOld) setTableSize(info, numRecordsNew);
        /* Step 2: draw a hyperslab in the dataset */
        h5pp::Hyperslab dsetSlab;
        dsetSlab.offset = {offset};
        dsetSlab.extent = {numRecordsWrt};

        /* and a hyperslab for the data */
        h5pp::Hyperslab dataSlab;
        dataSlab.offset = {0};
        dataSlab.extent = {numRecordsWrt};
        herr_t   retval = 0;
        hid::h5s dsetSpace;
        hid::h5s dataSpace;

        // Log
        if(h5pp::logger::log->level() <= 1) {
            h5pp::logger::log->debug(
                "writeTableRecords: offset {} | extent {} | {} bytes/record | buffer [{}] | table [{}] (size {} -> {} records) ",
                offset,
                extent.value(),
                info.recordBytes.value(),
                type::sfinae::type_name<DataType>(),
                info.tablePath.value(),
                numRecordsOld,
                numRecordsNew);
        }

        if constexpr(has_direct_chunk) {
            if(use_direct_chunk) {
                /* Step 3: write the records */
                H5Dwrite_chunkwise(data,
                                   info.h5Dset.value(),
                                   info.h5Type.value(),
                                   info.h5DsetCreate.value(),
                                   plists.dsetXfer,
                                   {numRecordsNew},
                                   info.chunkDims.value(),
                                   dsetSlab,
                                   dataSlab);
            } else {
                /* Step 2: Get the dataset and memory spaces */
                dsetSpace = H5Dget_space(info.h5Dset.value()); /* get a copy of the new file data space for writing */
                dataSpace = H5Screate_simple(type::safe_cast<int>(dataSlab.extent->size()),
                                             dataSlab.extent->data(),
                                             nullptr); /* create a simple memory data space */
                //
                /* Step 3: Select the region in the dataset space*/
                H5Sselect_hyperslab(dsetSpace, H5S_SELECT_SET, dsetSlab.offset->data(), nullptr, dsetSlab.extent->data(), nullptr);

                /* Step 4: write the records */
                // Get the memory address to the data buffer
                auto dataPtr = h5pp::util::getVoidPointer<const void *>(data);
                retval       = H5Dwrite(info.h5Dset.value(), info.h5Type.value(), dataSpace, dsetSpace, plists.dsetXfer, dataPtr);
            }

        } else {
            /* Step 2: Get the dataset and memory spaces */
            dsetSpace = H5Dget_space(info.h5Dset.value()); /* get a copy of the new file data space for writing */
            dataSpace = H5Screate_simple(type::safe_cast<int>(dataSlab.extent->size()),
                                         dataSlab.extent->data(),
                                         nullptr); /* create a simple memory data space */
            //
            /* Step 3: Select the region in the dataset space*/
            H5Sselect_hyperslab(dsetSpace, H5S_SELECT_SET, dsetSlab.offset->data(), nullptr, dsetSlab.extent->data(), nullptr);

            /* Step 4: write the records */
            // Get the memory address to the data buffer
            auto dataPtr = h5pp::util::getVoidPointer<const void *>(data);
            retval       = H5Dwrite(info.h5Dset.value(), info.h5Type.value(), dataSpace, dsetSpace, plists.dsetXfer, dataPtr);
        }

        if(retval < 0) {
            throw h5pp::runtime_error("writeTableRecords : Failed to write data to table\n"
                                      "    table         : [{}]\n"
                                      "    table size    : {} records (write would add {})\n"
                                      "    table record  : {} bytes\n"
                                      "    buffer type   : {}\n"
                                      "    buffer size   : {}\n"
                                      "    buffer items  : {} bytes per [{}]\n"
                                      "    write offset  : {}\n"
                                      "    write extent  : {}\n"
                                      "    dataspace     {}\n"
                                      "    dsetspace     {}",
                                      info.tablePath.value(),
                                      numRecordsOld,
                                      numRecordsNew - numRecordsOld,
                                      info.recordBytes.value(),
                                      type::sfinae::type_name<DataType>(),
                                      util::getSize(data),
                                      util::getBytesPerElem<DataType>(),
                                      type::sfinae::value_type_name<DataType>(),
                                      offset,
                                      extent.value(),
                                      Hyperslab(dataSpace).string(),
                                      Hyperslab(dsetSpace).string());
        }
    }

    inline void copyTableRecords(const h5pp::TableInfo &srcInfo,
                                 hsize_t                srcOffset,
                                 hsize_t                srcExtent,
                                 h5pp::TableInfo       &tgtInfo,
                                 hsize_t                tgtOffset,
                                 const PropertyLists   &plists = PropertyLists()) {
        srcInfo.assertReadReady();
        tgtInfo.assertWriteReady();
        srcOffset = util::wrapUnsigned(srcOffset, srcInfo.numRecords.value()); // Allows python style negative indexing
        tgtOffset = util::wrapUnsigned(tgtOffset, tgtInfo.numRecords.value()); // Allows python style negative indexing

        if(h5pp::logger::log->level() <= 1) {
            std::string fileLogInfo;
            if(getName(srcInfo.h5File.value()) != getName(tgtInfo.h5File.value())) fileLogInfo = "|on external file";
            h5pp::logger::log->debug("copyTableRecords: src: table [{}] offset {} records {}  | dst: table [{}] offset {} records {} | "
                                     "extent {} | {} bytes/record {}",
                                     srcInfo.tablePath.value(),
                                     srcOffset,
                                     srcInfo.numRecords.value(),
                                     tgtInfo.tablePath.value(),
                                     tgtOffset,
                                     tgtInfo.numRecords.value(),
                                     srcExtent,
                                     tgtInfo.recordBytes.value(),
                                     fileLogInfo);
        }

        // Sanity checks for table types
        if(not h5pp::type::H5Tequal_recurse(srcInfo.h5Type.value(), tgtInfo.h5Type.value()))
            throw h5pp::runtime_error("copyTableRecords: table type mismatch");
        if(srcInfo.recordBytes.value() != tgtInfo.recordBytes.value()) {
            throw h5pp::runtime_error("copyTableRecords: table record byte size mismatch \n src {} \n tgt {}",
                                      srcInfo.recordBytes.value(),
                                      tgtInfo.recordBytes.value());
        }
        if(srcInfo.fieldSizes.value() != tgtInfo.fieldSizes.value()) {
            throw h5pp::runtime_error("copyTableRecords: table field sizes mismatch \n src {} \n tgt {}",
                                      srcInfo.fieldSizes.value(),
                                      tgtInfo.fieldSizes.value());
        }
        if(srcInfo.fieldOffsets.value() != tgtInfo.fieldOffsets.value()) {
            throw h5pp::runtime_error("copyTableRecords: table field offsets mismatch \n src {} \n tgt {}",
                                      srcInfo.fieldOffsets.value(),
                                      tgtInfo.fieldOffsets.value());
        }

        // Sanity check for record ranges
        if(srcInfo.numRecords.value() < srcOffset + srcExtent) {
            throw h5pp::runtime_error("copyTableRecords: records on source table are out of bound:\n"
                                      "table [{}]\n"
                                      "table records {}\n"
                                      "read offset {}\n"
                                      "read extent {}",
                                      srcInfo.tablePath.value(),
                                      srcInfo.numRecords.value(),
                                      srcOffset,
                                      srcExtent);
        }

        std::vector<std::byte> data(srcExtent * tgtInfo.recordBytes.value());
        data.resize(srcExtent * tgtInfo.recordBytes.value());
        h5pp::hdf5::readTableRecords(data, srcInfo, srcOffset, srcExtent, plists);
        h5pp::hdf5::writeTableRecords(data, tgtInfo, tgtOffset, srcExtent, plists);
        srcInfo.reclaim(); // In case there were any vlen arrays in a field
    }

    template<typename T>
    inline bool checkIfTableFieldsExists(const hid::h5t &h5Type, const std::vector<T> &fields) {
        static_assert(std::is_integral_v<T> or h5pp::type::sfinae::is_text_v<T>);
        return std::all_of(fields.begin(), fields.end(), [&h5Type](const auto &field) {
            if constexpr(std::is_integral_v<T>) {
                return H5Tget_member_class(h5Type, type::safe_cast<unsigned int>(field)) >= 0;
            } else if constexpr(h5pp::type::sfinae::is_text_v<T>) {
                if constexpr(h5pp::type::sfinae::has_data_v<T>) return H5Tget_member_index(h5Type, field.data()) >= 0;
                else return H5Tget_member_index(h5Type, field) >= 0;
            }
        });
    }
    template<typename T>
    inline bool checkIfTableFieldsExists(const hid::h5d &h5Dset, const std::vector<T> &fields) {
        hid::h5t h5Type = H5Dget_type(h5Dset);
        return checkIfTableFieldsExists(h5Type, fields);
    }
    template<typename T>
    inline bool checkIfTableFieldsExists(const hid::h5f       &h5File,
                                         std::string_view      tablePath,
                                         const std::vector<T> &fields,
                                         const PropertyLists  &plists = PropertyLists()) {
        auto exists = checkIfLinkExists(h5File, tablePath, plists.linkAccess);
        if(not exists) return false;
        auto h5Dset = hdf5::openLink<hid::h5d>(h5File, tablePath, exists, plists.dsetAccess);
        return checkIfTableFieldsExists(h5Dset, fields);
    }

    template<typename DataType>
    inline void readTableField(DataType              &data,
                               const TableInfo       &info,
                               const hid::h5t        &h5t_fields,
                               std::optional<hsize_t> offset = std::nullopt,
                               std::optional<hsize_t> extent = std::nullopt,
                               const PropertyLists   &plists = PropertyLists()) {
        static_assert(not std::is_const_v<DataType>);
        static_assert(not type::sfinae::is_h5pp_id<DataType>);
        // If none of offset or extent are given:
        //          If data resizeable: offset = 0, extent = totalRecords
        //          If data not resizeable: offset = last record index, extent = 1.
        // If offset given but extent is not:
        //          If data resizeable -> read from offset to the end
        //          If data not resizeable -> read a single record starting from offset
        // If extent given but offset is not -> read the last extent records
        info.assertReadReady();
        if(offset) offset = util::wrapUnsigned(offset.value(), info.numRecords.value()); // Allows python style negative indexing
        hsize_t totalRecords = info.numRecords.value();
        if(not offset and not extent) {
            if constexpr(type::sfinae::has_resize_v<DataType>) {
                offset = 0;
                extent = totalRecords;
            } else {
                offset = totalRecords - 1;
                extent = 1;
            }
        } else if(offset and not extent) {
            if constexpr(type::sfinae::has_resize_v<DataType>) extent = totalRecords - offset.value();
            else extent = 1;
        } else if(extent and not offset) {
            offset = totalRecords - extent.value();
        }

        // Sanity checks
        auto fieldClass = H5Tget_class(h5t_fields);
        if(fieldClass != H5T_class_t::H5T_COMPOUND)
            throw h5pp::logic_error("readTableField: expected HDF5 class H5T_COMPOUND. Got {}", h5pp::enum2str(fieldClass));

        int    nmembers    = H5Tget_nmembers(h5t_fields);
        htri_t detect_vlen = H5Tdetect_class(h5t_fields, H5T_class_t::H5T_VLEN);
        if(detect_vlen > 0 and nmembers > 1) {
            throw h5pp::logic_error("readTableField: h5t_fields has multiple fields, but one or more are variable-length types. h5pp can "
                                    "only read one variable-length field at a time");
        }

        if(offset.value() + extent.value() > info.numRecords.value()) {
            throw h5pp::logic_error("readTableField: requested offset {} and extent {} is out of bounds for table with {} records: [{}]",
                                    offset.value(),
                                    extent.value(),
                                    info.numRecords.value(),
                                    info.tablePath.value());
        }

        if(h5pp::logger::log->level() <= 1) {
            h5pp::logger::log->debug("readTableField: offset {} | extent {} | records {} | {} bytes/record | buffer [{}] | table [{}]",
                                     offset.value(),
                                     extent.value(),
                                     info.numRecords.value(),
                                     info.recordBytes.value(),
                                     type::sfinae::type_name<DataType>(),
                                     info.tablePath.value());
        }
        if constexpr(not h5pp::ndebug) {
            if(h5pp::logger::log->level() == 0) {
                auto h5t_info = getH5TInfo(h5t_fields);
                h5pp::logger::log->trace("readTableField: table [{}] | {}{}{}",
                                         info.tablePath.value(),
                                         h5t_info.string(),
                                         h5t_info.numMembers ? "\n" : "",
                                         h5t_info.string_members());
            }
        }

        // Last sanity check. If there are no records to read, just return;
        if(extent.value() == 0) return;
        if(info.numRecords.value() == 0) return;

        /* Step 1: Get the dataset and memory spaces */
        hid::h5s dsetSpace = H5Dget_space(info.h5Dset.value());
        hid::h5s dataSpace = util::getMemSpace(extent.value(), {extent.value()});

        /* Step 2: draw a hyperslab in the dataset */
        h5pp::Hyperslab slab;
        slab.offset = {offset.value()};
        slab.extent = {extent.value()};
        selectHyperslab(dsetSpace, slab, H5S_SELECT_SET);

        /* Step 3: Resize the recipient data buffer */
        if(detect_vlen > 0) {
            auto               size = h5pp::hdf5::getSizeSelected(dsetSpace);
            std::vector<hvl_t> vdata(type::safe_cast<size_t>(size)); // Allocate len/ptr pairs for "size" number of vlen arrays
            // HDF5 allocates space for each vlen array
            herr_t             retval = H5Dread(info.h5Dset.value(), h5t_fields, dataSpace, dsetSpace, plists.dsetXfer, vdata.data());
            if(retval < 0) {
                auto h5t_info = getH5TInfo(h5t_fields);
                h5pp::runtime_error("readTableField: H5Dread failed for variable-length field data\n"
                                    "table [{}] | {}{}{}",
                                    info.tablePath.value(),
                                    h5t_info.string(),
                                    h5t_info.numMembers ? "\n" : "",
                                    h5t_info.string_members());
            }

            // Now vdata contains the dataset selection, and we need to put the data into the user-given container.
            // Depending on the type of the recipient data container, we can copy the data in different ways e.g. plain vector or of vectors
            if constexpr(std::is_same_v<DataType, std::vector<hvl_t>>) {
                data = vdata;
                /* Save metadata so that VLEN allocation can be reclaimed/free'd later */
                if(util::should_track_vlen_reclaims<DataType>(info.h5Type.value(), plists)) {
                    info.reclaimInfo = h5pp::ReclaimInfo::Reclaim(info.h5Type.value(),
                                                                  dataSpace,
                                                                  plists.dsetXfer,
                                                                  vdata.data(),
                                                                  info.tablePath.value());
                }
                return;
            }
            if constexpr(std::is_same_v<DataType, std::vector<std::byte>>) {
                hsize_t bufsize = 0;
                herr_t  buferr  = H5Dvlen_get_buf_size(info.h5Dset.value(), h5t_fields, dsetSpace, &bufsize);
                if(buferr < 0) throw h5pp::runtime_error("Failed to get vlen buffer size");
                data.resize(bufsize);
            }
            [[maybe_unused]] size_t bufoffset = 0;
            for(size_t idx = 0; idx < size; idx++) {
                if constexpr(type::sfinae::has_value_type_v<DataType>) {
                    using value_type = typename DataType::value_type;
                    if constexpr(type::sfinae::is_std_vector_v<value_type>) {
                        // Vector of vectors
                        using inner_value_type = typename DataType::value_type::value_type;
                        auto begin             = static_cast<inner_value_type *>(vdata[idx].p);
                        auto end               = static_cast<inner_value_type *>(vdata[idx].p) + vdata[idx].len;
                        data.emplace_back(begin, end);
                    } else if constexpr(type::sfinae::is_std_vector_v<DataType>) {
                        // Plain vector takes all the data
                        auto begin = static_cast<value_type *>(vdata[idx].p);
                        auto end   = static_cast<value_type *>(vdata[idx].p) + vdata[idx].len;
                        std::copy(begin, end, std::back_inserter(data));
                    } else if constexpr(std::is_same_v<DataType, std::vector<std::byte>>) {
                        // Raw buffer takes all the data
                        hsize_t         bufsize;
                        hid::h5s        elemSpace;
                        h5pp::Hyperslab elemSlab({offset.value() + idx}, {1});
                        selectHyperslab(elemSpace, elemSlab, H5S_SELECT_SET);
                        herr_t buferr = H5Dvlen_get_buf_size(info.h5Dset.value(), h5t_fields, elemSpace, &bufsize);
                        if(buferr < 0) throw h5pp::runtime_error("Failed to get vlen buffer size");
                        std::memcpy(util::getVoidPointer<void *>(data, bufoffset), vdata[idx].p, bufsize);
                        bufoffset += bufsize;
                    }
                }
            }
#if H5_VERSION_GE(1, 12, 0)
            herr_t reclaim_err = H5Treclaim(h5t_fields, dsetSpace, H5P_DEFAULT, vdata.data());
#else
            herr_t reclaim_err = H5Dvlen_reclaim(h5t_fields, dsetSpace, H5P_DEFAULT, vdata.data());
#endif

            if(reclaim_err) {
                auto h5t_info = getH5TInfo(h5t_fields);
                h5pp::runtime_error("readTableField: H5Dvlen_reclaim failed when reading variable-length field data\n"
                                    "table [{}] | {}{}{}",
                                    info.tablePath.value(),
                                    h5t_info.string(),
                                    h5t_info.numMembers ? "\n" : "",
                                    h5t_info.string_members());
            }
            return;
        } else if constexpr(std::is_same_v<DataType, std::vector<std::byte>>) {
            auto fieldSizeSum = getBytesPerElem(h5t_fields);
            data.resize(fieldSizeSum * extent.value());
        } else {
            h5pp::util::resizeData(data, {extent.value()});
            // Make sure the data type of the given read buffer matches the size computed above.
            // If there is a mismatch here it can cause horrible bugs/segfaults
            size_t dtypeSize    = util::getBytesPerElem<DataType>();
            auto   fieldSizeSum = getBytesPerElem(h5t_fields);
            if(dtypeSize != fieldSizeSum) {
                auto        h5t_info = getH5TInfo(h5t_fields);
                std::string error_msg;
                for(size_t idx = 0; idx < type::safe_cast<size_t>(h5t_info.numMembers.value()); idx++) {
                    error_msg += h5pp::format("     Field {:>5}/{:<5} {:>32} = {} bytes\n",
                                              idx,
                                              h5t_info.numMembers.value(),
                                              h5t_info.memberNames.value()[idx],
                                              h5t_info.memberSizes.value()[idx]);
                }
                throw h5pp::runtime_error("readTableField: Type size mismatch:\n"
                                          "Table [{}]\n"
                                          "{}"
                                          "Fields require {} bytes/element\n"
                                          "Buffer [{}] has {} bytes/element\n"
                                          "Hint: If the buffer is a container of structs, those structs may been padded by the compiler\n"
                                          "      Consider declaring such structs with __attribute__((packed, aligned(1)))",
                                          info.tablePath.value(),
                                          error_msg,
                                          fieldSizeSum,
                                          type::sfinae::type_name<DataType>(),
                                          dtypeSize);
            }
        }

        // Get the memory address to the data buffer
        auto dataPtr = h5pp::util::getVoidPointer<void *>(data);

        /* Read data */
        herr_t retval = H5Dread(info.h5Dset.value(), h5t_fields, dataSpace, dsetSpace, H5P_DEFAULT, dataPtr);
        if(retval < 0) throw h5pp::runtime_error("Could not read table fields on table [{}]", info.tablePath.value());
    }

    template<typename DataType>
    inline void readTableField(DataType                  &data,
                               const TableInfo           &info,
                               const std::vector<size_t> &fieldIndices, // Field indices for the table on file
                               std::optional<hsize_t>     offset = std::nullopt,
                               std::optional<hsize_t>     extent = std::nullopt,
                               const PropertyLists       &plists = PropertyLists()) {
        static_assert(not std::is_const_v<DataType>);
        hid::h5t tgtTypeId = util::getFieldTypeId(info, fieldIndices);
        readTableField(data, info, tgtTypeId, offset, extent, plists);
    }

    template<typename DataType>
    inline void readTableField(DataType                       &data,
                               const TableInfo                &info,
                               const std::vector<std::string> &fieldNames,
                               std::optional<hsize_t>          offset = std::nullopt,
                               std::optional<hsize_t>          extent = std::nullopt,
                               const PropertyLists            &plists = PropertyLists()) {
        static_assert(not std::is_const_v<DataType>);
        hid::h5t tgtTypeId = util::getFieldTypeId(info, fieldNames);
        readTableField(data, info, tgtTypeId, offset, extent, plists);
    }

    template<typename h5x_src,
             typename h5x_tgt,
             // enable_if so the compiler doesn't think it can use overload with std::string those arguments
             typename = std::enable_if_t<type::sfinae::is_hdf5_loc_id<h5x_src>>,
             typename = std::enable_if_t<type::sfinae::is_hdf5_loc_id<h5x_tgt>>>
    inline void copyLink(const h5x_src       &srcLocId,
                         std::string_view     srcLinkPath,
                         const h5x_tgt       &tgtLocId,
                         std::string_view     tgtLinkPath,
                         const PropertyLists &plists = PropertyLists()) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x_src>,
                      "Template function [h5pp::hdf5::copyLink(const h5x_src & srcLocId, ...)] requires type h5x_src to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        static_assert(type::sfinae::is_hdf5_loc_id<h5x_tgt>,
                      "Template function [h5pp::hdf5::copyLink(..., ..., const h5x_tgt & tgtLocId, ...)] requires type h5x_tgt to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");

        h5pp::logger::log->trace("Copying link [{}] --> [{}]", srcLinkPath, tgtLinkPath);
        // Copy the link srcLinkPath to tgtLinkPath. Note that H5Ocopy does this recursively, so we don't need
        // to iterate links recursively here.
        hid::h5p lcpl   = H5Pcopy(plists.linkCreate);
        hid::h5p ocpypl = H5Pcreate(H5P_OBJECT_COPY);
        if(H5Pset_create_intermediate_group(lcpl, 1) < 0) // Set to create intermediate groups
            throw h5pp::runtime_error("H5Pset_create_intermediate_group failed");
        if(H5Pset_copy_object(ocpypl, H5O_COPY_MERGE_COMMITTED_DTYPE_FLAG) < 0) throw h5pp::runtime_error("H5Pset_copy_object failed");

        auto retval = H5Ocopy(srcLocId, util::safe_str(srcLinkPath).c_str(), tgtLocId, util::safe_str(tgtLinkPath).c_str(), ocpypl, lcpl);
        if(retval < 0) throw h5pp::runtime_error("Could not copy link [{}] --> [{}]", srcLinkPath, tgtLinkPath);
    }

    template<typename h5x_src,
             typename h5x_tgt,
             // enable_if so the compiler doesn't think it can use overload with fs::path those arguments
             typename = std::enable_if_t<type::sfinae::is_hdf5_loc_id<h5x_src>>,
             typename = std::enable_if_t<type::sfinae::is_hdf5_loc_id<h5x_tgt>>>
    inline void moveLink(const h5x_src       &srcLocId,
                         std::string_view     srcLinkPath,
                         const h5x_tgt       &tgtLocId,
                         std::string_view     tgtLinkPath,
                         LocationMode         locationMode = LocationMode::DETECT,
                         const PropertyLists &plists       = PropertyLists()) {
        static_assert(type::sfinae::is_hdf5_loc_id<h5x_src>,
                      "Template function [h5pp::hdf5::moveLink(const h5x_src & srcLocId, ...)] requires type h5x_src to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        static_assert(type::sfinae::is_hdf5_loc_id<h5x_tgt>,
                      "Template function [h5pp::hdf5::moveLink(..., ..., const h5x_tgt & tgtLocId, ...)] requires type h5x_tgt to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");

        h5pp::logger::log->trace("Moving link [{}] --> [{}]", srcLinkPath, tgtLinkPath);
        // Move the link srcLinkPath to tgtLinkPath. Note that H5Lmove only works inside a single file.
        // For different files we should do H5Ocopy followed by H5Ldelete
        bool     sameFile = h5pp::util::onSameFile(srcLocId, tgtLocId, locationMode);
        hid::h5p lcpl     = H5Pcopy(plists.linkCreate);
        if(H5Pset_create_intermediate_group(lcpl, 1) < 0) // Set to create intermediate groups
            throw h5pp::runtime_error("H5Pset_create_intermediate_group failed");

        if(sameFile) {
            auto retval = H5Lmove(srcLocId,
                                  util::safe_str(srcLinkPath).c_str(),
                                  tgtLocId,
                                  util::safe_str(tgtLinkPath).c_str(),
                                  lcpl,
                                  plists.linkAccess);
            if(retval < 0) throw h5pp::runtime_error("Could not copy link [{}] --> [{}]", srcLinkPath, tgtLinkPath);

        } else {
            // Different files
            hid::h5p ocpypl = H5Pcreate(H5P_OBJECT_COPY);
            if(H5Pset_copy_object(ocpypl, H5O_COPY_MERGE_COMMITTED_DTYPE_FLAG) < 0) throw h5pp::runtime_error("H5Pset_copy_object failed");
            auto retval =
                H5Ocopy(srcLocId, util::safe_str(srcLinkPath).c_str(), tgtLocId, util::safe_str(tgtLinkPath).c_str(), ocpypl, lcpl);
            if(retval < 0) throw h5pp::runtime_error("Could not copy link [{}] --> [{}]", srcLinkPath, tgtLinkPath);

            retval = H5Ldelete(srcLocId, util::safe_str(srcLinkPath).c_str(), plists.linkAccess);
            if(retval < 0) throw h5pp::runtime_error("Could not delete link after move [{}]", srcLinkPath);
        }
    }

    inline void copyLink(const h5pp::fs::path &srcFilePath,
                         std::string_view      srcLinkPath,
                         const h5pp::fs::path &tgtFilePath,
                         std::string_view      tgtLinkPath,
                         FileAccess            tgtFileAccess = FileAccess::READWRITE,
                         const PropertyLists  &plists        = PropertyLists()) {
        h5pp::logger::log->trace("Copying link: source link [{}] | source file [{}]  -->  target link [{}] | target file [{}]",
                                 srcLinkPath,
                                 srcFilePath.string(),
                                 tgtLinkPath,
                                 tgtFilePath.string());

        try {
            auto srcPath = fs::absolute(srcFilePath);
            if(not fs::exists(srcPath)) {
                throw h5pp::runtime_error("Could not copy link [{}] from file [{}]: source file does not exist [{}]",
                                          srcLinkPath,
                                          srcFilePath.string(),
                                          srcPath.string());
            }
            auto tgtPath = h5pp::hdf5::createFile(tgtFilePath, tgtFileAccess, plists);

            hid_t hidSrc = H5Fopen(srcPath.string().c_str(), H5F_ACC_RDONLY, plists.fileAccess);
            hid_t hidTgt = H5Fopen(tgtPath.string().c_str(), H5F_ACC_RDWR, plists.fileAccess);
            if(hidSrc < 0) throw h5pp::runtime_error("Failed to open source file [{}] in read-only mode", srcPath.string());

            if(hidTgt < 0) throw h5pp::runtime_error("Failed to open target file [{}] in read-write mode", tgtPath.string());

            hid::h5f srcFile = hidSrc;
            hid::h5f tgtFile = hidTgt;
            h5pp::hdf5::copyLink(srcFile, srcLinkPath, tgtFile, tgtLinkPath, plists);
        } catch(const std::exception &ex) {
            throw h5pp::runtime_error("Could not copy link [{}] from file [{}]: {}", srcLinkPath, srcFilePath.string(), ex.what());
        }
    }

    inline fs::path copyFile(const h5pp::fs::path &srcFilePath,
                             const h5pp::fs::path &tgtFilePath,
                             FileAccess            tgtFileAccess = FileAccess::COLLISION_FAIL,
                             const PropertyLists  &plists        = PropertyLists()) {
        h5pp::logger::log->trace("Copying file [{}] --> [{}]", srcFilePath.string(), tgtFilePath.string());
        auto tgtPath = h5pp::hdf5::createFile(tgtFilePath, tgtFileAccess, plists);
        auto srcPath = fs::absolute(srcFilePath);
        try {
            if(not fs::exists(srcPath)) {
                throw h5pp::runtime_error("Could not copy file [{}] --> [{}]: source file does not exist [{}]",
                                          srcFilePath.string(),
                                          tgtFilePath.string(),
                                          srcPath.string());
            }
            if(tgtPath == srcPath)
                h5pp::logger::log->debug("Skipped copying file: source and target files have the same path [{}]", srcPath.string());

            hid_t hidSrc = H5Fopen(srcPath.string().c_str(), H5F_ACC_RDONLY, plists.fileAccess);
            hid_t hidTgt = H5Fopen(tgtPath.string().c_str(), H5F_ACC_RDWR, plists.fileAccess);
            if(hidSrc < 0) throw h5pp::runtime_error("Failed to open source file [{}] in read-only mode", srcPath.string());

            if(hidTgt < 0) throw h5pp::runtime_error("Failed to open target file [{}] in read-write mode", tgtPath.string());

            hid::h5f srcFile = hidSrc;
            hid::h5f tgtFile = hidTgt;
            hid::h5p lcpl    = H5Pcopy(plists.linkCreate);
            hid::h5p ocpypl  = H5Pcreate(H5P_OBJECT_COPY);
            if(H5Pset_create_intermediate_group(lcpl, 1) < 0) // Set to create intermediate groups
                throw h5pp::runtime_error("H5Pset_create_intermediate_group failed");

            if(H5Pset_copy_object(ocpypl, H5O_COPY_MERGE_COMMITTED_DTYPE_FLAG) < 0) throw h5pp::runtime_error("H5Pset_copy_object failed");

            // Copy all the groups in the file root recursively. Note that H5Ocopy does this recursively, so we don't need
            // to iterate links recursively here. Therefore, maxDepth = 0
            long maxDepth = 0;
            for(const auto &link : getContentsOfLink<H5O_TYPE_UNKNOWN>(srcFile, "/", maxDepth, plists)) {
                if(link == ".") continue;
                h5pp::logger::log->trace("Copying recursively: [{}]", link);
                auto retval = H5Ocopy(srcFile, link.c_str(), tgtFile, link.c_str(), ocpypl, lcpl);
                if(retval < 0) {
                    throw h5pp::runtime_error(
                        "Failed to copy file contents with H5Ocopy(srcFile,{},tgtFile,{},H5P_DEFAULT,link_create_propery_list)",
                        link,
                        link);
                }
            }
            // ... Find out how to copy attributes that are written on the root itself
            return tgtPath;
        } catch(const std::exception &ex) {
            throw h5pp::runtime_error("Could not copy file [{}] --> [{}]: ", srcFilePath.string(), tgtFilePath.string(), ex.what());
        }
    }

    inline void moveLink(const h5pp::fs::path &srcFilePath,
                         std::string_view      srcLinkPath,
                         const h5pp::fs::path &tgtFilePath,
                         std::string_view      tgtLinkPath,
                         FileAccess            tgtFileAccess = FileAccess::READWRITE,
                         const PropertyLists  &plists        = PropertyLists()) {
        h5pp::logger::log->trace("Moving link: source link [{}] | source file [{}]  -->  target link [{}] | target file [{}]",
                                 srcLinkPath,
                                 srcFilePath.string(),
                                 tgtLinkPath,
                                 tgtFilePath.string());

        try {
            auto srcPath = fs::absolute(srcFilePath);
            if(not fs::exists(srcPath)) {
                throw h5pp::runtime_error("Could not move link [{}] from file [{}]:\n\t source file with absolute path [{}] does not exist",
                                          srcLinkPath,
                                          srcFilePath.string(),
                                          srcPath.string());
            }
            auto tgtPath = h5pp::hdf5::createFile(tgtFilePath, tgtFileAccess, plists);

            hid_t hidSrc = H5Fopen(srcPath.string().c_str(), H5F_ACC_RDWR, plists.fileAccess);
            hid_t hidTgt = H5Fopen(tgtPath.string().c_str(), H5F_ACC_RDWR, plists.fileAccess);
            if(hidSrc < 0) throw h5pp::runtime_error("Failed to open source file [{}] in read-only mode", srcPath.string());
            if(hidTgt < 0) throw h5pp::runtime_error("Failed to open target file [{}] in read-write mode", tgtPath.string());

            hid::h5f srcFile = hidSrc;
            hid::h5f tgtFile = hidTgt;

            auto locMode = h5pp::util::getLocationMode(srcFilePath, tgtFilePath);
            h5pp::hdf5::moveLink(srcFile, srcLinkPath, tgtFile, tgtLinkPath, locMode, plists);
        } catch(const std::exception &ex) {
            throw h5pp::runtime_error("Could not move link [{}] from file [{}]: {}", srcLinkPath, srcFilePath.string(), ex.what());
        }
    }

    inline fs::path moveFile(const h5pp::fs::path &src,
                             const h5pp::fs::path &tgt,
                             FileAccess            tgtFileAccess = FileAccess::COLLISION_FAIL,
                             const PropertyLists  &plists        = PropertyLists()) {
        h5pp::logger::log->trace("Moving file by copy+remove: [{}] --> [{}]", src.string(), tgt.string());
        auto tgtPath = h5pp::hdf5::copyFile(src, tgt, tgtFileAccess, plists); // Returns the path to the newly created file
        auto srcPath = fs::absolute(src);
        if(fs::exists(tgtPath)) {
            h5pp::logger::log->trace("Removing file [{}]", srcPath.string());
            try {
                fs::remove(srcPath);
            } catch(const std::exception &err) {
                throw h5pp::runtime_error("Remove failed. File may be locked [{}] | what(): {} ", srcPath.string(), err.what());
            }
            return tgtPath;
        } else {
            throw h5pp::runtime_error("Could not copy file [{}] to target [{}]", srcPath.string(), tgt.string());
        }

        return tgtPath;
    }
}
