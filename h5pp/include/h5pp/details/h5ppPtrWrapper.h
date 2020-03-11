#pragma once
#include <array>
#include <cassert>
#include <numeric>
#include <vector>

/*! An STL-style wrapper for raw C-style arrays
 * We do not need the whole iterator machinery in h5pp,
 * so we only implement the most basic things.
 * In addition this wrapper supports a description of the
 * dimensions of the data.
 * */

namespace h5pp {
    template<typename PointerType, size_t N = 1, typename = std::enable_if_t<std::is_pointer_v<PointerType> or std::is_array_v<PointerType>>>
    class PtrWrapper {
        private:
        const PointerType         data_;
        const std::vector<size_t> dims_;
        const std::size_t         size_;

        public:
        constexpr static size_t NumIndices = N;

        PtrWrapper(const PointerType data, const size_t size) : data_(data), dims_({size}), size_(size) { assert(N == dims_.size() and "Dimension mismatch"); }

        template<typename T>
        PtrWrapper(const PointerType data, const T (&dims)[N])
            : data_(data), dims_(std::begin(dims), std::end(dims)), size_(std::accumulate(std::begin(dims), std::end(dims), 1.0, std::multiplies<>())) {
            static_assert(std::is_integral_v<T> and "Type of dimension array must be integral");
            assert(N == dims_.size() and "Dimension mismatch");
        }

        template<typename T>
        PtrWrapper(const PointerType data, const std::array<T, N> &dims)
            : data_(data), dims_(dims.begin(), dims.end()), size_(std::accumulate(dims.begin(), dims.end(), (T) 1, std::multiplies<>())) {
            static_assert(std::is_integral_v<T> and "Type of dimension array must be integral");
            assert(N == dims_.size() and "Dimension mismatch");
        }

        template<typename T>
        PtrWrapper(const PointerType data, const std::vector<T> &dims) : data_(data), dims_(dims), size_(std::accumulate(dims.begin(), dims.end(), (T) 1, std::multiplies<>())) {
            static_assert(std::is_integral_v<T> and "Type of dimension array must be integral");
            assert(N == dims_.size() and "Dimension mismatch");
        }

        using value_type = std::remove_pointer_t<PointerType>;
        auto        data() const { return data_; }
        auto        size() const { return size_; }
        auto        begin() const { return data_; }
        auto        end() const { return data_ + size_; }
        auto        front() const { return *data_; }
        auto        back() const { return *(data_ + size_); }
        const auto &dimensions() const { return dims_; }
    };

}
