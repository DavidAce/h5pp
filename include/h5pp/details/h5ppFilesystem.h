#pragma once
#if __has_include(<filesystem>)
    #include <filesystem>
    #include <utility>
namespace h5pp {
    namespace fs = std::filesystem;
}
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
namespace h5pp {
    namespace fs = std::experimental::filesystem;
}
#else
    #error Could not find includes: <filesystem> or <experimental/filesystem>
#endif
