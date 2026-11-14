
#pragma once

namespace h5pp::constants {
    static constexpr unsigned long maxSizeCompact    = 32 * 1024;  // Max size of compact datasets is 32 kB
    static constexpr unsigned long maxSizeContiguous = 512 * 1024; // Max size of contiguous datasets is 512 kB
    static constexpr unsigned long minChunkBytes     = 10 * 1024;  // 10 kB
    static constexpr unsigned long maxChunkBytes     = 500 * 1024; // 500 kB
}
