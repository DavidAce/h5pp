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
    inline constexpr hsize_t unlimited = H5S_UNLIMITED;        /*!< Unlimited maximum extent for a chunked dataset dimension. */
    using Dims                         = std::vector<hsize_t>; /*!< Dataset or buffer dimensions, one extent per axis. */
    using OptDims                      = std::optional<Dims>;  /*!< Optional dataset or buffer dimensions. */

    /*! Properties used when creating a dataset. Existing datasets are left unchanged. */
    struct DatasetCreateOptions {
        OptDims                     dims        = std::nullopt; /*!< Dataset dimensions; inferred from data when available. */
        OptDims                     dimsMax     = std::nullopt; /*!< Maximum dimensions; use unlimited for growing axes. */
        OptDims                     dimsChunk   = std::nullopt; /*!< Chunk dimensions; chosen automatically when omitted. */
        std::optional<H5D_layout_t> h5Layout    = std::nullopt; /*!< Storage layout; chunking allows resizing and compression. */
        std::optional<int>          compression = std::nullopt; /*!< Deflate level: -1 disabled, 0 none, 9 maximum. */
        std::optional<hid::h5t>     h5Type      = std::nullopt; /*!< Stored datatype; inferred from data when available. */
    };

    /*! Buffer description and resize policy for Dataset::write(). */
    struct DatasetWriteOptions {
        OptDims                           dims         = std::nullopt; /*!< Full buffer dimensions; required for pointers or reshaping. */
        std::optional<Hyperslab>          dataSlab     = std::nullopt; /*!< Memory selection; a direct slab argument takes precedence. */
        std::optional<hid::h5t>           h5Type       = std::nullopt; /*!< Datatype override for writing and creating datasets. */
        std::optional<h5pp::ResizePolicy> resizePolicy = std::nullopt; /*!< Resize policy: FIT, GROW or OFF; chunked datasets only. */
    };

    /*! Buffer description for Dataset::append(). */
    struct DatasetAppendOptions {
        OptDims                 dims   = std::nullopt; /*!< Input-buffer dimensions. Required for pointer data. */
        std::optional<hid::h5t> h5Type = std::nullopt; /*!< HDF5 datatype override for the input. */
    };

    /*! Buffer description for Dataset::read() and Dataset::readInto(). */
    struct DatasetReadOptions {
        OptDims                  dims     = std::nullopt; /*!< Buffer dimensions; required for pointers or reshaping. */
        std::optional<Hyperslab> dataSlab = std::nullopt; /*!< Memory selection; a direct slab argument takes precedence. */
        std::optional<hid::h5t>  h5Type   = std::nullopt; /*!< HDF5 datatype override for the read. */
    };

    /*! Buffer description for Attribute::write(). */
    struct AttributeWriteOptions {
        OptDims                 dims   = std::nullopt; /*!< Input-buffer dimensions. Required for pointer data. */
        std::optional<hid::h5t> h5Type = std::nullopt; /*!< Datatype override for writing and creating attributes. */
    };

    /*! Buffer description for Attribute::read() and Attribute::readInto(). */
    struct AttributeReadOptions {
        OptDims                 dims   = std::nullopt; /*!< Destination-buffer dimensions. Required for pointer data. */
        std::optional<hid::h5t> h5Type = std::nullopt; /*!< HDF5 datatype override for the read. */
    };

    /*! Properties used when creating a table. Existing tables are left unchanged. */
    struct TableCreateOptions {
        std::optional<std::string> title       = std::nullopt; /*!< Table title; required unless supplied to create(). */
        OptDims                    dimsChunk   = std::nullopt; /*!< Record chunk dimensions; chosen automatically when omitted. */
        std::optional<int>         compression = std::nullopt; /*!< Deflate level: -1 disabled, 0 none, 9 maximum. */
        std::optional<hid::h5t>    h5Type      = std::nullopt; /*!< Record datatype; required unless supplied to create(). */
    };
}
