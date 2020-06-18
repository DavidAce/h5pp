
#pragma once

namespace h5pp::constants {
    static constexpr unsigned long maxSizeCompact    = 32 * 1024;  // Max size of compact datasets is 32 kb
    static constexpr unsigned long maxSizeContiguous = 512 * 1024; // Max size of contiguous datasets is 512 kb
    static constexpr unsigned long minChunkSize   = 10 * 1024;
    static constexpr unsigned long maxChunkSize   = 1000 * 1024;
}
