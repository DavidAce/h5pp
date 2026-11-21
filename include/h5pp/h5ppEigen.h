#pragma once
#include "h5ppExcept.h"
#include "h5ppTypeTraits.h"
#ifdef H5PP_USE_EIGEN3
    #include <Eigen/Core>
    #include <unsupported/Eigen/CXX11/Tensor>
#endif

#include <iterator>
#include <numeric>
#if !defined(_MSC_VER)
    #define H5PP_CONSTEXPR constexpr
#else
    #define H5PP_CONSTEXPR
#endif

namespace h5pp {
#ifdef H5PP_USE_EIGEN3
    namespace eigen {
        template<auto rank>
        using array = Eigen::array<Eigen::Index, rank>;

        // Handy functions to copy lists of dimensions
        template<auto rank, typename Container>
        [[nodiscard]] H5PP_CONSTEXPR Eigen::DSizes<Eigen::Index, rank> copy_dims(const Container &container) {
            Eigen::DSizes<Eigen::Index, rank> dsizes;
            if constexpr(std::is_array_v<Container>) {
                std::copy_n(std::begin(container), rank, dsizes.begin());
            } else {
                if(container.size() != rank) throw h5pp::runtime_error("copy_dims: Wrong container size, can't copy dimensions.");
                std::copy(std::begin(container), std::end(container), dsizes.begin());
            }
            return dsizes;
        }

        template<typename Derived>
        using is_matrixObject = std::is_base_of<Eigen::MatrixBase<std::decay_t<Derived>>, std::decay_t<Derived>>;
        template<typename Derived>
        using is_arrayObject = std::is_base_of<Eigen::ArrayBase<std::decay_t<Derived>>, std::decay_t<Derived>>;

        //************************//
        // change storage layout //
        //************************//
        template<typename Derived>
        auto to_RowMajor(const Eigen::TensorBase<Derived, Eigen::ReadOnlyAccessors> &tensor) {
            if constexpr(Eigen::RowMajor == static_cast<Eigen::StorageOptions>(Derived::Layout)) {
                return tensor;
            } else {
                array<Derived::NumIndices> neworder;
                std::iota(std::begin(neworder), std::end(neworder), 0);
                std::reverse(neworder.data(), neworder.data() + neworder.size());
                return Eigen::Tensor<typename Derived::Scalar, Derived::NumIndices, Eigen::RowMajor>(
                    tensor.swap_layout().shuffle(neworder));
            }
        }

        template<typename Derived>
        auto to_ColMajor(const Eigen::TensorBase<Derived, Eigen::ReadOnlyAccessors> &tensor) {
            if constexpr(Eigen::ColMajor == static_cast<Eigen::StorageOptions>(Derived::Layout)) {
                return tensor;
            } else {
                array<Derived::NumIndices> neworder;
                std::iota(std::begin(neworder), std::end(neworder), 0);
                std::reverse(neworder.data(), neworder.data() + neworder.size());
                return Eigen::Tensor<typename Derived::Scalar, Derived::NumIndices, Eigen::ColMajor>(
                    tensor.swap_layout().shuffle(neworder));
            }
        }

        template<typename Derived>
        auto to_RowMajor(const Eigen::DenseBase<Derived> &dense) {
            if constexpr(Derived::IsRowMajor) return dense;
            if constexpr(is_matrixObject<Derived>::value) {
                return Eigen::Matrix<typename Derived::Scalar, Derived::RowsAtCompileTime, Derived::ColsAtCompileTime, Eigen::RowMajor>(
                    dense);
            } else if constexpr(is_arrayObject<Derived>::value) {
                return Eigen::Array<typename Derived::Scalar, Derived::RowsAtCompileTime, Derived::ColsAtCompileTime, Eigen::RowMajor>(
                    dense);
            }
            throw h5pp::runtime_error("Wrong dense type?? Report this bug!");
        }

        template<typename Derived>
        auto to_ColMajor(const Eigen::DenseBase<Derived> &dense) {
            if constexpr(not Derived::IsRowMajor) return dense;
            if constexpr(is_matrixObject<Derived>::value) {
                return Eigen::Matrix<typename Derived::Scalar, Derived::RowsAtCompileTime, Derived::ColsAtCompileTime, Eigen::ColMajor>(
                    dense);
            } else if constexpr(is_arrayObject<Derived>::value) {
                return Eigen::Array<typename Derived::Scalar, Derived::RowsAtCompileTime, Derived::ColsAtCompileTime, Eigen::ColMajor>(
                    dense);
            }
            throw h5pp::runtime_error("Wrong dense type?? Report this bug!");
        }
    }

    namespace traits {

