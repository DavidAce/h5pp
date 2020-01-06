#pragma once
#include "h5ppType.h"
#include "h5ppTypeCheck.h"
#include <numeric>
#include <spdlog/spdlog.h>

namespace h5pp::Utils {
    template<typename DataType, size_t size> constexpr size_t getArraySize([[maybe_unused]] const DataType (&arr)[size]) {
        if constexpr(std::is_same<char, typename std::decay_t<DataType>>::value)
            return strlen(arr);
        else
            return size;
    }

    inline hsize_t setStringSize(hid_t datatype, hsize_t size) {
        size          = std::max((hsize_t) 1, size);
        herr_t retval = H5Tset_size(datatype, size);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to set size: " + std::to_string(size));
        }
        //            retval  = H5Tset_strpad(datatype,H5T_STR_NULLTERM);
        //            retval  = H5Tset_strpad(datatype,H5T_STR_NULLPAD);
        if(retval < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to set strpad");
        }
        return size;
    }

    template<typename DataType> hsize_t getSize(const DataType &data) {
        namespace tc = h5pp::Type::Check;
        if constexpr(tc::hasMember_size<DataType>::value) return data.size(); // Fails on clang?
        if constexpr(std::is_array<DataType>::value) return getArraySize(data);
        if constexpr(std::is_arithmetic<DataType>::value) return 1;
        if constexpr(std::is_pod<DataType>::value) return 1;
        if constexpr(tc::is_StdComplex<DataType>()) return 1;
        if constexpr(tc::is_ScalarN<DataType>()) return 1;
        spdlog::warn("WARNING: getSize could not get the size of the type provided: " + std::string(typeid(data).name()));
        return 0;
    }

    template<typename DataType> constexpr int getRank() {
        namespace tc = h5pp::Type::Check;
        if constexpr(tc::is_eigen_1d<DataType>::value) return 1;
        if constexpr(tc::is_eigen_tensor<DataType>::value) return (int) DataType::NumIndices;
        if constexpr(tc::is_eigen_core<DataType>::value) return 2;
        if constexpr(std::is_arithmetic<DataType>::value) return 1;
        if constexpr(tc::is_std_vector<DataType>::value) return 1;
        if constexpr(tc::is_std_array<DataType>::value) return 1;
        if constexpr(tc::is_ScalarN<DataType>()) return 1;
        if constexpr(std::is_same<std::string, DataType>::value) return 1;
        if constexpr(std::is_same<const char *, DataType>::value) return 1;
        if constexpr(std::is_same<char, typename std::decay_t<DataType>>::value) return 1;
        if constexpr(std::is_array<DataType>::value) return 1;
        if constexpr(std::is_pod<DataType>::value) return 1;
        if constexpr(tc::is_StdComplex<DataType>()) return 1;
        return 1;
        //        else
        //            tc::print_type_and_exit_compile_time<DataType>();
    }

    template<typename T> size_t getSizeOf() {
        namespace tc = h5pp::Type::Check;
        // If userDataType is a container we should check that elements in the container have matching size as the hdf5DataType
        if constexpr(tc::is_StdComplex<T>()) return sizeof(T);
        if constexpr(tc::is_ScalarN<T>()) return sizeof(T);
        if constexpr(std::is_arithmetic<T>::value) return sizeof(T);
        if constexpr(tc::is_eigen_type<T>::value) return sizeof(typename T::Scalar);
        if constexpr(tc::is_std_vector<T>::value) return sizeof(typename T::value_type);
        if constexpr(tc::is_std_array<T>::value) return sizeof(typename T::value_type);
        if constexpr(std::is_array<T>::value) return sizeof(std::remove_all_extents_t<T>);
        if constexpr(std::is_same<T, std::string>::value) return sizeof(char);
        return sizeof(std::remove_all_extents_t<T>);
    }

    template<typename userDataType> bool typeSizesMatch([[maybe_unused]] hid_t hdf5Datatype) {
        size_t hdf5DataTypeSize;
        size_t userDataTypeSize;
        if constexpr(std::is_same<userDataType, std::string>::value or std::is_same<userDataType, char[]>::value) {
            hdf5DataTypeSize = sizeof(char);
            userDataTypeSize = getSizeOf<userDataType>();
        } else {
            hdf5DataTypeSize = H5Tget_size(hdf5Datatype);
            userDataTypeSize = getSizeOf<userDataType>();
        }

        if(userDataTypeSize != hdf5DataTypeSize) {
            h5pp::Logger::log->error("Type size mismatch: given {} bytes | inferred {} bytes", userDataTypeSize, hdf5DataTypeSize);
            return false;
        } else {
            return true;
        }
    }

    template<typename DataType> std::vector<hsize_t> getDimensions(const DataType &data) {
        namespace tc              = h5pp::Type::Check;
        constexpr int        rank = getRank<DataType>();
        std::vector<hsize_t> dims(rank);
        if constexpr(tc::is_eigen_tensor<DataType>::value) {
            std::copy(data.dimensions().begin(), data.dimensions().end(), dims.begin());
            return dims;
        } else if constexpr(tc::is_eigen_core<DataType>::value) {
            dims[0] = (hsize_t) data.rows();
            dims[1] = (hsize_t) data.cols();
            return dims;
        } else if constexpr(tc::is_std_vector<DataType>::value) {
            dims[0] = {data.size()};
            return dims;
        } else if constexpr(tc::is_std_array<DataType>::value) {
            dims[0] = data.size();
            return dims;
        } else if constexpr(std::is_same<std::string, DataType>::value or std::is_same<char *, typename std::decay<DataType>::type>::value) {
            // Read more about this step here
            // http://www.astro.sunysb.edu/mzingale/io_tutorial/HDF5_simple/hdf5_simple.c
            dims[0] = 1;
            return dims;
        } else if constexpr(std::is_array<DataType>::value) {
            dims[0] = getArraySize(data);
            return dims;
        } else if constexpr(tc::hasMember_size<DataType>::value and rank == 1) {
            dims[0] = 1;
            return dims;
        } else if constexpr(std::is_arithmetic<DataType>::value or tc::is_StdComplex<DataType>()) {
            dims[0] = 1;
            return dims;
        }

        else {
            tc::print_type_and_exit_compile_time<DataType>();
            std::string error = "getDimensions can't match the type provided: " + std::string(typeid(DataType).name());
            spdlog::critical(error);
            throw(std::logic_error(error));
        }
    }

    template<typename DataType> std::vector<hsize_t> getChunkDimensions(const DataType &data) {
        // Chunk dimensions are always the same rank as the dataset dimensions.
        // The simplest way to get a reasonable chunk dimension is to pick a slice, by setting one of
        // the dimensions to 1.

        // HDF5 is row-major, meaning that on a rank 3 tensor with indices i,j,k, we get contiguous slices
        // when i is fixed (i.e. chunk dims: 1 x size(j) x size(k)). However since the i,j, and k dim
        // sizes may be significantly different, we may have to reduce the chunk dimension even more.

        // Here is a simple heuristic to find a chunk size:
        // For a square matrix with N elements (N = n x n) reasonable chunk dimensions could be (1 x n),
        // so that each row is a single chunk, and where n = sqrt(N) is the size of a single slice.
        // When generalizing this to other dimensions, we can use sqrt(N) as an estimator of the slice size.

        // For example, a rank 3 tensor with dimensions (100 x 20 x 5) has size N = 10000,
        // with slice size = sqrt(N) = 100. To get reasonable chunk dimensions we reduce dataset
        // dimensions by one starting from the left and stop right before becomming smaller than sqrt(N)
        // In this case we get (1 x 20 x 5)

        // A less trivial example is a tensor with dimensions (20 x 15 x 3 x 8), with N = 7200 and
        // slice size sqrt(N) = 84.8528... The same procedure as before gives us chunk
        // dimensions (1 x 4 x 3 x 8). Note that 1 x 4 x 3 x 8 = 96 which is just slightly bigger
        // than 84.8528...

        size_t               size       = getSize<DataType>(data);
        std::vector<hsize_t> dims       = getDimensions<DataType>(data);
        size_t               slice_size = std::ceil(std::sqrt(size));
        std::vector<hsize_t> chunkDims  = dims;
        for(size_t dim = 0; dim < dims.size(); dim++) {
            while(chunkDims[dim] > 1) {
                size_t chunkSize = std::accumulate(chunkDims.begin(), chunkDims.end(), 1, std::multiplies<>());
                if(chunkSize < slice_size)
                    break;
                else
                    chunkDims[dim]--;
            }
        }
        return chunkDims;
    }

    inline hid_t getDataSpace(const int rank, const std::vector<hsize_t> &dims, const bool unlimited = false) {
        std::vector<hsize_t> max_dims(rank);
        if(unlimited) {
            std::fill_n(max_dims.begin(), rank, H5S_UNLIMITED);
            return H5Screate_simple(rank, dims.data(), max_dims.data());
        } else {
            if(rank == 0)
                return H5Screate(H5S_SCALAR);
            else {
                max_dims = dims;
                return H5Screate_simple(rank, dims.data(), max_dims.data());
            }
        }
    }

    inline hid_t getMemSpace(const int rank, const std::vector<hsize_t> &dims) {
        if(rank == 0)
            return H5Screate(H5S_SCALAR);
        else
            return H5Screate_simple(rank, dims.data(), nullptr);
    }

    template<typename DataType> auto getByteSize(const DataType &data) {
        hsize_t num_elems = getSize(data);
        hsize_t typesize  = sizeof(data);
        if constexpr(h5pp::Type::Check::hasMember_data<DataType>::value) typesize = sizeof(data.data()[0]);
        if constexpr(h5pp::Type::Check::hasMember_c_str<DataType>::value) typesize = sizeof(data.c_str()[0]);
        if constexpr(h5pp::Type::Check::hasMember_Scalar<DataType>::value) typesize = sizeof(typename DataType::Scalar);
        if constexpr(std::is_array<DataType>::value) typesize = sizeof(data[0]);
        return num_elems * typesize;
    }

}
