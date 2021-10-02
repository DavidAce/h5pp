#pragma once
#include "h5ppDebug.h"
#include "h5ppEigen.h"
#include "h5ppEnums.h"
#include "h5ppFilesystem.h"
#include "h5ppHyperslab.h"
#include "h5ppInfo.h"
#include "h5ppLogger.h"
#include "h5ppPropertyLists.h"
#include "h5ppTypeSfinae.h"
#include "h5ppUtils.h"
#include <cstddef>
#include <hdf5.h>
#include <map>
#include <typeindex>
#include <utility>
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
            if(currentPosition != foundPosition) { output.emplace_back(path.substr(0, foundPosition)); }
            if(foundPosition == std::string_view::npos) break;
            currentPosition = foundPosition + 1;
        }
        return output;
    }

    template<typename h5x, typename = std::enable_if_t<std::is_base_of_v<hid::hid_base<h5x>, h5x>>>
    [[nodiscard]] std::string getName(const h5x &object) {
        // Read about the buffer size inconsistency here
        // http://hdf-forum.184993.n3.nabble.com/H5Iget-name-inconsistency-td193143.html
        std::string buf;
        ssize_t     bufSize = H5Iget_name(object, nullptr, 0); // Size in bytes of the object name (NOT including \0)
        if(bufSize > 0) {
            buf.resize(static_cast<size_t>(bufSize) + 1);                      // We allocate space for the null terminator with +1
            H5Iget_name(object, buf.data(), static_cast<size_t>(bufSize + 1)); // Read name including \0 with +1
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
        if(ndims < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to get chunk dimensions");
        } else if(ndims > 0) {
            std::vector<hsize_t> chunkDims(static_cast<size_t>(ndims));
            H5Pget_chunk(dsetCreatePropertyList, ndims, chunkDims.data());
            return chunkDims;
        } else
            return std::nullopt;
    }

    [[nodiscard]] inline std::optional<std::vector<hsize_t>> getChunkDimensions(const hid::h5d &dataset) {
        hid::h5p dcpl = H5Dget_create_plist(dataset);
        return getChunkDimensions(dcpl);
    }

    [[nodiscard]] inline int getCompressionLevel(const hid::h5p &dsetCreatePropertyList) {
        auto                        nfilter = H5Pget_nfilters(dsetCreatePropertyList);
        H5Z_filter_t                filter  = H5Z_FILTER_NONE;
        std::array<unsigned int, 1> cdval   = {0};
        std::array<size_t, 1>       cdelm   = {0};
        for(int idx = 0; idx < nfilter; idx++) {
            filter = H5Pget_filter(dsetCreatePropertyList, idx, nullptr, cdelm.data(), cdval.data(), 0, nullptr, nullptr);
            if(filter != H5Z_FILTER_DEFLATE) continue;
            H5Pget_filter_by_id(dsetCreatePropertyList, filter, nullptr, cdelm.data(), cdval.data(), 0, nullptr, nullptr);
        }
        return static_cast<int>(cdval[0]);
    }

    [[nodiscard]] inline std::optional<std::vector<hsize_t>> getMaxDimensions(const hid::h5s &space, H5D_layout_t layout) {
        if(layout != H5D_CHUNKED) return std::nullopt;
        if(H5Sget_simple_extent_type(space) != H5S_SIMPLE) return std::nullopt;
        int rank = H5Sget_simple_extent_ndims(space);
        if(rank < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to get dimensions");
        }
        std::vector<hsize_t> dimsMax(static_cast<size_t>(rank));
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
        std::vector<const char *> vdata{static_cast<size_t>(size)}; // Allocate for pointers for "size" number of strings
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
            *vlen += static_cast<hsize_t>(std::min(std::string_view(elem).size(), maxLen) + 1); // Add null-terminator
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
        herr_t                    retval = H5Aread(attr, type, vdata.data());
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            return 0;
        }
        // Sum up the number of bytes
        size_t maxLen = h5pp::constants::maxSizeCompact;
        for(auto elem : vdata) {
            if(elem == nullptr) continue;
            *vlen += static_cast<hsize_t>(std::min(std::string_view(elem).size(), maxLen) + 1); // Add null-terminator
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

    [[nodiscard]] inline size_t getBytesSelected(const hid::h5s &space, const hid::h5t &type) {
        return getBytesPerElem(type) * getSizeSelected(space);
    }

    template<typename DataType>
    void assertWriteBufferIsLargeEnough(const DataType &data, const hid::h5s &space, const hid::h5t &type) {
        if(H5Tget_class(type) == H5T_STRING) {
            if(H5Tis_variable_str(type)) return; // This transfers the string from memory until finding a null terminator
            if constexpr(h5pp::type::sfinae::is_text_v<DataType>) {
                auto hdf5Byte = H5Tget_size(type); // Chars including null-terminator. The memory buffer must fit this size. Also, these
                                                   // many bytes will participate in IO
                auto hdf5Size = getSizeSelected(space);
                auto dataByte = h5pp::util::getCharArraySize(data, false); // Allocated chars including null terminator
                auto dataSize = h5pp::util::getSize(data);
                if(dataByte < hdf5Byte)
                    throw std::runtime_error(
                        h5pp::format("The text buffer given for this write operation is smaller than the selected space in memory.\n"
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
                throw std::runtime_error(
                    h5pp::format("The buffer given for this write operation is smaller than the selected space in memory.\n"
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
    void assertReadSpaceIsLargeEnough(const DataType &data, const hid::h5s &space, const hid::h5t &type) {
        if(H5Tget_class(type) == H5T_STRING) {
            if(H5Tis_variable_str(type)) return; // These are resized on the fly
            if constexpr(h5pp::type::sfinae::is_text_v<DataType>) {
                // The memory buffer must fit hdf5Byte: that's how many bytes will participate in IO
                auto hdf5Byte = H5Tget_size(type); // Chars including null-terminator.
                auto hdf5Size = getSizeSelected(space);
                auto dataByte = h5pp::util::getCharArraySize(data) + 1; // Chars including null terminator
                auto dataSize = h5pp::util::getSize(data);
                if(dataByte < hdf5Byte)
                    throw std::runtime_error(h5pp::format(
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
                throw std::runtime_error(
                    h5pp::format("The buffer allocated for this read operation is smaller than the selected space in memory.\n"
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
                h5pp::logger::log->warn(
                    "Detected packed HDF5 type: packed size {} bytes | native size {} bytes. This is not supported by h5pp yet!",
                    packedTypesize,
                    dataTypeSize);
        }
        return dataTypeSize == dsetTypeSize;
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    void assertBytesPerElemMatch(const hid::h5t &h5Type) {
        size_t dsetTypeSize = 0;
        size_t dataTypeSize = h5pp::util::getBytesPerElem<DataType>();
        if(H5Tget_class(h5Type) == H5T_STRING)
            dsetTypeSize = H5Tget_size(H5T_C_S1);
        else
            dsetTypeSize = h5pp::hdf5::getBytesPerElem(h5Type);
        if(dataTypeSize != dsetTypeSize) {
            // The dsetType may have been generated by H5Tpack, in which case we should check against the native type
            size_t   packedTypesize = dsetTypeSize;
            hid::h5t nativetype     = H5Tget_native_type(h5Type, H5T_DIR_ASCEND);
            dsetTypeSize            = h5pp::hdf5::getBytesPerElem(nativetype);
            if(dataTypeSize != dsetTypeSize) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Type size mismatch: dataset type is [{}] bytes | Type of given data is [{}] bytes",
                                                      dsetTypeSize,
                                                      dataTypeSize));
            }

            else
                h5pp::logger::log->warn(
                    "Detected packed HDF5 type: packed size {} bytes | native size {} bytes. This is not supported by h5pp yet!",
                    packedTypesize,
                    dataTypeSize);
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    void assertReadTypeIsLargeEnough(const hid::h5t &h5Type) {
        size_t dsetTypeSize = h5pp::hdf5::getBytesPerElem(h5Type);
        size_t dataTypeSize = h5pp::util::getBytesPerElem<DataType>();
        if(H5Tget_class(h5Type) == H5T_STRING) dsetTypeSize = H5Tget_size(H5T_C_S1);
        if(dataTypeSize != dsetTypeSize) {
            // The dsetType may have been generated by H5Tpack, in which case we should check against the native type
            size_t   packedTypesize = dsetTypeSize;
            hid::h5t nativetype     = H5Tget_native_type(h5Type, H5T_DIR_ASCEND);
            dsetTypeSize            = h5pp::hdf5::getBytesPerElem(nativetype);
            if(dataTypeSize > dsetTypeSize)
                h5pp::logger::log->debug(
                    "Given data-type is too large: elements of type [{}] are [{}] bytes (each) | target HDF5 type is [{}] bytes",
                    h5pp::type::sfinae::type_name<DataType>(),
                    dataTypeSize,
                    dsetTypeSize);
            else if(dataTypeSize < dsetTypeSize)
                throw std::runtime_error(h5pp::format(
                    "Given data-type is too small: elements of type [{}] are [{}] bytes (each) | target HDF5 type is [{}] bytes",
                    h5pp::type::sfinae::type_name<DataType>(),
                    dataTypeSize,
                    dsetTypeSize));
            else
                h5pp::logger::log->warn(
                    "Detected packed HDF5 type: packed size {} bytes | native size {} bytes. This is not supported by h5pp yet!",
                    packedTypesize,
                    dataTypeSize);
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
                    retval              = H5Tset_size(type, desiredSize); // Desired size should be num chars + null terminator
                    dims                = {};
                    size                = 1;
                    bytes               = desiredSize;
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

    template<typename h5x>
    [[nodiscard]] inline bool checkIfLinkExists(const h5x &loc, std::string_view linkPath, const hid::h5p &linkAccess = H5P_DEFAULT) {
        static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x>,
                      "Template function [h5pp::hdf5::checkIfLinkExists<h5x>(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
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

    template<typename h5x, typename h5x_loc>
    [[nodiscard]] h5x openLink(const h5x_loc      &loc,
                               std::string_view    linkPath,
                               std::optional<bool> linkExists = std::nullopt,
                               const hid::h5p     &linkAccess = H5P_DEFAULT) {
        static_assert(h5pp::type::sfinae::is_h5_link_v<h5x>,
                      "Template function [h5pp::hdf5::openLink<h5x>(...)] requires type h5x to be: "
                      "[h5pp::hid::h5d], [h5pp::hid::h5g] or [h5pp::hid::h5o]");
        static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x_loc>,
                      "Template function [h5pp::hdf5::openLink<h5x>(const h5x_loc & loc, ...)] requires type h5x_loc to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        if constexpr(not h5pp::ndebug){
            if(not linkExists) linkExists = checkIfLinkExists(loc, linkPath, linkAccess);
            if(not linkExists.value())
                throw std::runtime_error(h5pp::format("Cannot open link [{}]: it does not exist [{}]", linkPath));
        }
        if constexpr(std::is_same_v<h5x, hid::h5d>) h5pp::logger::log->trace("Opening dataset [{}]", linkPath);
        if constexpr(std::is_same_v<h5x, hid::h5g>) h5pp::logger::log->trace("Opening group [{}]", linkPath);
        if constexpr(std::is_same_v<h5x, hid::h5o>) h5pp::logger::log->trace("Opening object [{}]", linkPath);

        hid_t link;
        if constexpr(std::is_same_v<h5x, hid::h5d>) link = H5Dopen(loc, util::safe_str(linkPath).c_str(), linkAccess);
        if constexpr(std::is_same_v<h5x, hid::h5g>) link = H5Gopen(loc, util::safe_str(linkPath).c_str(), linkAccess);
        if constexpr(std::is_same_v<h5x, hid::h5o>) link = H5Oopen(loc, util::safe_str(linkPath).c_str(), linkAccess);

        if(link < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            if constexpr(std::is_same_v<h5x, hid::h5d>) throw std::runtime_error(h5pp::format("Failed to open dataset [{}]", linkPath));
            if constexpr(std::is_same_v<h5x, hid::h5g>) throw std::runtime_error(h5pp::format("Failed to open group [{}]", linkPath));
            if constexpr(std::is_same_v<h5x, hid::h5o>) throw std::runtime_error(h5pp::format("Failed to open object [{}]", linkPath));
        }

        return link;

    }

    template<typename h5x>
    [[nodiscard]] inline bool checkIfAttrExists(const h5x &link, std::string_view attrName, const hid::h5p &linkAccess = H5P_DEFAULT) {
        static_assert(h5pp::type::sfinae::is_h5_link_or_hid_v<h5x>,
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
        static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x>,
                      "Template function [h5pp::hdf5::checkIfAttrExists<h5x>(const h5x & loc, ..., ...)] requires type h5x to be: "
                      "[h5pp::hid::h5d], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        if(not linkExists) linkExists = checkIfLinkExists(loc, linkPath, linkAccess);
        // If the link does not exist the attribute doesn't exist either
        if(not linkExists.value()) return false;
        // Otherwise we open the link and check
        auto link = openLink<hid::h5o>(loc, linkPath, linkExists, linkAccess);
        h5pp::logger::log->trace("Checking if attribute [{}] exitst in link [{}] ...", attrName, linkPath);
        bool exists = H5Aexists_by_name(link, std::string(".").c_str(), util::safe_str(attrName).c_str(), linkAccess) > 0;
        h5pp::logger::log->trace("Checking if attribute [{}] exitst in link [{}] ... {}", attrName, linkPath, exists);
        return exists;
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
                char            *mem1  = H5Tget_member_name(type1, static_cast<unsigned int>(idx));
                char            *mem2  = H5Tget_member_name(type2, static_cast<unsigned int>(idx));
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

    [[nodiscard]] inline unsigned int getValidCompressionLevel(std::optional<unsigned int> compressionLevel = std::nullopt) {
        if(checkIfCompressionIsAvailable()) {
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
            buf.resize(bufSize);
            H5Aget_name(attribute, static_cast<size_t>(bufSize) + 1, buf.data()); // buf is guaranteed to have \0 at the end
        } else {
            H5Eprint(H5E_DEFAULT, stderr);
            h5pp::logger::log->debug("Failed to get attribute names");
        }
        return buf.c_str();
    }

    template<typename h5x>
    [[nodiscard]] inline std::vector<std::string> getAttributeNames(const h5x &link) {
        static_assert(h5pp::type::sfinae::is_h5_link_or_hid_v<h5x>,
                      "Template function [h5pp::hdf5::getAttributeNames(const h5x & link, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5d], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        auto                     numAttrs = H5Aget_num_attrs(link);
        std::vector<std::string> attrNames;
        std::string              buf;
        for(auto i = 0; i < numAttrs; i++) {
            hid::h5a attrId = H5Aopen_idx(link, static_cast<unsigned int>(i));
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
            h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x>,
            "Template function [h5pp::hdf5::getAttributeNames(const h5x & link, std::string_view linkPath)] requires type h5x to be: "
            "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        auto link = openLink<hid::h5o>(loc, linkPath, linkExists, linkAccess);
        return getAttributeNames(link);
    }

    template<typename T>
    [[nodiscard]] std::tuple<std::type_index, std::string, size_t> getCppType() {
        return {typeid(T), std::string(h5pp::type::sfinae::type_name<T>()), sizeof(T)};
    }

    [[nodiscard]] inline std::tuple<std::type_index, std::string, size_t> getCppType(const hid::h5t &type) {
        using namespace h5pp::type::compound;
        auto h5class = H5Tget_class(type);
        auto h5size  = H5Tget_size(type);
        /* clang-format off */
        if(h5class == H5T_class_t::H5T_INTEGER){
            if(h5size == 8){
                if(H5Tequal(type, H5T_NATIVE_INT8))          return getCppType<int8_t>();
                if(H5Tequal(type, H5T_NATIVE_UINT8))         return getCppType<uint8_t>();
                if(H5Tequal(type, H5T_NATIVE_INT_FAST8))     return getCppType<int_fast8_t>();
                if(H5Tequal(type, H5T_NATIVE_UINT_FAST8))    return getCppType<uint_fast8_t>();
            }else if(h5size == 16){
                if(H5Tequal(type, H5T_NATIVE_INT16))         return getCppType<int16_t>();
                if(H5Tequal(type, H5T_NATIVE_UINT16))        return getCppType<uint16_t>();
                if(H5Tequal(type, H5T_NATIVE_INT_FAST16))    return getCppType<int_fast16_t>();
                if(H5Tequal(type, H5T_NATIVE_UINT_FAST16))   return getCppType<uint_fast16_t>();
            }else if(h5size == 32){
                if(H5Tequal(type, H5T_NATIVE_INT32))         return getCppType<int32_t>();
                if(H5Tequal(type, H5T_NATIVE_UINT32))        return getCppType<uint32_t>();
                if(H5Tequal(type, H5T_NATIVE_INT_FAST32))    return getCppType<int_fast32_t>();
                if(H5Tequal(type, H5T_NATIVE_UINT_FAST32))   return getCppType<uint_fast32_t>();
            }
            else if(h5size == 64){
                if(H5Tequal(type, H5T_NATIVE_INT64))         return getCppType<int64_t>();
                if(H5Tequal(type, H5T_NATIVE_UINT64))        return getCppType<uint64_t>();
                if(H5Tequal(type, H5T_NATIVE_INT_FAST64))    return getCppType<int_fast64_t>();
                if(H5Tequal(type, H5T_NATIVE_UINT_FAST64))   return getCppType<uint_fast64_t>();
            }else{
                if(H5Tequal(type, H5T_NATIVE_SHORT))         return getCppType<short>();
                if(H5Tequal(type, H5T_NATIVE_INT))           return getCppType<int>();
                if(H5Tequal(type, H5T_NATIVE_LONG))          return getCppType<long>();
                if(H5Tequal(type, H5T_NATIVE_LLONG))         return getCppType<long long>();
                if(H5Tequal(type, H5T_NATIVE_USHORT))        return getCppType<unsigned short>();
                if(H5Tequal(type, H5T_NATIVE_UINT))          return getCppType<unsigned int>();
                if(H5Tequal(type, H5T_NATIVE_ULONG))         return getCppType<unsigned long>();
                if(H5Tequal(type, H5T_NATIVE_ULLONG))        return getCppType<unsigned long long >();
            }
        }else if (h5class == H5T_class_t::H5T_FLOAT){
            if(H5Tequal(type, H5T_NATIVE_DOUBLE))           return getCppType<double>();
            if(H5Tequal(type, H5T_NATIVE_LDOUBLE))          return getCppType<long double>();
            if(H5Tequal(type, H5T_NATIVE_FLOAT))            return getCppType<float>();
        }else if  (h5class == H5T_class_t::H5T_STRING){
            if(H5Tequal(type, H5T_NATIVE_CHAR))             return getCppType<char>();
            if(H5Tequal_recurse(type, H5Tcopy(H5T_C_S1)))   return getCppType<std::string>();
            if(H5Tequal(type, H5T_NATIVE_SCHAR))            return getCppType<signed char>();
            if(H5Tequal(type, H5T_NATIVE_UCHAR))            return getCppType<unsigned char>();
        }else if (h5class == H5T_COMPOUND){
            auto h5nmemb = H5Tget_nmembers(type);
            if(h5size == 8ul*h5nmemb){
                if (H5T_COMPLEX<int8_t>::equal(type))           return getCppType<std::complex<int8_t>>();
                if (H5T_COMPLEX<uint8_t>::equal(type))          return getCppType<std::complex<uint8_t>>();
                if (H5T_COMPLEX<int_fast8_t>::equal(type))      return getCppType<std::complex<int_fast8_t>>();
                if (H5T_COMPLEX<uint_fast8_t>::equal(type))     return getCppType<std::complex<uint_fast8_t>>();

                if (H5T_SCALAR2<int8_t>::equal(type))           return getCppType<h5pp::type::compound::Scalar2<int8_t>>();
                if (H5T_SCALAR2<uint8_t>::equal(type))          return getCppType<h5pp::type::compound::Scalar2<uint8_t>>();
                if (H5T_SCALAR2<int_fast8_t>::equal(type))      return getCppType<h5pp::type::compound::Scalar2<int_fast8_t>>();
                if (H5T_SCALAR2<uint_fast8_t>::equal(type))     return getCppType<h5pp::type::compound::Scalar2<uint_fast8_t>>();

                if (H5T_SCALAR3<int8_t>::equal(type))           return getCppType<h5pp::type::compound::Scalar3<int8_t>>();
                if (H5T_SCALAR3<uint8_t>::equal(type))          return getCppType<h5pp::type::compound::Scalar3<uint8_t>>();
                if (H5T_SCALAR3<int_fast8_t>::equal(type))      return getCppType<h5pp::type::compound::Scalar3<int_fast8_t>>();
                if (H5T_SCALAR3<uint_fast8_t>::equal(type))     return getCppType<h5pp::type::compound::Scalar3<uint_fast8_t>>();

            }
            else if(h5size == 16ul*h5nmemb){
                if (H5T_COMPLEX<int16_t>::equal(type))          return getCppType<std::complex<int16_t>>();
                if (H5T_COMPLEX<uint16_t>::equal(type))         return getCppType<std::complex<uint16_t>>();
                if (H5T_COMPLEX<int_fast16_t>::equal(type))     return getCppType<std::complex<int_fast16_t>>();
                if (H5T_COMPLEX<uint_fast16_t>::equal(type))    return getCppType<std::complex<uint_fast16_t>>();

                if (H5T_SCALAR2<int16_t>::equal(type))          return getCppType<h5pp::type::compound::Scalar2<int16_t>>();
                if (H5T_SCALAR2<uint16_t>::equal(type))         return getCppType<h5pp::type::compound::Scalar2<uint16_t>>();
                if (H5T_SCALAR2<int_fast16_t>::equal(type))     return getCppType<h5pp::type::compound::Scalar2<int_fast16_t>>();
                if (H5T_SCALAR2<uint_fast16_t>::equal(type))    return getCppType<h5pp::type::compound::Scalar2<uint_fast16_t>>();

                if (H5T_SCALAR3<int16_t>::equal(type))          return getCppType<h5pp::type::compound::Scalar3<int16_t>>();
                if (H5T_SCALAR3<uint16_t>::equal(type))         return getCppType<h5pp::type::compound::Scalar3<uint16_t>>();
                if (H5T_SCALAR3<int_fast16_t>::equal(type))     return getCppType<h5pp::type::compound::Scalar3<int_fast16_t>>();
                if (H5T_SCALAR3<uint_fast16_t>::equal(type))    return getCppType<h5pp::type::compound::Scalar3<uint_fast16_t>>();

            }
            else if(h5size == 32ul*h5nmemb){
                if (H5T_COMPLEX<int32_t>::equal(type))          return getCppType<std::complex<int32_t>>();
                if (H5T_COMPLEX<uint32_t>::equal(type))         return getCppType<std::complex<uint32_t>>();
                if (H5T_COMPLEX<int_fast32_t>::equal(type))     return getCppType<std::complex<int_fast32_t>>();
                if (H5T_COMPLEX<uint_fast32_t>::equal(type))    return getCppType<std::complex<uint_fast32_t>>();

                if (H5T_SCALAR2<int32_t>::equal(type))          return getCppType<h5pp::type::compound::Scalar2<int32_t>>();
                if (H5T_SCALAR2<uint32_t>::equal(type))         return getCppType<h5pp::type::compound::Scalar2<uint32_t>>();
                if (H5T_SCALAR2<int_fast32_t>::equal(type))     return getCppType<h5pp::type::compound::Scalar2<int_fast32_t>>();
                if (H5T_SCALAR2<uint_fast32_t>::equal(type))    return getCppType<h5pp::type::compound::Scalar2<uint_fast32_t>>();

                if (H5T_SCALAR3<int32_t>::equal(type))          return getCppType<h5pp::type::compound::Scalar3<int32_t>>();
                if (H5T_SCALAR3<uint32_t>::equal(type))         return getCppType<h5pp::type::compound::Scalar3<uint32_t>>();
                if (H5T_SCALAR3<int_fast32_t>::equal(type))     return getCppType<h5pp::type::compound::Scalar3<int_fast32_t>>();
                if (H5T_SCALAR3<uint_fast32_t>::equal(type))    return getCppType<h5pp::type::compound::Scalar3<uint_fast32_t>>();

            }
            else if(h5size == 64ul*h5nmemb){
                if (H5T_COMPLEX<int64_t>::equal(type))          return getCppType<std::complex<int64_t>>();
                if (H5T_COMPLEX<uint64_t>::equal(type))         return getCppType<std::complex<uint64_t>>();
                if (H5T_COMPLEX<int_fast64_t>::equal(type))     return getCppType<std::complex<int_fast64_t>>();
                if (H5T_COMPLEX<uint_fast64_t>::equal(type))    return getCppType<std::complex<uint_fast64_t>>();
                if (H5T_SCALAR2<int64_t>::equal(type))          return getCppType<h5pp::type::compound::Scalar2<int64_t>>();
                if (H5T_SCALAR2<uint64_t>::equal(type))         return getCppType<h5pp::type::compound::Scalar2<uint64_t>>();
                if (H5T_SCALAR2<int_fast64_t>::equal(type))     return getCppType<h5pp::type::compound::Scalar2<int_fast64_t>>();
                if (H5T_SCALAR2<uint_fast64_t>::equal(type))    return getCppType<h5pp::type::compound::Scalar2<uint_fast64_t>>();
                if (H5T_SCALAR3<int64_t>::equal(type))          return getCppType<h5pp::type::compound::Scalar3<int64_t>>();
                if (H5T_SCALAR3<uint64_t>::equal(type))         return getCppType<h5pp::type::compound::Scalar3<uint64_t>>();
                if (H5T_SCALAR3<int_fast64_t>::equal(type))     return getCppType<h5pp::type::compound::Scalar3<int_fast64_t>>();
                if (H5T_SCALAR3<uint_fast64_t>::equal(type))    return getCppType<h5pp::type::compound::Scalar3<uint_fast64_t>>();

            }else{
                if (H5T_COMPLEX<double>::equal(type))           return getCppType<std::complex<double>>();
                if (H5T_COMPLEX<long double>::equal(type))      return getCppType<std::complex<long double>>();
                if (H5T_COMPLEX<float>::equal(type))            return getCppType<std::complex<float>>();
                if (H5T_SCALAR2<double>::equal(type))           return getCppType<h5pp::type::compound::Scalar2<double>>();
                if (H5T_SCALAR2<long double>::equal(type))      return getCppType<h5pp::type::compound::Scalar2<long double>>();
                if (H5T_SCALAR2<float>::equal(type))            return getCppType<h5pp::type::compound::Scalar2<float>>();
                if (H5T_SCALAR3<double>::equal(type))           return getCppType<h5pp::type::compound::Scalar3<double>>();
                if (H5T_SCALAR3<long double>::equal(type))      return getCppType<h5pp::type::compound::Scalar3<long double>>();
                if (H5T_SCALAR3<float>::equal(type))            return getCppType<h5pp::type::compound::Scalar3<float>>();
            }
        }
        if(H5Tequal(type, H5T_NATIVE_HBOOL))            return getCppType<hbool_t>();
        if(H5Tcommitted(type) > 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            h5pp::logger::log->debug("No C++ type match for HDF5 type [{}]", getName(type));
        } else {
            h5pp::logger::log->debug("No C++ type match for non-committed HDF5 type");
        }
        std::string name;
        switch(h5class){
            case H5T_class_t::H5T_INTEGER:      name = "H5T_INTEGER"; break;
            case H5T_class_t::H5T_FLOAT:        name = "H5T_FLOAT"; break;
            case H5T_class_t::H5T_TIME:         name = "H5T_TIME"; break;
            case H5T_class_t::H5T_STRING:       name = "H5T_STRING"; break;
            case H5T_class_t::H5T_BITFIELD:     name = "H5T_BITFIELD"; break;
            case H5T_class_t::H5T_OPAQUE:       name = "H5T_OPAQUE"; break;
            case H5T_class_t::H5T_COMPOUND:     name = "H5T_COMPOUND"; break;
            case H5T_class_t::H5T_REFERENCE:    name = "H5T_REFERENCE"; break;
            case H5T_class_t::H5T_ENUM:         name = "H5T_ENUM"; break;
            case H5T_class_t::H5T_VLEN:         name = "H5T_VLEN"; break;
            case H5T_class_t::H5T_ARRAY:        name = "H5T_ARRAY"; break;
            default: name = "UNKNOWN TYPE";
        }
        /* clang-format on */

        return {typeid(nullptr), name, getBytesPerElem(type)};
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

        std::tie(typeInfo.cppTypeIndex, typeInfo.cppTypeName, typeInfo.cppTypeBytes) = getCppType(typeInfo.h5Type.value());
        return typeInfo;
    }

    [[nodiscard]] inline TypeInfo getTypeInfo(const hid::h5d &dataset) {
        auto dsetPath = h5pp::hdf5::getName(dataset);
        h5pp::logger::log->trace("Collecting type info about dataset [{}]", dsetPath);
        return getTypeInfo(dsetPath, std::nullopt, H5Dget_space(dataset), H5Dget_type(dataset));
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc_or_hid_t<h5x>>
    // enable_if so the compiler doesn't think it can use overload with std::string on first argument
    [[nodiscard]] inline TypeInfo getTypeInfo(const h5x          &loc,
                                              std::string_view    dsetName,
                                              std::optional<bool> dsetExists = std::nullopt,
                                              const hid::h5p     &dsetAccess = H5P_DEFAULT) {
        auto dataset = openLink<hid::h5d>(loc, dsetName, dsetExists, dsetAccess);
        return getTypeInfo(dataset);
    }

    [[nodiscard]] inline TypeInfo getTypeInfo(const hid::h5a &attribute) {
        auto attrName = getAttributeName(attribute);
        auto linkPath = getName(attribute); // Returns the name of the link which has the attribute
        h5pp::logger::log->trace("Collecting type info about attribute [{}] in link [{}]", attrName, linkPath);
        return getTypeInfo(linkPath, attrName, H5Aget_space(attribute), H5Aget_type(attribute));
    }

    template<typename h5x>
    [[nodiscard]] inline TypeInfo getTypeInfo(const h5x          &loc,
                                              std::string_view    linkPath,
                                              std::string_view    attrName,
                                              std::optional<bool> linkExists = std::nullopt,
                                              std::optional<bool> attrExists = std::nullopt,
                                              const hid::h5p     &linkAccess = H5P_DEFAULT) {
        if(not linkExists) linkExists = checkIfLinkExists(loc, linkPath, linkAccess);
        if(not linkExists.value())
            throw std::runtime_error(
                h5pp::format("Attribute [{}] does not exist because its link does not exist: [{}]", attrName, linkPath));

        auto link = openLink<hid::h5o>(loc, linkPath, linkExists, linkAccess);
        if(not attrExists) attrExists = checkIfAttrExists(link, attrName, linkAccess);
        if(attrExists.value()) {
            hid::h5a attribute = H5Aopen_name(link, util::safe_str(attrName).c_str());
            return getTypeInfo(attribute);
        } else {
            throw std::runtime_error(h5pp::format("Attribute [{}] does not exist in link [{}]", attrName, linkPath));
        }
    }

    template<typename h5x>
    [[nodiscard]] std::vector<TypeInfo> getTypeInfo_allAttributes(const h5x &link) {
        static_assert(h5pp::type::sfinae::is_h5_link_or_hid_v<h5x>,
                      "Template function [h5pp::hdf5::getTypeInfo_allAttributes(const h5x & link)] requires type h5x to be: "
                      "[h5pp::hid::h5d], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
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

    template<typename h5x>
    [[nodiscard]] inline std::vector<TypeInfo> getTypeInfo_allAttributes(const h5x          &loc,
                                                                         std::string_view    linkPath,
                                                                         std::optional<bool> linkExists = std::nullopt,
                                                                         const hid::h5p     &linkAccess = H5P_DEFAULT) {
        static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x>,
                      "Template function [h5pp::hdf5::getTypeInfo_allAttributes(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        auto link = openLink<hid::h5o>(loc, linkPath, linkExists, linkAccess);
        return getTypeInfo_allAttributes(link);
    }

    template<typename h5x>
    inline void createGroup(const h5x           &loc,
                            std::string_view     groupName,
                            std::optional<bool>  linkExists = std::nullopt,
                            const PropertyLists &plists     = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x>,
                      "Template function [h5pp::hdf5::createGroup(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        // Check if group exists already
        if(not linkExists) linkExists = checkIfLinkExists(loc, groupName, plists.linkAccess);
        if(not linkExists.value()) {
            h5pp::logger::log->trace("Creating group link [{}]", groupName);
            hid_t gid = H5Gcreate(loc, util::safe_str(groupName).c_str(), plists.linkCreate, plists.groupCreate, plists.groupAccess);
            if(gid < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to create group link [{}]", groupName));
            }
            H5Gclose(gid);
        } else
            h5pp::logger::log->trace("Group exists already: [{}]", groupName);
    }

    template<typename h5x>
    inline void createSoftLink(std::string_view     targetLinkPath,
                               const h5x           &loc,
                               std::string_view     softLinkPath,
                               const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x>,
                      "Template function [h5pp::hdf5::createSoftLink(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        if constexpr(not h5pp::ndebug) {
            if(not checkIfLinkExists(loc, targetLinkPath, plists.linkAccess))
                throw std::runtime_error(h5pp::format("Tried to create soft link to a path that does not exist [{}]", targetLinkPath));
        }

        h5pp::logger::log->trace("Creating soft link [{}] --> [{}]", targetLinkPath, softLinkPath);
        herr_t retval = H5Lcreate_soft(util::safe_str(targetLinkPath).c_str(),
                                       loc,
                                       util::safe_str(softLinkPath).c_str(),
                                       plists.linkCreate,
                                       plists.linkAccess);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to create soft link [{}]  ", targetLinkPath));
        }
    }

    template<typename h5x>
    inline void createHardLink(
        const h5x           &targetLinkLoc,
        std::string_view     targetLinkPath,
                               const h5x           &hardLinkLoc,
                               std::string_view     hardLinkPath,
                               const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x>,
                      "Template function [h5pp::hdf5::createHardLink(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        if constexpr(not h5pp::ndebug) {
            if(not checkIfLinkExists(targetLinkLoc, targetLinkPath, plists.linkAccess))
                throw std::runtime_error(h5pp::format("Tried to create a hard link to a path that does not exist [{}]", targetLinkPath));
        }
        h5pp::logger::log->trace("Creating hard link [{}] --> [{}]", targetLinkPath, hardLinkPath);
        herr_t retval = H5Lcreate_hard(targetLinkLoc,
                                       util::safe_str(targetLinkPath).c_str(),
                                       hardLinkLoc,
                                       util::safe_str(hardLinkPath).c_str(),
                                       plists.linkCreate,
                                       plists.linkAccess);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to create hard link [{}] -> [{}] ", targetLinkPath, hardLinkPath));
        }
    }

    template<typename h5x>
    void createExternalLink(std::string_view     targetFilePath,
                            std::string_view     targetLinkPath,
                            const h5x           &loc,
                            std::string_view     softLinkPath,
                            const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x>,
                      "Template function [h5pp::hdf5::createExternalLink(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        h5pp::logger::log->trace("Creating external link [{}] from file [{}] : [{}]", softLinkPath, targetFilePath, targetLinkPath);

        herr_t retval = H5Lcreate_external(util::safe_str(targetFilePath).c_str(),
                                           util::safe_str(targetLinkPath).c_str(),
                                           loc,
                                           util::safe_str(softLinkPath).c_str(),
                                           plists.linkCreate,
                                           plists.linkAccess);

        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to create external link [{}] --> [{}]", targetLinkPath, softLinkPath));
        }
    }

    inline void setProperty_layout(DsetInfo &dsetInfo) {
        if(not dsetInfo.h5PlistDsetCreate)
            throw std::logic_error("Could not configure the H5D layout: the dataset creation property list has not been initialized");
        if(not dsetInfo.h5Layout)
            throw std::logic_error("Could not configure the H5D layout: the H5D layout parameter has not been initialized");
        switch(dsetInfo.h5Layout.value()) {
            case H5D_CHUNKED: h5pp::logger::log->trace("Setting layout H5D_CHUNKED"); break;
            case H5D_COMPACT: h5pp::logger::log->trace("Setting layout H5D_COMPACT"); break;
            case H5D_CONTIGUOUS: h5pp::logger::log->trace("Setting layout H5D_CONTIGUOUS"); break;
            default:
                throw std::runtime_error(
                    "Given invalid layout when creating dataset property list. Choose one of H5D_COMPACT,H5D_CONTIGUOUS,H5D_CHUNKED");
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
            dsetInfo.dsetChunk    = std::nullopt;
            dsetInfo.dsetDimsMax  = std::nullopt;
            dsetInfo.h5Layout     = H5D_CONTIGUOUS; // In case it's a big text
            dsetInfo.resizePolicy = h5pp::ResizePolicy::OFF;
            setProperty_layout(dsetInfo);
            return;
        }

        if(not dsetInfo.h5PlistDsetCreate)
            throw std::logic_error("Could not configure chunk dimensions: the dataset creation property list has not been initialized");
        if(not dsetInfo.dsetRank)
            throw std::logic_error("Could not configure chunk dimensions: the dataset rank (n dims) has not been initialized");
        if(not dsetInfo.dsetDims)
            throw std::logic_error("Could not configure chunk dimensions: the dataset dimensions have not been initialized");
        if(dsetInfo.dsetRank.value() != static_cast<int>(dsetInfo.dsetDims->size()))
            throw std::logic_error(h5pp::format("Could not set chunk dimensions properties: Rank mismatch: dataset dimensions {} has "
                                                "different number of elements than reported rank {}",
                                                dsetInfo.dsetDims.value(),
                                                dsetInfo.dsetRank.value()));
        if(dsetInfo.dsetDims->size() != dsetInfo.dsetChunk->size())
            throw std::logic_error(h5pp::format("Could not configure chunk dimensions: Rank mismatch: dataset dimensions {} and chunk "
                                                "dimensions {} do not have the same number of elements",
                                                dsetInfo.dsetDims->size(),
                                                dsetInfo.dsetChunk->size()));

        h5pp::logger::log->trace("Setting chunk dimensions {}", dsetInfo.dsetChunk.value());
        herr_t err =
            H5Pset_chunk(dsetInfo.h5PlistDsetCreate.value(), static_cast<int>(dsetInfo.dsetChunk->size()), dsetInfo.dsetChunk->data());
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Could not set chunk dimensions");
        }
    }

    inline void setProperty_compression(DsetInfo &dsetInfo) {
        if(not dsetInfo.compression) return;
        if(not checkIfCompressionIsAvailable()) return;
        if(not dsetInfo.h5PlistDsetCreate)
            throw std::runtime_error("Could not configure compression: field h5_plist_dset_create has not been initialized");
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

    inline void
        selectHyperslab(hid::h5s &space, const Hyperslab &hyperSlab, std::optional<H5S_seloper_t> select_op_override = std::nullopt) {
        if(hyperSlab.empty()) return;
        int rank = H5Sget_simple_extent_ndims(space);
        if(rank < 0) throw std::runtime_error("Failed to read space rank");
        std::vector<hsize_t> dims(static_cast<size_t>(rank));
        H5Sget_simple_extent_dims(space, dims.data(), nullptr);
        // If one of slabOffset or slabExtent is given, then the other must also be given
        if(hyperSlab.offset and not hyperSlab.extent)
            throw std::logic_error("Could not setup hyperslab metadata: Given hyperslab offset but not extent");
        if(not hyperSlab.offset and hyperSlab.extent)
            throw std::logic_error("Could not setup hyperslab metadata: Given hyperslab extent but not offset");

        // If given, ranks of slabOffset and slabExtent must be identical to each other and to the rank of the existing dataset
        if(hyperSlab.offset and hyperSlab.extent and (hyperSlab.offset.value().size() != hyperSlab.extent.value().size()))
            throw std::logic_error(
                h5pp::format("Could not setup hyperslab metadata: Size mismatch in given hyperslab arrays: offset {} | extent {}",
                             hyperSlab.offset.value(),
                             hyperSlab.extent.value()));

        if(hyperSlab.offset and hyperSlab.offset.value().size() != static_cast<size_t>(rank))
            throw std::logic_error(
                h5pp::format("Could not setup hyperslab metadata: Hyperslab arrays have different rank compared to the given space: "
                             "offset {} | extent {} | space dims {}",
                             hyperSlab.offset.value(),
                             hyperSlab.extent.value(),
                             dims));

        // If given, slabStride must have the same rank as the dataset
        if(hyperSlab.stride and hyperSlab.stride.value().size() != static_cast<size_t>(rank))
            throw std::logic_error(
                h5pp::format("Could not setup hyperslab metadata: Hyperslab stride has a different rank compared to the dataset: "
                             "stride {} | dataset dims {}",
                             hyperSlab.stride.value(),
                             dims));
        // If given, slabBlock must have the same rank as the dataset
        if(hyperSlab.blocks and hyperSlab.blocks.value().size() != static_cast<size_t>(rank))
            throw std::logic_error(
                h5pp::format("Could not setup hyperslab metadata: Hyperslab blocks has a different rank compared to the dataset: "
                             "blocks {} | dataset dims {}",
                             hyperSlab.blocks.value(),
                             dims));

        if(not select_op_override) select_op_override = hyperSlab.select_oper;
        if(H5Sget_select_type(space) != H5S_SEL_HYPERSLABS and select_op_override != H5S_SELECT_SET)
            select_op_override = H5S_SELECT_SET; // First hyperslab selection must be H5S_SELECT_SET

        herr_t retval = 0;
        /* clang-format off */
        if(hyperSlab.offset and hyperSlab.extent and hyperSlab.stride and hyperSlab.blocks)
            retval = H5Sselect_hyperslab(space, select_op_override.value(), hyperSlab.offset.value().data(), hyperSlab.stride.value().data(), hyperSlab.extent.value().data(), hyperSlab.blocks.value().data());
        else if (hyperSlab.offset and hyperSlab.extent and hyperSlab.stride)
            retval = H5Sselect_hyperslab(space, select_op_override.value(), hyperSlab.offset.value().data(), hyperSlab.stride.value().data(), hyperSlab.extent.value().data(), nullptr);
        else if (hyperSlab.offset and hyperSlab.extent and hyperSlab.blocks)
            retval = H5Sselect_hyperslab(space, select_op_override.value(), hyperSlab.offset.value().data(), nullptr, hyperSlab.extent.value().data(), hyperSlab.blocks.value().data());
        else if (hyperSlab.offset and hyperSlab.extent)
            retval = H5Sselect_hyperslab(space, select_op_override.value(), hyperSlab.offset.value().data(), nullptr, hyperSlab.extent.value().data(), nullptr);
        /* clang-format on */
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to select hyperslab"));
        }
#if H5_VERSION_GE(1, 10, 0)
        htri_t is_regular = H5Sis_regular_hyperslab(space);
        if(is_regular < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to check if Hyperslab selection is regular (non-rectangular)"));
        } else if(is_regular == 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Hyperslab selection is irregular (non-rectangular).\n"
                                                  "This is not yet supported by h5pp"));
        }
#endif
        htri_t valid = H5Sselect_valid(space);
        if(valid < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            Hyperslab slab(space);
            throw std::runtime_error(
                h5pp::format("Hyperslab selection is invalid. {} | space dims {}", getDimensions(space), slab.string()));
        } else if(valid == 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            Hyperslab slab(space);
            throw std::runtime_error(h5pp::format("Hyperslab selection is not contained in the given space. {} | space dims {}",
                                                  getDimensions(space),
                                                  slab.string()));
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

        } else
            for(const auto &slab : hyperSlabs) selectHyperslab(space, slab, H5S_seloper_t::H5S_SELECT_OR);
    }

    inline void setSpaceExtent(const hid::h5s                     &h5Space,
                               const std::vector<hsize_t>         &dims,
                               std::optional<std::vector<hsize_t>> dimsMax = std::nullopt) {
        if(H5Sget_simple_extent_type(h5Space) == H5S_SCALAR) return;
        if(dims.empty()) return;
        herr_t err;
        if(dimsMax) {
            // Here dimsMax was given by the user and we have to do some sanity checks
            // Check that the ranks match
            if(dims.size() != dimsMax->size())
                throw std::runtime_error(h5pp::format("Number of dimensions (rank) mismatch: dims {} | max dims {}\n"
                                                      "\t Hint: Dimension lists must have the same number of elements",
                                                      dims,
                                                      dimsMax.value()));

            std::vector<long> dimsMaxPretty;
            for(auto &dim : dimsMax.value()) {
                if(dim == H5S_UNLIMITED)
                    dimsMaxPretty.emplace_back(-1);
                else
                    dimsMaxPretty.emplace_back(static_cast<long>(dim));
            }
            h5pp::logger::log->trace("Setting dataspace extents: dims {} | max dims {}", dims, dimsMaxPretty);
            err = H5Sset_extent_simple(h5Space, static_cast<int>(dims.size()), dims.data(), dimsMax->data());
            if(err < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to set extents on space: dims {} | max dims {}", dims, dimsMax.value()));
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
        } catch(const std::exception &err) {
            throw std::runtime_error(h5pp::format("Failed to set extent on dataset {} \n Reason {}", dsetInfo.string(), err.what()));
        }
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

    inline void extendDataset(const hid::h5d &dataset, const std::vector<hsize_t> &dims) {
        if(H5Dset_extent(dataset, dims.data()) < 0) throw std::runtime_error("Failed to set extent on dataset");
    }

    template<typename h5x>
    inline void extendDataset(const h5x          &loc,
                              std::string_view    datasetRelativeName,
                              const int           dim,
                              const hsize_t       extent,
                              std::optional<bool> linkExists = std::nullopt,
                              const hid::h5p     &dsetAccess = H5P_DEFAULT) {
        auto dataset = openLink<hid::h5d>(loc, datasetRelativeName, linkExists, dsetAccess);
        extendDataset(dataset, dim, extent);
    }

    template<typename h5x, typename DataType>
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
                throw std::runtime_error(h5pp::format(
                    "Could not append to dataset [{}] along axis {}: Dataset rank ({}) must be strictly larger than the given axis ({})",
                    info.dsetPath.value(),
                    axis,
                    info.dsetRank.value(),
                    axis));
            if(info.dsetRank.value() < appRank)
                throw std::runtime_error(h5pp::format("Cannot append to dataset [{}] along axis {}: Dataset rank {} < appended rank {}",
                                                      info.dsetPath.value(),
                                                      axis,
                                                      info.dsetRank.value(),
                                                      appRank));

            // If we have a dataset with dimensions ijkl and we want to append along j, say, then the remaining
            // ikl should be at least as large as the corresponding dimensions on the given data.
            for(size_t idx = 0; idx < appDimensions.size(); idx++)
                if(idx != axis and appDimensions[idx] > info.dsetDims.value()[idx])
                    throw std::runtime_error(
                        h5pp::format("Could not append to dataset [{}] along axis {}: Dimension {} size mismatch: data {} | dset {}",
                                     info.dsetPath.value(),
                                     axis,
                                     idx,
                                     appDimensions,
                                     info.dsetDims.value()));

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
            std::string oldInfoStr = info.string(h5pp::logger::logIf(1));
            herr_t      err        = H5Dset_extent(info.h5Dset.value(), newDsetDims.data());
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
            h5pp::logger::log->debug("Extended dataset \n \t old: {} \n \t new: {}", oldInfoStr, info.string(h5pp::logger::logIf(1)));
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
            if(outofbounds)
                h5pp::logger::log->warn("A hyperslab selection was made on the dataset [{}{}]. "
                                        "However, resize policy [FIT] will resize this dataset to dimensions {}. "
                                        "This is likely an error.",
                                        info.dsetPath.value(),
                                        info.dsetSlab->string(),
                                        newDimensions);
        }

        if(info.h5Layout and info.h5Layout.value() != H5D_CHUNKED) switch(info.h5Layout.value()) {
                case H5D_COMPACT: throw std::runtime_error("Datasets with H5D_COMPACT layout cannot be resized");
                case H5D_CONTIGUOUS: throw std::runtime_error("Datasets with H5D_CONTIGUOUS layout cannot be resized");
                default: break;
            }
        if(not info.dsetPath) throw std::runtime_error("Could not resize dataset: Path undefined");
        if(not info.h5Space)
            throw std::runtime_error(h5pp::format("Could not resize dataset [{}]: info.h5Space undefined", info.dsetPath.value()));
        if(not info.h5Type)
            throw std::runtime_error(h5pp::format("Could not resize dataset [{}]: info.h5Type undefined", info.dsetPath.value()));
        if(H5Sget_simple_extent_type(info.h5Space.value()) == H5S_SCALAR) return; // These are not supposed to be resized. Typically strings
        if(H5Tis_variable_str(info.h5Type.value()) > 0) return;                   // These are resized on the fly
        info.assertResizeReady();

        // Return if there is no change compared to the current dimensions
        if(info.dsetDims.value() == newDimensions) return;
        // Compare ranks
        if(info.dsetDims->size() != newDimensions.size())
            throw std::runtime_error(
                h5pp::format("Could not resize dataset [{}]: "
                             "Rank mismatch: "
                             "The given dimensions {} must have the same number of elements as the target dimensions {}",
                             info.dsetPath.value(),
                             info.dsetDims.value(),
                             newDimensions));
        if(policy == h5pp::ResizePolicy::GROW) {
            bool allDimsAreSmaller = true;
            for(size_t idx = 0; idx < newDimensions.size(); idx++)
                if(newDimensions[idx] > info.dsetDims.value()[idx]) allDimsAreSmaller = false;
            if(allDimsAreSmaller) return;
        }
        std::string oldInfoStr = info.string(h5pp::logger::logIf(1));
        // Chunked datasets can shrink and grow in any direction
        // Non-chunked datasets can't be resized at all

        for(size_t idx = 0; idx < newDimensions.size(); idx++) {
            if(newDimensions[idx] > info.dsetDimsMax.value()[idx])
                throw std::runtime_error(h5pp::format(
                    "Could not resize dataset [{}]: "
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
            throw std::runtime_error(h5pp::format("Failed to resize dataset [{}] from dimensions {} to {}",
                                                  info.dsetPath.value(),
                                                  info.dsetDims.value(),
                                                  newDimensions));
        }
        // By default, all the space (old and new) is selected
        info.dsetDims = newDimensions;
        info.h5Space  = H5Dget_space(info.h5Dset->value()); // Needs to be refreshed after H5Dset_extent
        info.dsetByte = h5pp::hdf5::getBytesTotal(info.h5Dset.value(), info.h5Space, info.h5Type);
        info.dsetSize = h5pp::hdf5::getSize(info.h5Space.value());
        h5pp::logger::log->debug("Resized dataset \n \t old: {} \n \t new: {}", oldInfoStr, info.string(h5pp::logger::logIf(1)));
    }

    inline void resizeDataset(DsetInfo &dsetInfo, const DataInfo &dataInfo) {
        // We use this function when writing to a dataset on file.
        // Then we RESIZE the dataset to FIT given data.
        // If there is a hyperslab selection on given data, we only need to take that into account.
        // The new dataset dimensions should be dataInfo.dataDims, unless dataInfo.dataSlab.extent exists, which has priority.
        // Note that the final dataset size is then determined by dsetInfo.resizePolicy
        dataInfo.assertWriteReady();
        if(dataInfo.dataSlab and dataInfo.dataSlab->extent)
            resizeDataset(dsetInfo, dataInfo.dataSlab->extent.value());
        else
            resizeDataset(dsetInfo, dataInfo.dataDims.value());
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
            if constexpr(h5pp::type::sfinae::is_text_v<DataType>)
                // Minus one: String resize allocates the null-terminator automatically, and bytes is the number of characters including
                // null-terminator
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
                throw std::runtime_error(h5pp::format("Could not resize given container for text data: Unrecognized type for text [{}]",
                                                      h5pp::type::sfinae::type_name<DataType>()));
            }
        } else if(H5Sget_simple_extent_type(space) == H5S_SCALAR)
            h5pp::util::resizeData(data, {static_cast<hsize_t>(1)});
        else {
            int                  rank = H5Sget_simple_extent_ndims(space);
            std::vector<hsize_t> extent(static_cast<size_t>(rank), 0); // This will have the bounding box containing the current selection
            H5S_sel_type         select_type = H5Sget_select_type(space);
            if(select_type == H5S_sel_type::H5S_SEL_HYPERSLABS) {
                std::vector<hsize_t> start(static_cast<size_t>(rank), 0);
                std::vector<hsize_t> end(static_cast<size_t>(rank), 0);
                H5Sget_select_bounds(space, start.data(), end.data());
                for(size_t idx = 0; idx < extent.size(); idx++) extent[idx] = std::max<hsize_t>(0, 1 + end[idx] - start[idx]);
            } else {
                H5Sget_simple_extent_dims(space, extent.data(), nullptr);
            }
            h5pp::util::resizeData(data, extent);
            if(bytes != h5pp::util::getBytesTotal(data))
                h5pp::logger::log->debug("Size mismatch after resize: data [{}] bytes | dset [{}] bytes ",
                                         h5pp::util::getBytesTotal(data),
                                         bytes);
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    inline void resizeData(DataType &data, DataInfo &dataInfo, const DsetInfo &info) {
        if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>) return; // h5pp never uses malloc
        if(not info.h5Space)
            throw std::runtime_error(h5pp::format("Could not resize given data container: DsetInfo field [h5Space] is not defined"));
        if(not info.h5Type)
            throw std::runtime_error(h5pp::format("Could not resize given data container: DsetInfo field [h5Type] is not defined"));
        if(not info.dsetByte)
            throw std::runtime_error(h5pp::format("Could not resize given data container: DsetInfo field [dsetByte] is not defined"));
        auto oldDims = h5pp::util::getDimensions(data);                                     // Store the old dimensions
        resizeData(data, info.h5Space.value(), info.h5Type.value(), info.dsetByte.value()); // Resize the container
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

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    inline void resizeData(DataType &data, DataInfo &dataInfo, const AttrInfo &attrInfo) {
        if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>) return; // h5pp never uses malloc
        if(not attrInfo.h5Space)
            throw std::runtime_error(h5pp::format("Could not resize given data container: AttrInfo field [h5Space] is not defined"));
        if(not attrInfo.h5Type)
            throw std::runtime_error(h5pp::format("Could not resize given data container: AttrInfo field [h5Type] is not defined"));
        if(not attrInfo.attrByte)
            throw std::runtime_error(h5pp::format("Could not resize given data container: AttrInfo field [attrByte] is not defined"));

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
        std::string msg;
        if(not enable) return msg;
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
                auto dataSelectedSize = getSizeSelected(dataSpace);
                auto dsetSelectedSize = getSizeSelected(dsetSpace);
                if(getSizeSelected(dataSpace) != getSizeSelected(dsetSpace)) {
                    auto msg1 = getSpaceString(dataSpace);
                    auto msg2 = getSpaceString(dsetSpace);
                    throw std::runtime_error(h5pp::format("Hyperslab selections are not equal size. Selected elements: Data {} | Dataset {}"
                                                          "\n\t data space \t {} \n\t dset space \t {}",
                                                          dataSelectedSize,
                                                          dsetSelectedSize,
                                                          msg1,
                                                          msg2));
                }
            } else {
                // Compare the dimensions
                if(getDimensions(dataSpace) == getDimensions(dsetSpace)) return;
                if(getSize(dataSpace) != getSize(dsetSpace)) {
                    auto msg1 = getSpaceString(dataSpace);
                    auto msg2 = getSpaceString(dsetSpace);
                    throw std::runtime_error(
                        h5pp::format("Spaces are not equal size \n\t data space \t {} \n\t dset space \t {}", msg1, msg2));
                } else if(getDimensions(dataSpace) != getDimensions(dsetSpace)) {
                    h5pp::logger::log->debug("Spaces have different shape:");
                    h5pp::logger::log->debug(" data space {}", getSpaceString(dataSpace, h5pp::logger::logIf(1)));
                    h5pp::logger::log->debug(" dset space {}", getSpaceString(dsetSpace, h5pp::logger::logIf(1)));
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

            std::string_view linkPath(name); // <-- This is the full path to the object that we are currently visiting.

            // If this group is deeper than maxDepth, just return
            if(maxDepth >= 0 and std::count(linkPath.begin(), linkPath.end(), '/') > maxDepth) return 0;

            // Get the name of the object without the full path, to match the searchKey
            auto slashpos = linkPath.rfind('/');
            if(slashpos == std::string_view::npos) slashpos = 0;
            std::string_view linkName = linkPath.substr(slashpos);

            auto matchList = reinterpret_cast<std::vector<std::string> *>(opdata);
            try {
                if constexpr(std::is_same_v<InfoType, H5O_info_t>) {
                    if(info->type == ObjType or ObjType == H5O_TYPE_UNKNOWN) {
                        if(searchKey.empty() or linkName.find(searchKey) != std::string::npos) matchList->push_back(name);
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
                        if(searchKey.empty() or linkName.find(searchKey) != std::string::npos) matchList->push_back(name);
                    }
                } else {
                    if(searchKey.empty() or linkName.find(searchKey) != std::string::npos) { matchList->push_back(name); }
                }

                if(maxHits > 0 and static_cast<long>(matchList->size()) >= maxHits)
                    return 1;
                else
                    return 0;
            } catch(...) { throw std::logic_error(h5pp::format("Could not match object [{}] | loc_id [{}]", name, id)); }
        }

        template<H5O_type_t ObjType>
        [[nodiscard]] inline constexpr std::string_view getObjTypeName() {
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

        template<H5O_type_t ObjType, typename h5x>
        herr_t H5L_custom_iterate_by_name(const h5x                &loc,
                                          std::string_view          root,
                                          std::vector<std::string> &matchList,
                                          const hid::h5p           &linkAccess = H5P_DEFAULT) {
            H5Eset_auto(H5E_DEFAULT, nullptr, nullptr); // Silence the error we get from using index directly
            constexpr size_t maxsize           = 512;
            char             linkname[maxsize] = {};
            H5O_info_t       oInfo;
            /* clang-format off */
            for(hsize_t idx = 0; idx < 100000000; idx++) {
                ssize_t namesize = H5Lget_name_by_idx(loc, util::safe_str(root).c_str(), H5_index_t::H5_INDEX_NAME,
                                                      H5_iter_order_t::H5_ITER_NATIVE, idx,linkname,maxsize,linkAccess);
                if(namesize <= 0) {
                    H5Eclear(H5E_DEFAULT);
                    break;
                }
                std::string_view linkPath(linkname, static_cast<size_t>(namesize)); // <-- This is the full path to the object that we are currently visiting.

                // If this group is deeper than maxDepth, just return
                if(maxDepth >= 0 and std::count(linkPath.begin(), linkPath.end(), '/') > maxDepth) return 0;

                // Get the name of the object without the full path, to match the searchKey
                auto slashpos = linkPath.rfind('/');
                if(slashpos == std::string_view::npos) slashpos = 0;
                std::string_view linkName = linkPath.substr(slashpos);



                if(ObjType == H5O_TYPE_UNKNOWN) {
                    // Accept based on name
                    if(searchKey.empty() or linkName.find(searchKey) != std::string::npos) matchList.emplace_back(linkname);
                    if(maxHits > 0 and static_cast<long>(matchList.size()) >= maxHits) return 0;
                } else {
                    // Name is not enough to establish a match. Check type.
                    #if defined(H5Oget_info_by_idx_vers) && H5Oget_info_by_idx_vers >= 2
                    herr_t res = H5Oget_info_by_idx(loc, util::safe_str(root).c_str(),
                                                    H5_index_t::H5_INDEX_NAME, H5_iter_order_t::H5_ITER_NATIVE,
                                                    idx, &oInfo, H5O_INFO_BASIC,linkAccess );
                    #else
                    herr_t res = H5Oget_info_by_idx(loc, util::safe_str(root).c_str(),
                                                    H5_index_t::H5_INDEX_NAME, H5_iter_order_t::H5_ITER_NATIVE,
                                                    idx, &oInfo, linkAccess );
                    #endif
                    if(res < 0) {
                        H5Eclear(H5E_DEFAULT);
                        break;
                    }
                    if(oInfo.type == ObjType and (searchKey.empty() or linkName.find(searchKey) != std::string::npos))
                        matchList.emplace_back(linkname);
                    if(maxHits > 0 and static_cast<long>(matchList.size()) >= maxHits) return 0;
                }
            }
            /* clang-format on */
            return 0;
        }

        template<H5O_type_t ObjType, typename h5x>
        inline herr_t visit_by_name(const h5x                &loc,
                                    std::string_view          root,
                                    std::vector<std::string> &matchList,
                                    const hid::h5p           &linkAccess = H5P_DEFAULT) {
            static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x>,
                          "Template function [h5pp::hdf5::visit_by_name(const h5x & loc, ...)] requires type h5x to be: "
                          "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
            if(internal::maxDepth == 0)
                // Faster when we don't need to iterate recursively
                return H5L_custom_iterate_by_name<ObjType>(loc, root, matchList, linkAccess);

#if defined(H5Ovisit_by_name_vers) && H5Ovisit_by_name_vers >= 2
            return H5Ovisit_by_name(loc,
                                    util::safe_str(root).c_str(),
                                    H5_INDEX_NAME,
                                    H5_ITER_NATIVE,
                                    internal::matcher<ObjType>,
                                    &matchList,
                                    H5O_INFO_BASIC,
                                    linkAccess);
#else
            return H5Ovisit_by_name(loc,
                                    util::safe_str(root).c_str(),
                                    H5_INDEX_NAME,
                                    H5_ITER_NATIVE,
                                    internal::matcher<ObjType>,
                                    &matchList,
                                    linkAccess);
#endif
        }

    }

    template<H5O_type_t ObjType, typename h5x>
    [[nodiscard]] inline std::vector<std::string> findLinks(const h5x       &loc,
                                                            std::string_view searchKey  = "",
                                                            std::string_view searchRoot = "/",
                                                            long             maxHits    = -1,
                                                            long             maxDepth   = -1,
                                                            const hid::h5p  &linkAccess = H5P_DEFAULT) {
        h5pp::logger::log->trace("search key: {} | root: {} | type: {} | max hits {} | max depth {}",
                                 searchKey,
                                 searchRoot,
                                 internal::getObjTypeName<ObjType>(),
                                 maxHits,
                                 maxDepth);

        if(not checkIfLinkExists(loc, searchRoot, linkAccess)) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Cannot find links inside group [{}]: it does not exist", searchRoot));
        }

        std::vector<std::string> matchList;
        internal::maxHits   = maxHits;
        internal::maxDepth  = maxDepth;
        internal::searchKey = searchKey;
        herr_t err          = internal::visit_by_name<ObjType>(loc, searchRoot, matchList, linkAccess);
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(
                h5pp::format("Error occurred when trying to find links of type [{}] containing [{}] while iterating from root [{}]",
                             internal::getObjTypeName<ObjType>(),
                             searchKey,
                             searchRoot));
        }
        return matchList;
    }

    template<H5O_type_t ObjType, typename h5x>
    [[nodiscard]] inline std::vector<std::string>
        getContentsOfLink(const h5x &loc, std::string_view linkPath, long maxDepth = 1, const hid::h5p &linkAccess = H5P_DEFAULT) {
        std::vector<std::string> contents;
        internal::maxHits  = -1;
        internal::maxDepth = maxDepth;
        internal::searchKey.clear();
        herr_t err = internal::visit_by_name<ObjType>(loc, linkPath, contents, linkAccess);
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(
                h5pp::format("Failed to iterate link [{}] of type [{}]", linkPath, internal::getObjTypeName<ObjType>()));
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
        h5pp::logger::log->debug("Creating dataset {}", dsetInfo.string(h5pp::logger::logIf(1)));
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
            h5pp::logger::log->trace("No need to create attribute [{}] in link [{}]: exists already",
                                     attrInfo.attrName.value(),
                                     attrInfo.linkPath.value());
            return;
        }
        h5pp::logger::log->trace("Creating attribute {}", attrInfo.string(h5pp::logger::logIf(0)));
        hid_t attrId = H5Acreate(attrInfo.h5Link.value(),
                                 util::safe_str(attrInfo.attrName.value()).c_str(),
                                 attrInfo.h5Type.value(),
                                 attrInfo.h5Space.value(),
                                 attrInfo.h5PlistAttrCreate.value(),
                                 attrInfo.h5PlistAttrAccess.value());
        if(attrId <= 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(
                h5pp::format("Failed to create attribute [{}] for link [{}]", attrInfo.attrName.value(), attrInfo.linkPath.value()));
        }
        attrInfo.h5Attr     = attrId;
        attrInfo.attrExists = true;
    }

    template<typename DataType>
    [[nodiscard]] std::vector<const char *> getCharPtrVector(const DataType &data) {
        std::vector<const char *> sv;
        if constexpr(h5pp::type::sfinae::is_text_v<DataType> and h5pp::type::sfinae::has_data_v<DataType>) // Takes care of std::string
            sv.push_back(data.data());
        else if constexpr(h5pp::type::sfinae::is_text_v<DataType>) // Takes care of char pointers and arrays
            sv.push_back(data);
        else if constexpr(h5pp::type::sfinae::is_iterable_v<DataType>) // Takes care of containers with text
            for(auto &elem : data) {
                if constexpr(h5pp::type::sfinae::is_text_v<decltype(elem)> and
                             h5pp::type::sfinae::has_data_v<decltype(elem)>) // Takes care of containers with std::string
                    sv.push_back(elem.data());
                else if constexpr(h5pp::type::sfinae::is_text_v<decltype(elem)>) // Takes care of containers  of char pointers and arrays
                    sv.push_back(elem);
                else
                    sv.push_back(&elem); // Takes care of other things?
            }
        else
            throw std::runtime_error(
                h5pp::format("Failed to get char pointer of datatype [{}]", h5pp::type::sfinae::type_name<DataType>()));
        return sv;
    }

    template<typename DataType>
    void writeDataset(const DataType      &data,
                      const DataInfo      &dataInfo,
                      const DsetInfo      &dsetInfo,
                      const PropertyLists &plists = PropertyLists()) {
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
        h5pp::logger::log->debug("Writing from memory  {}", dataInfo.string(h5pp::logger::logIf(1)));
        h5pp::logger::log->debug("Writing into dataset {}", dsetInfo.string(h5pp::logger::logIf(1)));
        h5pp::hdf5::assertWriteBufferIsLargeEnough(data, dataInfo.h5Space.value(), dsetInfo.h5Type.value());
        h5pp::hdf5::assertBytesPerElemMatch<DataType>(dsetInfo.h5Type.value());
        h5pp::hdf5::assertSpacesEqual(dataInfo.h5Space.value(), dsetInfo.h5Space.value(), dsetInfo.h5Type.value());
        herr_t retval = 0;

        // Get the memory address to the data buffer
        [[maybe_unused]] auto dataPtr = h5pp::util::getVoidPointer<const void *>(data);

        if constexpr(h5pp::type::sfinae::is_text_v<DataType> or h5pp::type::sfinae::has_text_v<DataType>) {
            auto vec = getCharPtrVector(data);
            // When H5T_VARIABLE, this function expects [const char **], which is what we get from vec.data()
            if(H5Tis_variable_str(dsetInfo.h5Type->value()) > 0)
                retval = H5Dwrite(dsetInfo.h5Dset.value(),
                                  dsetInfo.h5Type.value(),
                                  dataInfo.h5Space.value(),
                                  dsetInfo.h5Space.value(),
                                  plists.dsetXfer,
                                  vec.data());
            else {
                if(vec.size() == 1) {
                    retval = H5Dwrite(dsetInfo.h5Dset.value(),
                                      dsetInfo.h5Type.value(),
                                      dataInfo.h5Space.value(),
                                      dsetInfo.h5Space.value(),
                                      plists.dsetXfer,
                                      *vec.data());
                } else {
                    if constexpr(h5pp::type::sfinae::has_text_v<DataType> and h5pp::type::sfinae::is_iterable_v<DataType>) {
                        // We have a fixed-size string array now. We have to copy the strings to a contiguous array.
                        // vdata already contains the pointer to each string, and bytes should be the size of the whole array
                        // including null terminators. so
                        std::string strContiguous;
                        size_t      bytesPerStr = H5Tget_size(dsetInfo.h5Type.value()); // Includes null term
                        strContiguous.resize(bytesPerStr * vec.size());
                        for(size_t i = 0; i < vec.size(); i++) {
                            auto start_src = strContiguous.data() + static_cast<long>(i * bytesPerStr);
                            // Construct a view of the null-terminated character string, not including the null character.
                            auto view      = std::string_view(vec[i]); // view.size() will not include null term here!
                            std::copy_n(std::begin(view), std::min(view.size(), bytesPerStr - 1), start_src); // Do not copy null character
                        }
                        retval = H5Dwrite(dsetInfo.h5Dset.value(),
                                          dsetInfo.h5Type.value(),
                                          dataInfo.h5Space.value(),
                                          dsetInfo.h5Space.value(),
                                          plists.dsetXfer,
                                          strContiguous.data());
                    } else {
                        // Assume contigous array and hope for the best
                        retval = H5Dwrite(dsetInfo.h5Dset.value(),
                                          dsetInfo.h5Type.value(),
                                          dataInfo.h5Space.value(),
                                          dsetInfo.h5Space.value(),
                                          plists.dsetXfer,
                                          dataPtr);
                    }
                }
            }
        } else
            retval = H5Dwrite(dsetInfo.h5Dset.value(),
                              dsetInfo.h5Type.value(),
                              dataInfo.h5Space.value(),
                              dsetInfo.h5Space.value(),
                              plists.dsetXfer,
                              dataPtr);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(
                h5pp::format("Failed to write into dataset \n\t {} \n from memory \n\t {}", dsetInfo.string(), dataInfo.string()));
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
        h5pp::logger::log->debug("Reading into memory  {}", dataInfo.string(h5pp::logger::logIf(1)));
        h5pp::logger::log->debug("Reading from dataset {}", dsetInfo.string(h5pp::logger::logIf(1)));
        h5pp::hdf5::assertReadTypeIsLargeEnough<DataType>(dsetInfo.h5Type.value());
        h5pp::hdf5::assertReadSpaceIsLargeEnough(data, dataInfo.h5Space.value(), dsetInfo.h5Type.value());
        h5pp::hdf5::assertSpacesEqual(dataInfo.h5Space.value(), dsetInfo.h5Space.value(), dsetInfo.h5Type.value());
        //        h5pp::hdf5::assertBytesPerElemMatch<DataType>(dsetInfo.h5Type.value());
        herr_t retval = 0;

        // Get the memory address to the data buffer
        [[maybe_unused]] auto dataPtr = h5pp::util::getVoidPointer<void *>(data);

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
                retval = H5Dread(dsetInfo.h5Dset.value(),
                                 dsetInfo.h5Type.value(),
                                 H5S_ALL,
                                 dsetInfo.h5Space.value(),
                                 plists.dsetXfer,
                                 vdata.data());
                // Now vdata contains the whole dataset and we need to put the data into the user-given container.
                if constexpr(std::is_same_v<DataType, std::string>) {
                    // A vector of strings (vdata) can be put into a single string (data) with entries separated by new-lines
                    data.clear();
                    for(size_t i = 0; i < vdata.size(); i++) {
                        if(!vdata.empty() and vdata[i] != nullptr) data.append(vdata[i]);
                        if(i < vdata.size() - 1) data.append("\n");
                    }
                } else if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::string> and
                                    h5pp::type::sfinae::has_resize_v<DataType>) {
                    data.clear();
                    data.resize(vdata.size());
                    for(size_t i = 0; i < data.size(); i++) data[i] = std::string(vdata[i]);
                } else {
                    throw std::runtime_error(
                        "To read text-data, please use std::string or a container of std::string like std::vector<std::string>");
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
                retval = H5Dread(dsetInfo.h5Dset.value(),
                                 dsetInfo.h5Type.value(),
                                 dataInfo.h5Space.value(),
                                 dsetInfo.h5Space.value(),
                                 plists.dsetXfer,
                                 fdata.data());
                // Now fdata contains the whole dataset and we need to put the data into the user-given container.
                if constexpr(std::is_same_v<DataType, std::string>) {
                    // A vector of strings (fdata) can be put into a single string (data) with entries separated by new-lines
                    data.clear();
                    for(size_t i = 0; i < static_cast<size_t>(size); i++) {
                        data.append(fdata.substr(i * bytesPerString, bytesPerString));
                        if(data.size() < fdata.size() - 1) data.append("\n");
                    }
                    data.erase(std::find(data.begin(), data.end(), '\0'), data.end()); // Prune all but the last null terminator
                } else if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::string> and
                                    h5pp::type::sfinae::has_resize_v<DataType>) {
                    if(data.size() != static_cast<size_t>(size))
                        throw std::runtime_error(
                            h5pp::format("Given container of strings has the wrong size: dset size {} | container size {}",
                                         size,
                                         data.size()));
                    for(size_t i = 0; i < static_cast<size_t>(size); i++) {
                        // Each data[i] has type std::string, so we can use the std::string constructor to copy data
                        data[i] = std::string(fdata.data() + i * bytesPerString, bytesPerString);
                        // Prune away all null terminators except the last one
                        data[i].erase(std::find(data[i].begin(), data[i].end(), '\0'), data[i].end());
                    }
                } else {
                    throw std::runtime_error(
                        "To read text-data, please use std::string or a container of std::string like std::vector<std::string>");
                }
            }
        } else
            retval = H5Dread(dsetInfo.h5Dset.value(),
                             dsetInfo.h5Type.value(),
                             dataInfo.h5Space.value(),
                             dsetInfo.h5Space.value(),
                             plists.dsetXfer,
                             dataPtr);

        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(
                h5pp::format("Failed to read from dataset \n\t {} \n into memory \n\t {}", dsetInfo.string(), dataInfo.string()));
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
        h5pp::logger::log->debug("Writing from memory    {}", dataInfo.string(h5pp::logger::logIf(1)));
        h5pp::logger::log->debug("Writing into attribute {}", attrInfo.string(h5pp::logger::logIf(1)));
        h5pp::hdf5::assertWriteBufferIsLargeEnough(data, dataInfo.h5Space.value(), attrInfo.h5Type.value());
        h5pp::hdf5::assertBytesPerElemMatch<DataType>(attrInfo.h5Type.value());
        h5pp::hdf5::assertSpacesEqual(dataInfo.h5Space.value(), attrInfo.h5Space.value(), attrInfo.h5Type.value());
        herr_t retval = 0;

        // Get the memory address to the data buffer
        [[maybe_unused]] auto dataPtr = h5pp::util::getVoidPointer<const void *>(data);

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
            throw std::runtime_error(
                h5pp::format("Failed to write into attribute \n\t {} \n from memory \n\t {}", attrInfo.string(), dataInfo.string()));
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
        h5pp::logger::log->debug("Reading into memory {}", dataInfo.string(h5pp::logger::logIf(1)));
        h5pp::logger::log->debug("Reading from file   {}", attrInfo.string(h5pp::logger::logIf(1)));
        h5pp::hdf5::assertReadSpaceIsLargeEnough(data, dataInfo.h5Space.value(), attrInfo.h5Type.value());
        h5pp::hdf5::assertBytesPerElemMatch<DataType>(attrInfo.h5Type.value());
        h5pp::hdf5::assertSpacesEqual(dataInfo.h5Space.value(), attrInfo.h5Space.value(), attrInfo.h5Type.value());
        herr_t                retval  = 0;
        // Get the memory address to the data buffer
        [[maybe_unused]] auto dataPtr = h5pp::util::getVoidPointer<void *>(data);

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
                } else if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::string> and
                                    h5pp::type::sfinae::has_resize_v<DataType>) {
                    data.clear();
                    data.resize(vdata.size());
                    for(size_t i = 0; i < data.size(); i++) data[i] = std::string(vdata[i]);
                } else {
                    throw std::runtime_error(
                        "To read text-data, please use std::string or a container of std::string like std::vector<std::string>");
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
                } else if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::string> and
                                    h5pp::type::sfinae::has_resize_v<DataType>) {
                    data.clear();
                    data.resize(static_cast<size_t>(size));
                    for(size_t i = 0; i < static_cast<size_t>(size); i++) data[i] = fdata.substr(i * bytesPerString, bytesPerString);
                } else {
                    throw std::runtime_error(
                        "To read text-data, please use std::string or a container of std::string like std::vector<std::string>");
                }
            }
            if constexpr(std::is_same_v<DataType, std::string>) {
                data.erase(std::find(data.begin(), data.end(), '\0'), data.end()); // Prune all but the last null terminator
            }
        } else
            retval = H5Aread(attrInfo.h5Attr.value(), attrInfo.h5Type.value(), dataPtr);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(
                h5pp::format("Failed to read from attribute \n\t {} \n into memory \n\t {}", attrInfo.string(), dataInfo.string()));
        }
    }

    [[nodiscard]] inline bool fileIsValid(const fs::path &filePath) {
        return fs::exists(filePath) and H5Fis_hdf5(filePath.string().c_str()) > 0;
    }

    [[nodiscard]] inline fs::path getAvailableFileName(const fs::path &filePath) {
        int      i           = 1;
        fs::path newFileName = filePath;
        while(fs::exists(newFileName)) {
            newFileName.replace_filename(filePath.stem().string() + "-" + std::to_string(i++) + filePath.extension().string());
        }
        return newFileName;
    }

    [[nodiscard]] inline fs::path getBackupFileName(const fs::path &filePath) {
        int      i           = 1;
        fs::path newFilePath = filePath;
        while(fs::exists(newFilePath)) { newFilePath.replace_extension(filePath.extension().string() + ".bak_" + std::to_string(i++)); }
        return newFilePath;
    }

    [[nodiscard]] inline h5pp::FilePermission convertFileAccessFlags(unsigned int H5F_ACC_FLAGS) {
        h5pp::FilePermission permission = h5pp::FilePermission::RENAME;
        if((H5F_ACC_FLAGS & (H5F_ACC_TRUNC | H5F_ACC_EXCL)) == (H5F_ACC_TRUNC | H5F_ACC_EXCL))
            throw std::runtime_error("File access modes H5F_ACC_EXCL and H5F_ACC_TRUNC are mutually exclusive");
        if((H5F_ACC_FLAGS & H5F_ACC_RDONLY) == H5F_ACC_RDONLY) permission = h5pp::FilePermission::READONLY;
        if((H5F_ACC_FLAGS & H5F_ACC_RDWR) == H5F_ACC_RDWR) permission = h5pp::FilePermission::READWRITE;
        if((H5F_ACC_FLAGS & H5F_ACC_EXCL) == H5F_ACC_EXCL) permission = h5pp::FilePermission::COLLISION_FAIL;
        if((H5F_ACC_FLAGS & H5F_ACC_TRUNC) == H5F_ACC_TRUNC) permission = h5pp::FilePermission::REPLACE;
        return permission;
    }

    [[nodiscard]] inline unsigned int convertFileAccessFlags(h5pp::FilePermission permission) {
        unsigned int H5F_ACC_MODE = H5F_ACC_RDONLY;
        if(permission == h5pp::FilePermission::COLLISION_FAIL) H5F_ACC_MODE |= H5F_ACC_EXCL;
        if(permission == h5pp::FilePermission::REPLACE) H5F_ACC_MODE |= H5F_ACC_TRUNC;
        if(permission == h5pp::FilePermission::RENAME) H5F_ACC_MODE |= H5F_ACC_TRUNC;
        if(permission == h5pp::FilePermission::READONLY) H5F_ACC_MODE |= H5F_ACC_RDONLY;
        if(permission == h5pp::FilePermission::READWRITE) H5F_ACC_MODE |= H5F_ACC_RDWR;
        return H5F_ACC_MODE;
    }

    [[nodiscard]] inline fs::path
        createFile(const h5pp::fs::path &filePath_, const h5pp::FilePermission &permission, const PropertyLists &plists = PropertyLists()) {
        fs::path filePath = fs::absolute(filePath_);
        fs::path fileName = filePath_.filename();
        if(fs::exists(filePath)) {
            if(not fileIsValid(filePath)) h5pp::logger::log->debug("Pre-existing file may be corrupted [{}]", filePath.string());
            if(permission == h5pp::FilePermission::READONLY) return filePath;
            if(permission == h5pp::FilePermission::COLLISION_FAIL)
                throw std::runtime_error(h5pp::format("[COLLISION_FAIL]: Previous file exists with the same name [{}]", filePath.string()));
            if(permission == h5pp::FilePermission::RENAME) {
                auto newFilePath = getAvailableFileName(filePath);
                h5pp::logger::log->info("[RENAME]: Previous file exists. Choosing a new file name [{}] --> [{}]",
                                        filePath.filename().string(),
                                        newFilePath.filename().string());
                filePath = newFilePath;
                fileName = filePath.filename();
            }
            if(permission == h5pp::FilePermission::READWRITE) return filePath;
            if(permission == h5pp::FilePermission::BACKUP) {
                auto backupPath = getBackupFileName(filePath);
                h5pp::logger::log->info("[BACKUP]: Backing up existing file [{}] --> [{}]",
                                        filePath.filename().string(),
                                        backupPath.filename().string());
                fs::rename(filePath, backupPath);
            }
            if(permission == h5pp::FilePermission::REPLACE) {} // Do nothing
        } else {
            if(permission == h5pp::FilePermission::READONLY)
                throw std::runtime_error(h5pp::format("[READONLY]: File does not exist [{}]", filePath.string()));
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
        if(permission == h5pp::FilePermission::READONLY)
            throw std::logic_error("About to create/truncate a file even though READONLY was specified. This is a programming error!");

        // Go ahead
        hid_t file = H5Fcreate(filePath.string().c_str(), H5F_ACC_TRUNC, plists.fileCreate, plists.fileAccess);
        if(file < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to create file [{}]\n\t\t Check that you have the right permissions and that the "
                                                  "file is not locked by another program",
                                                  filePath.string()));
        }
        H5Fclose(file);
        return fs::canonical(filePath);
    }

    inline void createTable(TableInfo &info, const PropertyLists &plists = PropertyLists()) {
        info.assertCreateReady();
        h5pp::logger::log->debug("Creating table [{}] | num fields {} | record size {} bytes",
                                 info.tablePath.value(),
                                 info.numFields.value(),
                                 info.recordBytes.value());

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
        int    compression = info.compressionLevel.value() == 0 ? 0 : 1; // Only true/false (1/0). Is set to level 6 in HDF5 sources
        herr_t retval      = H5TBmake_table(util::safe_str(info.tableTitle.value()).c_str(),
                                            info.getLocId(),
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
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Could not create table [{}]", info.tablePath.value()));
        }
        h5pp::logger::log->trace("Successfully created table [{}]", info.tablePath.value());
        info.tableExists = true;
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    inline void readTableRecords(DataType             &data,
                                 const TableInfo      &info,
                                 std::optional<size_t> startIdx       = std::nullopt,
                                 std::optional<size_t> numReadRecords = std::nullopt) {
        /*
         *  This function replaces H5TBread_records() and avoids creating expensive temporaries for the dataset id and type id for the
         * compound table type.
         *
         */

        // If none of startIdx or numReadRecords are given:
        //          If data resizeable: startIdx = 0, numReadRecords = totalRecords
        //          If data not resizeable: startIdx = last record, numReadRecords = 1.
        // If startIdx given but numReadRecords is not:
        //          If data resizeable -> read from startIdx to the end
        //          If data not resizeable -> read a single record starting from startIdx
        // If numReadRecords given but startIdx is not -> read the last numReadRecords records

        info.assertReadReady();
        if constexpr(std::is_same_v<DataType, std::vector<std::byte>>) {
            if(not numReadRecords)
                throw std::runtime_error("Optional argument [numReadRecords] is required when reading std::vector<std::byte> from table");
        }

        if(not startIdx and not numReadRecords) {
            if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) {
                startIdx       = 0;
                numReadRecords = info.numRecords.value();
            } else {
                startIdx       = info.numRecords.value() - 1;
                numReadRecords = 1;
            }
        } else if(startIdx and not numReadRecords) {
            if(startIdx.value() > info.numRecords.value() - 1)
                throw std::runtime_error(h5pp::format("Invalid start index {} for table [{}] | total records {}",
                                                      startIdx.value(),
                                                      info.tablePath.value(),
                                                      info.numRecords.value()));
            if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) {
                numReadRecords = info.numRecords.value() - startIdx.value();
            } else {
                numReadRecords = 1;
            }
        } else if(numReadRecords and not startIdx) {
            if(numReadRecords.value() > info.numRecords.value())
                throw std::logic_error(h5pp::format("Cannot read {} records from table [{}] which only has {} records",
                                                    numReadRecords.value(),
                                                    info.tablePath.value(),
                                                    info.numRecords.value()));
            startIdx = info.numRecords.value() - numReadRecords.value();
        }

        // Sanity check
        if(numReadRecords.value() > info.numRecords.value())
            throw std::logic_error(h5pp::format("Cannot read {} records from table [{}] which only has {} records",
                                                numReadRecords.value(),
                                                info.tablePath.value(),
                                                info.numRecords.value()));
        if(startIdx.value() + numReadRecords.value() > info.numRecords.value())
            throw std::logic_error(h5pp::format("Cannot read {} records starting from index {} from table [{}] which only has {} records",
                                                numReadRecords.value(),
                                                startIdx.value(),
                                                info.tablePath.value(),
                                                info.numRecords.value()));

        h5pp::logger::log->debug("Reading table [{}] | read from record {} | records to read {} | total records {} | record size {} bytes",
                                 info.tablePath.value(),
                                 startIdx.value(),
                                 numReadRecords.value(),
                                 info.numRecords.value(),
                                 info.recordBytes.value());

        if constexpr(not std::is_same_v<DataType, std::vector<std::byte>>) {
            // Make sure the given container and the registered table record type have the same size.
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
                                                      "Table is {} bytes per record",
                                                      info.tablePath.value(),
                                                      dataSize,
                                                      info.recordBytes.value()));
            h5pp::util::resizeData(data, {numReadRecords.value()});
        }

        // Last sanity check. If there are no records to read, just return;
        if(numReadRecords.value() == 0) return;
        if(info.numRecords.value() == 0) return;

        /* Step 1: Get the dataset and memory spaces */
        hid::h5s dsetSpace = H5Dget_space(info.h5Dset.value()); /* get a copy of the new file data space for writing */
        hid::h5s dataSpace = util::getMemSpace(numReadRecords.value(), {numReadRecords.value()}); /* create a simple memory data space */

        /* Step 2: draw a hyperslab in the dataset */
        h5pp::Hyperslab slab;
        slab.offset = {startIdx.value()};
        slab.extent = {numReadRecords.value()};
        selectHyperslab(dsetSpace, slab, H5S_SELECT_SET);

        /* Step 3: read the records */
        // Get the memory address to the data buffer
        auto   dataPtr = h5pp::util::getVoidPointer<void *>(data);
        herr_t retval  = H5Dread(info.h5Dset.value(), info.h5Type.value(), dataSpace, dsetSpace, H5P_DEFAULT, dataPtr);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to read data from table [{}]", info.tablePath.value()));
        }
    }

    template<typename DataType>
    inline void appendTableRecords(const DataType &data, TableInfo &info, std::optional<size_t> numNewRecords = std::nullopt) {
        /*
         *  This function replaces H5TBappend_records() and avoids creating expensive temporaries for the dataset id and type id for the
         * compound table type.
         *
         */
        if constexpr(std::is_same_v<DataType, std::vector<std::byte>>)
            if(not numNewRecords)
                throw std::runtime_error("Optional argument [numNewRecords] is required when appending std::vector<std::byte> to table");
        if(not numNewRecords) numNewRecords = h5pp::util::getSize(data);
        if(numNewRecords.value() == 0)
            h5pp::logger::log->warn("Given 0 records to write to table [{}]. This is likely an error.", info.tablePath.value());
        h5pp::logger::log->debug("Appending {} records to table [{}] | current num records {} | record size {} bytes",
                                 numNewRecords.value(),
                                 info.tablePath.value(),
                                 info.numRecords.value(),
                                 info.recordBytes.value());
        info.assertWriteReady();
        if constexpr(not std::is_same_v<DataType, std::vector<std::byte>>) {
            // Make sure the given container and the registered table entry have the same size.
            // If there is a mismatch here it can cause horrible bugs/segfaults
            if constexpr(h5pp::type::sfinae::has_value_type_v<DataType>) {
                if(sizeof(typename DataType::value_type) != info.recordBytes.value())
                    throw std::runtime_error(h5pp::format("Size mismatch: Given container of type {} has elements of {} bytes, but the "
                                                          "table records on file are {} bytes each ",
                                                          h5pp::type::sfinae::type_name<DataType>(),
                                                          sizeof(typename DataType::value_type),
                                                          info.recordBytes.value()));
            } else if constexpr(h5pp::type::sfinae::has_data_v<DataType> and h5pp::type::sfinae::is_iterable_v<DataType>) {
                if(sizeof(&data.data()) != info.recordBytes.value())
                    throw std::runtime_error(h5pp::format("Size mismatch: Given container of type {} has elements of {} bytes, but the "
                                                          "table records on file are {} bytes each ",
                                                          h5pp::type::sfinae::type_name<DataType>(),
                                                          sizeof(&data.data()),
                                                          info.recordBytes.value()));
            } else {
                if(sizeof(DataType) != info.recordBytes.value())
                    throw std::runtime_error(
                        h5pp::format("Size mismatch: Given data type {} is of {} bytes, but the table records on file are {} bytes each ",
                                     h5pp::type::sfinae::type_name<DataType>(),
                                     sizeof(DataType),
                                     info.recordBytes.value()));
            }
        }

        /* Step 1: extend the dataset */
        extendDataset(info.h5Dset.value(), {numNewRecords.value() + info.numRecords.value()});

        /* Step 2: Get the dataset and memory spaces */
        hid::h5s dsetSpace = H5Dget_space(info.h5Dset.value()); /* get a copy of the new file data space for writing */
        hid::h5s dataSpace = util::getMemSpace(numNewRecords.value(), {numNewRecords.value()}); /* create a simple memory data space */

        /* Step 3: draw a hyperslab in the dataset */
        h5pp::Hyperslab slab;
        slab.offset = {info.numRecords.value()};
        slab.extent = {numNewRecords.value()};
        selectHyperslab(dsetSpace, slab, H5S_SELECT_SET);

        /* Step 4: write the records */
        // Get the memory address to the data buffer
        auto   dataPtr = h5pp::util::getVoidPointer<const void *>(data);
        herr_t retval  = H5Dwrite(info.h5Dset.value(), info.h5Type.value(), dataSpace, dsetSpace, H5P_DEFAULT, dataPtr);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to append data to table [{}]", info.tablePath.value()));
        }
        /* Step 5: increment the number of records in the table */
        info.numRecords.value() += numNewRecords.value();
    }

    template<typename DataType>
    inline void writeTableRecords(const DataType       &data,
                                  TableInfo            &info,
                                  size_t                startIdx          = 0,
                                  std::optional<size_t> numRecordsToWrite = std::nullopt) {
        /*
         *  This function replaces H5TBwrite_records() and avoids creating expensive temporaries for the dataset id and type id for the
         * compound table type. In addition, it has the ability to extend the existing the dataset if the incoming data larger than the
         * current bound
         */

        if constexpr(std::is_same_v<DataType, std::vector<std::byte>>) {
            if(not numRecordsToWrite)
                throw std::runtime_error(
                    "Optional argument [numRecordsToWrite] is required when writing std::vector<std::byte> into table");
        }
        if(not numRecordsToWrite) numRecordsToWrite = h5pp::util::getSize(data);
        if(numRecordsToWrite.value() == 0)
            h5pp::logger::log->warn("Given 0 records to write to table [{}]. This is likely an error.", info.tablePath.value());
        info.assertWriteReady();

        // Check that startIdx is smaller than the number of records on file, otherwise append the data
        if(startIdx >= info.numRecords.value())
            return h5pp::hdf5::appendTableRecords(data, info, numRecordsToWrite); // return appendTableRecords(data, info);

        h5pp::logger::log->debug("Writing {} records to table [{}] | start from {} | current num records {} | record size {} bytes",
                                 numRecordsToWrite.value(),
                                 info.tablePath.value(),
                                 startIdx,
                                 info.numRecords.value(),
                                 info.recordBytes.value());

        if constexpr(not std::is_same_v<DataType, std::vector<std::byte>>) {
            // Make sure the given data type size matches the table record type size.
            // If there is a mismatch here it can cause horrible bugs/segfaults
            if constexpr(h5pp::type::sfinae::has_value_type_v<DataType>) {
                if(sizeof(typename DataType::value_type) != info.recordBytes.value())
                    throw std::runtime_error(h5pp::format("Size mismatch: Given container of type {} has elements of {} bytes, but the "
                                                          "table records on file are {} bytes each ",
                                                          h5pp::type::sfinae::type_name<DataType>(),
                                                          sizeof(typename DataType::value_type),
                                                          info.recordBytes.value()));
            } else if constexpr(h5pp::type::sfinae::has_data_v<DataType> and h5pp::type::sfinae::is_iterable_v<DataType>) {
                if(sizeof(&data.data()) != info.recordBytes.value())
                    throw std::runtime_error(h5pp::format("Size mismatch: Given container of type {} has elements of {} bytes, but the "
                                                          "table records on file are {} bytes each ",
                                                          h5pp::type::sfinae::type_name<DataType>(),
                                                          sizeof(&data.data()),
                                                          info.recordBytes.value()));
            } else {
                if(sizeof(DataType) != info.recordBytes.value())
                    throw std::runtime_error(
                        h5pp::format("Size mismatch: Given data type {} is of {} bytes, but the table records on file are {} bytes each ",
                                     h5pp::type::sfinae::type_name<DataType>(),
                                     sizeof(DataType),
                                     info.recordBytes.value()));
            }
        }

        /* Step 1: extend the dataset if necessary */
        if(startIdx + numRecordsToWrite.value() > info.numRecords.value())
            extendDataset(info.h5Dset.value(), {startIdx + numRecordsToWrite.value()});

        /* Step 2: Get the dataset and memory spaces */
        hid::h5s dsetSpace = H5Dget_space(info.h5Dset.value()); /* get a copy of the new file data space for writing */
        hid::h5s dataSpace =
            util::getMemSpace(numRecordsToWrite.value(), {numRecordsToWrite.value()}); /* create a simple memory data space */

        /* Step 3: draw a hyperslab in the dataset */
        h5pp::Hyperslab slab;
        slab.offset = {startIdx};
        slab.extent = {numRecordsToWrite.value()};
        selectHyperslab(dsetSpace, slab, H5S_SELECT_SET);

        /* Step 4: write the records */
        // Get the memory address to the data buffer
        auto   dataPtr = h5pp::util::getVoidPointer<const void *>(data);
        herr_t retval  = H5Dwrite(info.h5Dset.value(), info.h5Type.value(), dataSpace, dsetSpace, H5P_DEFAULT, dataPtr);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Failed to append data to table [{}]", info.tablePath.value()));
        }
        info.numRecords.value() = std::max<size_t>(startIdx + numRecordsToWrite.value(), info.numRecords.value());
    }

    inline void copyTableRecords(const h5pp::TableInfo &srcInfo,
                                 hsize_t                srcStartIdx,
                                 hsize_t                numRecordsToCopy,
                                 h5pp::TableInfo       &tgtInfo,
                                 hsize_t                tgtStartIdx) {
        srcInfo.assertReadReady();
        tgtInfo.assertWriteReady();
        // Sanity checks for table types
        if(srcInfo.h5Type.value() != tgtInfo.h5Type.value())
            throw std::runtime_error(h5pp::format("Failed to add table records: table type mismatch"));
        if(srcInfo.recordBytes.value() != tgtInfo.recordBytes.value())
            throw std::runtime_error(h5pp::format("Failed to copy table records: table record byte size mismatch src {} != tgt {}",
                                                  srcInfo.recordBytes.value(),
                                                  tgtInfo.recordBytes.value()));
        if(srcInfo.fieldSizes.value() != tgtInfo.fieldSizes.value())
            throw std::runtime_error(h5pp::format("Failed to copy table records: table field sizes mismatch src {} != tgt {}",
                                                  srcInfo.fieldSizes.value(),
                                                  tgtInfo.fieldSizes.value()));
        if(srcInfo.fieldOffsets.value() != tgtInfo.fieldOffsets.value())
            throw std::runtime_error(h5pp::format("Failed to copy table records: table field offsets mismatch src {} != tgt {}",
                                                  srcInfo.fieldOffsets.value(),
                                                  tgtInfo.fieldOffsets.value()));

        // Sanity check for record ranges
        if(srcInfo.numRecords.value() < srcStartIdx + numRecordsToCopy)
            throw std::runtime_error(h5pp::format("Failed to copy table records: Requested records out of bound: src table nrecords {} | "
                                                  "src table start index {} | num records to copy {}",
                                                  srcInfo.numRecords.value(),
                                                  srcStartIdx,
                                                  numRecordsToCopy));

        std::string fileLogInfo;
        // TODO: this check is not very thorough, but checks with H5Iget_file_id are too expensive...
        if(srcInfo.h5File.value() != tgtInfo.h5File.value()) fileLogInfo = "on different files";
        h5pp::logger::log->debug("Copying records from table [{}] to table [{}] {} | src start at record {} ({} total) | tgt start at "
                                 "record {} ({} total) | copy {} records | record size {} bytes",
                                 srcInfo.tablePath.value(),
                                 tgtInfo.tablePath.value(),
                                 fileLogInfo,
                                 srcStartIdx,
                                 srcInfo.numRecords.value(),
                                 tgtStartIdx,
                                 tgtInfo.numRecords.value(),
                                 numRecordsToCopy,
                                 tgtInfo.recordBytes.value());

        std::vector<std::byte> data(numRecordsToCopy * tgtInfo.recordBytes.value());
        data.resize(numRecordsToCopy * tgtInfo.recordBytes.value());
        h5pp::hdf5::readTableRecords(data, srcInfo, srcStartIdx, numRecordsToCopy);
        h5pp::hdf5::writeTableRecords(data, tgtInfo, tgtStartIdx, numRecordsToCopy);
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    inline void readTableField(DataType                  &data,
                               const TableInfo           &info,
                               const std::vector<size_t> &srcFieldIndices, // Field indices for the table on file
                               std::optional<size_t>      startIdx       = std::nullopt,
                               std::optional<size_t>      numReadRecords = std::nullopt) {
        // If none of startIdx or numReadRecords are given:
        //          If data resizeable: startIdx = 0, numReadRecords = totalRecords
        //          If data not resizeable: startIdx = last record index, numReadRecords = 1.
        // If startIdx given but numReadRecords is not:
        //          If data resizeable -> read from startIdx to the end
        //          If data not resizeable -> read a single record starting from startIdx
        // If numReadRecords given but startIdx is not -> read the last numReadRecords records
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
                throw std::runtime_error(h5pp::format("Invalid start record {} for table [{}] | total records [{}]",
                                                      startIdx.value(),
                                                      info.tablePath.value(),
                                                      totalRecords));
            if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) {
                numReadRecords = totalRecords - startIdx.value();
            } else {
                numReadRecords = 1;
            }

        } else if(numReadRecords and not startIdx) {
            if(numReadRecords and numReadRecords.value() > totalRecords)
                throw std::logic_error(h5pp::format("Cannot read {} records from table [{}] which only has {} records",
                                                    numReadRecords.value(),
                                                    info.tablePath.value(),
                                                    totalRecords));
            startIdx = totalRecords - numReadRecords.value();
        }

        // Sanity check
        if(numReadRecords.value() > totalRecords)
            throw std::logic_error(h5pp::format("Cannot read {} records from table [{}] which only has {} records",
                                                numReadRecords.value(),
                                                info.tablePath.value(),
                                                totalRecords));
        if(startIdx.value() + numReadRecords.value() > totalRecords)
            throw std::logic_error(h5pp::format("Cannot read {} records starting from index {} from table [{}] which only has {} records",
                                                numReadRecords.value(),
                                                startIdx.value(),
                                                info.tablePath.value(),
                                                totalRecords));

        // Build the field sizes and offsets of the given read buffer based on the corresponding quantities on file
        std::vector<size_t>      srcFieldOffsets;
        std::vector<size_t>      tgtFieldOffsets;
        std::vector<size_t>      tgtFieldSizes;
        std::vector<std::string> tgtFieldNames;
        size_t                   tgtFieldSizeSum = 0;
        for(const auto &idx : srcFieldIndices) {
            srcFieldOffsets.emplace_back(info.fieldOffsets.value()[idx]);
            tgtFieldOffsets.emplace_back(tgtFieldSizeSum);
            tgtFieldSizes.emplace_back(info.fieldSizes.value()[idx]);
            tgtFieldNames.emplace_back(info.fieldNames.value()[idx]);
            tgtFieldSizeSum += tgtFieldSizes.back();
        }

        // Make sure the data type of the given read buffer matches the size computed above.
        // If there is a mismatch here it can cause horrible bugs/segfaults
        size_t dataSize = 0;
        if constexpr(h5pp::type::sfinae::has_value_type_v<DataType>)
            dataSize = sizeof(typename DataType::value_type);
        else if constexpr(h5pp::type::sfinae::has_data_v<DataType> and h5pp::type::sfinae::is_iterable_v<DataType>)
            dataSize = sizeof(&data.data());
        else
            dataSize = sizeof(DataType);

        if(dataSize != tgtFieldSizeSum) {
            std::string error_msg = h5pp::format("Could not read fields {} from table [{}]\n", tgtFieldNames, info.tablePath.value());
            for(auto &idx : srcFieldIndices)
                error_msg += h5pp::format("{:<10} Field index {:<6} {:<24} = {} bytes\n",
                                          " ",
                                          idx,
                                          info.fieldNames.value()[idx],
                                          info.fieldSizes.value()[idx]);

            std::string dataTypeName;
            if constexpr(h5pp::type::sfinae::has_value_type_v<DataType>)
                dataTypeName = h5pp::type::sfinae::type_name<typename DataType::value_type>();
            else if constexpr(h5pp::type::sfinae::has_data_v<DataType> and h5pp::type::sfinae::is_iterable_v<DataType>)
                dataTypeName = h5pp::type::sfinae::type_name<decltype(&data.data())>();
            else
                dataTypeName = h5pp::type::sfinae::type_name<DataType>();
            error_msg += h5pp::format("{:<8} + {:-^60}\n", " ", "");
            error_msg += h5pp::format("{:<10} Fields total = {} bytes per record\n", " ", tgtFieldSizeSum);
            error_msg += h5pp::format("{:<10} Given buffer = {} bytes per record <{}>\n", " ", dataSize, dataTypeName);
            error_msg += h5pp::format("{:<10} Size mismatch\n", " ");
            error_msg += h5pp::format("{:<10} Hint: The buffer type <{}> may have been padded by the compiler\n", " ", dataTypeName);
            error_msg += h5pp::format("{:<10}       Consider declaring <{}> with __attribute__((packed, aligned(1)))\n", " ", dataTypeName);
            throw std::runtime_error(error_msg);
        }

        h5pp::util::resizeData(data, {numReadRecords.value()});
        h5pp::logger::log->debug("Reading table [{}] | field names {} | read from "
                                 "record {} | read num records {} | available "
                                 "records {} | record size {} bytes",
                                 info.tablePath.value(),
                                 tgtFieldNames,
                                 startIdx.value(),
                                 numReadRecords.value(),
                                 info.numRecords.value(),
                                 info.recordBytes.value());
        h5pp::logger::log->trace("Reading field indices {} sizes {} | offsets {} | offsets on dataset {}",
                                 srcFieldIndices,
                                 tgtFieldSizes,
                                 tgtFieldOffsets,
                                 srcFieldOffsets);

        // Get the memory address to the data buffer
        auto dataPtr = h5pp::util::getVoidPointer<void *>(data);

        /* Step 1: Get the dataset and memory spaces */
        hid::h5s dsetSpace = H5Dget_space(info.h5Dset.value()); /* get a copy of the new file data space for writing */
        hid::h5s dataSpace = util::getMemSpace(numReadRecords.value(), {numReadRecords.value()}); /* create a simple memory data space */

        /* Step 2: draw a hyperslab in the dataset */
        h5pp::Hyperslab slab;
        slab.offset = {startIdx.value()};
        slab.extent = {numReadRecords.value()};
        selectHyperslab(dsetSpace, slab, H5S_SELECT_SET);

        /* Step 3: Create a special tgtTypeId for reading a subset of the record with the following properties:
         *      - tgtTypeId has the size of the given buffer type, i.e. dataSize.
         *      - only the fields to read are defined in it
         *      - the defined fields are converted to native types
         *      Then H5Dread will take care of only reading the relevant components of the record
         */

        hid::h5t tgtTypeId = H5Tcreate(H5T_COMPOUND, dataSize);
        for(size_t tgtIdx = 0; tgtIdx < srcFieldIndices.size(); tgtIdx++) {
            size_t   srcIdx           = srcFieldIndices[tgtIdx];
            hid::h5t temp_member_id   = H5Tget_native_type(info.fieldTypes.value()[srcIdx], H5T_DIR_DEFAULT);
            size_t   temp_member_size = H5Tget_size(temp_member_id);
            if(tgtFieldSizes[tgtIdx] != temp_member_size) H5Tset_size(temp_member_id, tgtFieldSizes[tgtIdx]);
            H5Tinsert(tgtTypeId, tgtFieldNames[tgtIdx].c_str(), tgtFieldOffsets[tgtIdx], temp_member_id);
        }

        /* Read data */
        herr_t retval = H5Dread(info.h5Dset.value(), tgtTypeId, dataSpace, dsetSpace, H5P_DEFAULT, dataPtr);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Could not read table fields {} on table [{}]", tgtFieldNames, info.tablePath.value()));
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_const_v<DataType>>>
    inline void readTableField(DataType                       &data,
                               const TableInfo                &info,
                               const std::vector<std::string> &fieldNames,
                               std::optional<size_t>           startIdx       = std::nullopt,
                               std::optional<size_t>           numReadRecords = std::nullopt) {
        // Compute the field indices
        std::vector<size_t> fieldIndices;
        for(const auto &fieldName : fieldNames) {
            auto it = std::find(info.fieldNames->begin(), info.fieldNames->end(), fieldName);
            if(it == info.fieldNames->end())
                throw std::runtime_error(h5pp::format("Could not find field [{}] in table [{}]: "
                                                      "Available field names are {}",
                                                      fieldName,
                                                      info.tablePath.value(),
                                                      info.fieldNames.value()));
            else
                fieldIndices.emplace_back(static_cast<size_t>(std::distance(info.fieldNames->begin(), it)));
        }
        readTableField(data, info, fieldIndices, startIdx, numReadRecords);
    }

    template<typename h5x_src,
             typename h5x_tgt,
             // enable_if so the compiler doesn't think it can use overload with std::string those arguments
             typename = h5pp::type::sfinae::enable_if_is_h5_loc_or_hid_t<h5x_src>,
             typename = h5pp::type::sfinae::enable_if_is_h5_loc_or_hid_t<h5x_tgt>>
    inline void copyLink(const h5x_src       &srcLocId,
                         std::string_view     srcLinkPath,
                         const h5x_tgt       &tgtLocId,
                         std::string_view     tgtLinkPath,
                         const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x_src>,
                      "Template function [h5pp::hdf5::copyLink(const h5x_src & srcLocId, ...)] requires type h5x_src to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x_tgt>,
                      "Template function [h5pp::hdf5::copyLink(..., ..., const h5x_tgt & tgtLocId, ...)] requires type h5x_tgt to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");

        h5pp::logger::log->trace("Copying link [{}] --> [{}]", srcLinkPath, tgtLinkPath);
        // Copy the link srcLinkPath to tgtLinkPath. Note that H5Ocopy does this recursively, so we don't need
        // to iterate links recursively here.
        auto retval = H5Ocopy(srcLocId,
                              util::safe_str(srcLinkPath).c_str(),
                              tgtLocId,
                              util::safe_str(tgtLinkPath).c_str(),
                              H5P_DEFAULT,
                              plists.linkCreate);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(h5pp::format("Could not copy link [{}] --> [{}]", srcLinkPath, tgtLinkPath));
        }
    }

    template<typename h5x_src,
             typename h5x_tgt,
             // enable_if so the compiler doesn't think it can use overload with fs::path those arguments
             typename = h5pp::type::sfinae::enable_if_is_h5_loc_or_hid_t<h5x_src>,
             typename = h5pp::type::sfinae::enable_if_is_h5_loc_or_hid_t<h5x_tgt>>
    inline void moveLink(const h5x_src       &srcLocId,
                         std::string_view     srcLinkPath,
                         const h5x_tgt       &tgtLocId,
                         std::string_view     tgtLinkPath,
                         LocationMode         locationMode = LocationMode::DETECT,
                         const PropertyLists &plists       = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x_src>,
                      "Template function [h5pp::hdf5::moveLink(const h5x_src & srcLocId, ...)] requires type h5x_src to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        static_assert(h5pp::type::sfinae::is_h5_loc_or_hid_v<h5x_tgt>,
                      "Template function [h5pp::hdf5::moveLink(..., ..., const h5x_tgt & tgtLocId, ...)] requires type h5x_tgt to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");

        h5pp::logger::log->trace("Moving link [{}] --> [{}]", srcLinkPath, tgtLinkPath);
        // Move the link srcLinkPath to tgtLinkPath. Note that H5Lmove only works inside a single file.
        // For different files we should do H5Ocopy followed by H5Ldelete
        bool sameFile = h5pp::util::onSameFile(srcLocId, tgtLocId, locationMode);

        if(sameFile) {
            // Same file
            auto retval = H5Lmove(srcLocId,
                                  util::safe_str(srcLinkPath).c_str(),
                                  tgtLocId,
                                  util::safe_str(tgtLinkPath).c_str(),
                                  plists.linkCreate,
                                  plists.linkAccess);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Could not copy link [{}] --> [{}]", srcLinkPath, tgtLinkPath));
            }
        } else {
            // Different files
            auto retval = H5Ocopy(srcLocId,
                                  util::safe_str(srcLinkPath).c_str(),
                                  tgtLocId,
                                  util::safe_str(tgtLinkPath).c_str(),
                                  H5P_DEFAULT,
                                  plists.linkCreate);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Could not copy link [{}] --> [{}]", srcLinkPath, tgtLinkPath));
            }
            retval = H5Ldelete(srcLocId, util::safe_str(srcLinkPath).c_str(), plists.linkAccess);
            if(retval < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Could not delete link after move [{}]", srcLinkPath));
            }
        }
    }

    inline void copyLink(const h5pp::fs::path &srcFilePath,
                         std::string_view      srcLinkPath,
                         const h5pp::fs::path &tgtFilePath,
                         std::string_view      tgtLinkPath,
                         FilePermission        targetFileCreatePermission = FilePermission::READWRITE,
                         const PropertyLists  &plists                     = PropertyLists()) {
        h5pp::logger::log->trace("Copying link: source link [{}] | source file [{}]  -->  target link [{}] | target file [{}]",
                                 srcLinkPath,
                                 srcFilePath.string(),
                                 tgtLinkPath,
                                 tgtFilePath.string());

        try {
            auto srcPath = fs::absolute(srcFilePath);
            if(not fs::exists(srcPath))
                throw std::runtime_error(h5pp::format("Could not copy link [{}] from file [{}]: source file does not exist [{}]",
                                                      srcLinkPath,
                                                      srcFilePath.string(),
                                                      srcPath.string()));
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
            throw std::runtime_error(
                h5pp::format("Could not copy link [{}] from file [{}]: {}", srcLinkPath, srcFilePath.string(), ex.what()));
        }
    }

    inline fs::path copyFile(const h5pp::fs::path &srcFilePath,
                             const h5pp::fs::path &tgtFilePath,
                             FilePermission        permission = FilePermission::COLLISION_FAIL,
                             const PropertyLists  &plists     = PropertyLists()) {
        h5pp::logger::log->trace("Copying file [{}] --> [{}]", srcFilePath.string(), tgtFilePath.string());
        auto tgtPath = h5pp::hdf5::createFile(tgtFilePath, permission, plists);
        auto srcPath = fs::absolute(srcFilePath);
        try {
            if(not fs::exists(srcPath))
                throw std::runtime_error(h5pp::format("Could not copy file [{}] --> [{}]: source file does not exist [{}]",
                                                      srcFilePath.string(),
                                                      tgtFilePath.string(),
                                                      srcPath.string()));
            if(tgtPath == srcPath)
                h5pp::logger::log->debug("Skipped copying file: source and target files have the same path [{}]", srcPath.string());

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
                    throw std::runtime_error(h5pp::format(
                        "Failed to copy file contents with H5Ocopy(srcFile,{},tgtFile,{},H5P_DEFAULT,link_create_propery_list)",
                        link,
                        link));
                }
            }
            // ... Find out how to copy attributes that are written on the root itself
            return tgtPath;
        } catch(const std::exception &ex) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(
                h5pp::format("Could not copy file [{}] --> [{}]: ", srcFilePath.string(), tgtFilePath.string(), ex.what()));
        }
    }

    inline void moveLink(const h5pp::fs::path &srcFilePath,
                         std::string_view      srcLinkPath,
                         const h5pp::fs::path &tgtFilePath,
                         std::string_view      tgtLinkPath,
                         FilePermission        targetFileCreatePermission = FilePermission::READWRITE,
                         const PropertyLists  &plists                     = PropertyLists()) {
        h5pp::logger::log->trace("Moving link: source link [{}] | source file [{}]  -->  target link [{}] | target file [{}]",
                                 srcLinkPath,
                                 srcFilePath.string(),
                                 tgtLinkPath,
                                 tgtFilePath.string());

        try {
            auto srcPath = fs::absolute(srcFilePath);
            if(not fs::exists(srcPath))
                throw std::runtime_error(
                    h5pp::format("Could not move link [{}] from file [{}]:\n\t source file with absolute path [{}] does not exist",
                                 srcLinkPath,
                                 srcFilePath.string(),
                                 srcPath.string()));
            auto tgtPath = h5pp::hdf5::createFile(tgtFilePath, targetFileCreatePermission, plists);

            hid_t hidSrc = H5Fopen(srcPath.string().c_str(), H5F_ACC_RDWR, plists.fileAccess);
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

            auto locMode = h5pp::util::getLocationMode(srcFilePath, tgtFilePath);
            moveLink(srcFile, srcLinkPath, tgtFile, tgtLinkPath, locMode);
        } catch(const std::exception &ex) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error(
                h5pp::format("Could not move link [{}] from file [{}]: {}", srcLinkPath, srcFilePath.string(), ex.what()));
        }
    }

    inline fs::path moveFile(const h5pp::fs::path &src,
                             const h5pp::fs::path &tgt,
                             FilePermission        permission = FilePermission::COLLISION_FAIL,
                             const PropertyLists  &plists     = PropertyLists()) {
        h5pp::logger::log->trace("Moving file by copy+remove: [{}] --> [{}]", src.string(), tgt.string());
        auto tgtPath = copyFile(src, tgt, permission, plists); // Returns the path to the newly created file
        auto srcPath = fs::absolute(src);
        if(fs::exists(tgtPath)) {
            h5pp::logger::log->trace("Removing file [{}]", srcPath.string());
            try {
                fs::remove(srcPath);
            } catch(const std::exception &err) {
                throw std::runtime_error(
                    h5pp::format("Remove failed. File may be locked [{}] | what(): {} ", srcPath.string(), err.what()));
            }
            return tgtPath;
        } else
            throw std::runtime_error(h5pp::format("Could not copy file [{}] to target [{}]", srcPath.string(), tgt.string()));

        return tgtPath;
    }

}