        template<typename T>
        using is_eigen_matrix = std::is_base_of<Eigen::MatrixBase<std::decay_t<T>>, std::decay_t<T>>;
        template<typename T>
        inline constexpr bool is_eigen_matrix_v = is_eigen_matrix<T>::value;
        template<typename T>
        using is_eigen_array = std::is_base_of<Eigen::ArrayBase<std::decay_t<T>>, std::decay_t<T>>;
        template<typename T>
        inline constexpr bool is_eigen_array_v = is_eigen_array<T>::value;
        template<typename T>
        using is_eigen_tensor = std::is_base_of<Eigen::TensorBase<std::decay_t<T>, Eigen::ReadOnlyAccessors>, std::decay_t<T>>;
        template<typename T>
        inline constexpr bool is_eigen_tensor_v = is_eigen_tensor<T>::value;
        template<typename T>
        using is_eigen_dense = std::is_base_of<Eigen::DenseBase<std::decay_t<T>>, std::decay_t<T>>;
        template<typename T>
        inline constexpr bool is_eigen_dense_v = is_eigen_dense<T>::value;
        template<typename T>
        using is_eigen_map = std::is_base_of<Eigen::MapBase<std::decay_t<T>, Eigen::ReadOnlyAccessors>, std::decay_t<T>>;
        template<typename T>
        inline constexpr bool is_eigen_map_v = is_eigen_map<T>::value;
        template<typename T>
        using is_eigen_plain = std::is_base_of<Eigen::PlainObjectBase<std::decay_t<T>>, std::decay_t<T>>;
        template<typename T>
        inline constexpr bool is_eigen_plain_v = is_eigen_plain<T>::value;
        template<typename T>
        using is_eigen_base = std::is_base_of<Eigen::EigenBase<std::decay_t<T>>, std::decay_t<T>>;
        template<typename T>
        inline constexpr bool is_eigen_base_v = is_eigen_base<T>::value;
        template<typename T>
        struct is_eigen_core : public std::false_type {};
        template<typename T, int rows, int cols, int StorageOrder>
        struct is_eigen_core<Eigen::Matrix<T, rows, cols, StorageOrder>> : public std::true_type {};
        template<typename T, int rows, int cols, int StorageOrder>
        struct is_eigen_core<Eigen::Array<T, rows, cols, StorageOrder>> : public std::true_type {};
        template<typename T>
        inline constexpr bool is_eigen_core_v = is_eigen_core<T>::value;
        template<typename T>
        struct is_eigen_any {
            static constexpr bool value = is_eigen_base<T>::value or is_eigen_tensor<T>::value;
        };
        template<typename T>
        inline constexpr bool is_eigen_any_v = is_eigen_any<T>::value;
        template<typename T>
        struct is_eigen_contiguous {
            static constexpr bool value = is_eigen_any<T>::value and has_data_v<T>;
        };
        template<typename T>
        inline constexpr bool is_eigen_contiguous_v = is_eigen_contiguous<T>::value;

        template<typename T>
        class is_eigen_1d {
            private:
            template<typename U>
            static constexpr auto test() {
                if constexpr(is_eigen_map<U>::value) return test<typename U::PlainObject>();
                else if constexpr(is_eigen_dense<U>::value) return U::RowsAtCompileTime == 1 or U::ColsAtCompileTime == 1;
                else if constexpr(is_eigen_tensor<U>::value and has_NumIndices_v<U>) return U::NumIndices == 1;
                else return false;
            }

            public:
            static constexpr bool value = test<T>();
        };
        template<typename T>
        inline constexpr bool is_eigen_1d_v = is_eigen_1d<T>::value;

        template<typename T>
        class is_eigen_colmajor {
            template<typename U>
            static constexpr bool test() {
                if constexpr(is_eigen_base<U>::value) return not U::IsRowMajor;
                else if constexpr(is_eigen_tensor<U>::value) return Eigen::ColMajor == static_cast<Eigen::StorageOptions>(U::Layout);
                else return false;
            }

            public:
            static constexpr bool value = test<T>();
        };
        template<typename T>
        inline constexpr bool is_eigen_colmajor_v = is_eigen_colmajor<T>::value;

        template<typename T>
        class is_eigen_rowmajor {
            template<typename U>
            static constexpr bool test() {
                if constexpr(is_eigen_base<U>::value) return U::IsRowMajor;
                if constexpr(is_eigen_tensor<U>::value) return Eigen::RowMajor == static_cast<Eigen::StorageOptions>(U::Layout);
                else return false;
            }

            public:
            static constexpr bool value = test<T>();
        };
        template<typename T>
        inline constexpr bool is_eigen_rowmajor_v = is_eigen_rowmajor<T>::value;
    }

#endif

}
