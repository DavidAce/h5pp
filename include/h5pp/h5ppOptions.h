#pragma once

#include "h5ppDimensionType.h"
#include "h5ppEnums.h"
#include "h5ppHid.h"
#include "h5ppHyperslab.h"
#include <H5Dpublic.h>
#include <optional>
#include <string>
#include <vector>

namespace h5pp::inline v2 {
    /** @brief Unlimited maximum extent for a chunked dataset dimension. */
    inline constexpr hsize_t unlimited = H5S_UNLIMITED;
    /** @brief Dataset or buffer dimensions, one extent per axis. */
    using Dims                         = std::vector<hsize_t>;
    /** @brief Optional dataset or buffer dimensions. */
    using OptDims                      = std::optional<Dims>;

    /** @brief Properties used when creating a dataset. Existing datasets are left unchanged. */
    struct DatasetCreateOptions {
        /** @brief Initial dataset dimensions; inferred from data when supplied to create(data, options). */
        OptDims                     dims        = std::nullopt;
        /** @brief Maximum dimensions. Use unlimited for axes that may grow without a fixed bound. */
        OptDims                     dimsMax     = std::nullopt;
        /** @brief Chunk dimensions; chosen automatically for chunked storage when omitted. */
        OptDims                     dimsChunk   = std::nullopt;
        /** @brief Storage layout: compact, contiguous or chunked. Chunked storage permits resizing and compression. */
        std::optional<H5D_layout_t> h5Layout    = std::nullopt;
        /** @brief Deflate level from 0 (none) to 9 (maximum); -1 disables compression. */
        std::optional<int>          compression = std::nullopt;
        /** @brief Stored HDF5 datatype; inferred from data when omitted in create(data, options). */
        std::optional<hid::h5t>     h5Type      = std::nullopt;
    };

    /** @brief Buffer description and resize policy for Dataset::write(). */
    struct DatasetWriteOptions {
        /** @brief Full input-buffer dimensions, before selection. Required for pointers or reshaping flat buffers. */
        OptDims                           dims         = std::nullopt;
        /** @brief Optional selection in input-buffer coordinates. A direct write() slab argument takes precedence. */
        std::optional<Hyperslab>          dataSlab     = std::nullopt;
        /** @brief Datatype override used when inferring the write and creating a missing dataset. */
        std::optional<hid::h5t>           h5Type       = std::nullopt;
        /** @brief Override dataset resizing: FIT, GROW or OFF. Only chunked datasets can resize. */
        std::optional<h5pp::ResizePolicy> resizePolicy = std::nullopt;
    };

    /** @brief Buffer description for Dataset::append(). */
    struct DatasetAppendOptions {
        /** @brief Input-buffer dimensions. Required for pointer data. */
        OptDims                 dims   = std::nullopt;
        /** @brief HDF5 datatype override for the input. */
        std::optional<hid::h5t> h5Type = std::nullopt;
    };

    /** @brief Buffer description for Dataset::read() and Dataset::readInto(). */
    struct DatasetReadOptions {
        /** @brief Destination-buffer dimensions. Required for pointers or reshaping flat buffers. */
        OptDims                  dims     = std::nullopt;
        /** @brief Optional selection in destination-buffer coordinates. A direct readInto() slab argument takes precedence. */
        std::optional<Hyperslab> dataSlab = std::nullopt;
        /** @brief HDF5 datatype override for the read. */
        std::optional<hid::h5t>  h5Type   = std::nullopt;
    };

    /** @brief Buffer description for Attribute::write(). */
    struct AttributeWriteOptions {
        /** @brief Input-buffer dimensions. Required for pointer data. */
        OptDims                 dims   = std::nullopt;
        /** @brief Datatype override used when inferring the write and creating a missing attribute. */
        std::optional<hid::h5t> h5Type = std::nullopt;
    };

    /** @brief Buffer description for Attribute::read() and Attribute::readInto(). */
    struct AttributeReadOptions {
        /** @brief Destination-buffer dimensions. Required for pointer data. */
        OptDims                 dims   = std::nullopt;
        /** @brief HDF5 datatype override for the read. */
        std::optional<hid::h5t> h5Type = std::nullopt;
    };

    /** @brief Properties used when creating a table. Existing tables are left unchanged. */
    struct TableCreateOptions {
        /** @brief Descriptive table title. Required unless supplied directly to create(). */
        std::optional<std::string> title       = std::nullopt;
        /** @brief Chunk dimensions for the one-dimensional record dataset; inferred when omitted. */
        OptDims                    dimsChunk   = std::nullopt;
        /** @brief Deflate level from 0 (none) to 9 (maximum); -1 disables compression. */
        std::optional<int>         compression = std::nullopt;
        /** @brief Compound datatype describing one record. Required unless supplied directly to create(). */
        std::optional<hid::h5t>    h5Type      = std::nullopt;
    };
}
