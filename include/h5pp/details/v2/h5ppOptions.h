#pragma once

#include "../h5ppDimensionType.h"
#include "../h5ppEnums.h"
#include "../h5ppHid.h"
#include "../h5ppHyperslab.h"
#include <H5Dpublic.h>
#include <optional>
#include <string>
#include <vector>

namespace h5pp::v2 {
    inline constexpr hsize_t unlimited = H5S_UNLIMITED;
    using Dims                        = std::vector<hsize_t>;
    using OptDims                     = std::optional<Dims>;

    struct DatasetCreateOptions {
        OptDims                     dims         = std::nullopt;
        OptDims                     dimsMax      = std::nullopt;
        OptDims                     dimsChunk    = std::nullopt;
        std::optional<H5D_layout_t> h5Layout     = std::nullopt;
        std::optional<int>          compression  = std::nullopt;
        std::optional<hid::h5t>     h5Type       = std::nullopt;
    };

    struct DatasetWriteOptions {
        OptDims                            dims         = std::nullopt;
        std::optional<Hyperslab>           dataSlab     = std::nullopt;
        std::optional<hid::h5t>            h5Type       = std::nullopt;
        std::optional<h5pp::ResizePolicy>  resizePolicy = std::nullopt;
    };

    struct DatasetAppendOptions {
        OptDims                 dims   = std::nullopt;
        std::optional<hid::h5t> h5Type = std::nullopt;
    };

    struct DatasetReadOptions {
        OptDims                  dims     = std::nullopt;
        std::optional<Hyperslab> dataSlab = std::nullopt;
        std::optional<hid::h5t>  h5Type   = std::nullopt;
    };

    struct AttributeWriteOptions {
        OptDims                dims   = std::nullopt;
        std::optional<hid::h5t> h5Type    = std::nullopt;
    };

    struct AttributeReadOptions {
        OptDims                dims   = std::nullopt;
        std::optional<hid::h5t> h5Type    = std::nullopt;
    };

    struct TableCreateOptions {
        std::optional<std::string> title       = std::nullopt;
        OptDims                    dimsChunk   = std::nullopt;
        std::optional<int>         compression = std::nullopt;
        std::optional<hid::h5t>    h5Type      = std::nullopt;
    };
}
