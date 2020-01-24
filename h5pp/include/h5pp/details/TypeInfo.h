#pragma once
#include "h5ppHid.h"
#include "h5ppUtils.h"
#include <string>
#include <typeindex>
namespace h5pp {
    class TypeInfo {
        private:
        const std::string          name_;
        const size_t               size_;
        const std::type_index      type_;
        const int                  ndims_;
        const std::vector<hsize_t> dims_;
        const hid::h5t             h5type_;

        public:
        TypeInfo(std::string_view name, size_t size, std::type_index type, int ndims, std::vector<hsize_t> dims, const hid::h5t &h5type)
            : name_(name), size_(size), type_(type), ndims_(ndims), dims_(std::move(dims)), h5type_(h5type) {
            if(ndims_ != (int) dims_.size())
                throw std::runtime_error("Dimension mismatch, ndims (" + std::to_string(ndims_) + ") != dims.size(" + std::to_string(dims_.size()) + ")");
            size_t size_check = std::accumulate(std::begin(dims_), std::end(dims_), 1.0, std::multiplies<>());
            if(size_check != size) throw std::runtime_error("Size mismatch, size (" + std::to_string(size_) + ") != product of dims (" + std::to_string(size_check) + ")");
        }

        [[nodiscard]] const std::string &         name() const { return name_; }
        [[nodiscard]] const size_t &              size() const { return size_; }
        [[nodiscard]] const std::type_index &     type() const { return type_; }
        [[nodiscard]] const int &                 ndims() const { return ndims_; }
        [[nodiscard]] const std::vector<hsize_t> &dims() const { return dims_; }
        [[nodiscard]] const hid::h5t &            h5type() const { return h5type_; }
    };
}
