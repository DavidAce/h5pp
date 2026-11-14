#pragma once
// Include optional or experimental/optional
#if __has_include(<optional>)
    #include <optional>
#elif __has_include(<experimental/optional>)
    #include <experimental/optional>
namespace h5pp {
    constexpr const std::experimental::nullopt_t &nullopt = std::experimental::nullopt;
    template<typename T>
    using optional = std::experimental::optional<T>;
}
#else
    #error Could not find <optional> or <experimental/optional>
#endif