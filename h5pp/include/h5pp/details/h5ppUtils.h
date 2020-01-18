#pragma once
#include "h5ppTypeScan.h"
#include <cstring>
#include <numeric>

namespace h5pp::Utils {

    template<typename DataType>
    [[nodiscard]] Hid::h5t getH5DataType() {
        return h5pp::Type::Scan::getH5DataType<DataType>();
    }

    template<typename DataType, size_t size>
    constexpr size_t getArraySize([[maybe_unused]] const DataType (&arr)[size]) {
        if constexpr(h5pp::Type::Scan::is_text<DataType>())
            return strlen(arr);
        else
            return size;
    }

    template<typename DataType>
    hsize_t getSize(const DataType &data) {
        namespace tsc = h5pp::Type::Scan;
        if constexpr(tsc::hasMember_size<DataType>::value) return data.size();
        if constexpr(std::is_array<DataType>::value) return getArraySize(data);

        // Add more checks here. As it is, these two checks above handle all cases I have encountered.
        return 1; // All others should be "atomic" of size 1.
    }

    template<typename DataType>
    constexpr int getRank() {
        namespace tsc = h5pp::Type::Scan;
        if constexpr(tsc::hasMember_NumIndices<DataType>::value) return (int) DataType::NumIndices;
#ifdef H5PP_EIGEN3
        if constexpr(tsc::is_eigen_tensor<DataType>::value) return (int) DataType::NumIndices;
        if constexpr(tsc::is_eigen_1d<DataType>::value) return 1;
        if constexpr(tsc::is_eigen_dense<DataType>::value) return 2;
#endif
        return 1;
    }

    template<typename DataType>
    std::vector<hsize_t> getDimensions(const DataType &data) {
        namespace tsc       = h5pp::Type::Scan;
        constexpr int ndims = getRank<DataType>();
        if constexpr(tsc::hasMember_dimensions<DataType>::value) {
            std::vector<hsize_t> dims(data.dimensions().begin(),
                                      data.dimensions().end()); // We copy because the vectors may not be assignable or may not be implicitly convertible to hsize_t.
            assert(data.dimensions().size() == ndims and "given dimensions do not match detected rank");
            assert(dims.size() == ndims and "copied dimensions do not match detected rank");
            return dims;
        } else if constexpr(tsc::is_text<DataType>()) {
            // Read more about this step here
            // http://www.astro.sunysb.edu/mzingale/io_tutorial/HDF5_simple/hdf5_simple.c
            return {(hsize_t) 1};
        } else if constexpr(tsc::hasMember_size<DataType>::value and ndims == 1) {
            return {(hsize_t) data.size()};
        }
#ifdef H5PP_EIGEN3
        else if constexpr(tsc::is_eigen_tensor<DataType>::value) {
            assert(data.dimensions().size() == ndims and "given dimensions do not match detected rank");
            std::vector<hsize_t> dims(data.dimensions().begin(),
                                      data.dimensions().end()); // We copy because the vectors may not be assignable or may not be implicitly convertible to hsize_t.
            return dims;
        } else if constexpr(tsc::is_eigen_dense<DataType>::value) {
            std::vector<hsize_t> dims(ndims);
            dims[0] = (hsize_t) data.rows();
            dims[1] = (hsize_t) data.cols();
            return dims;
        }
#endif
        else if constexpr(std::is_array<DataType>::value) {
            return {getArraySize(data)};
        } else if constexpr(std::is_arithmetic<DataType>::value or tsc::is_StdComplex<DataType>()) {
            return {1};
        } else {
            tsc::print_type_and_exit_compile_time<DataType>();
            std::string error = "getDimensions can't match the type provided: " + h5pp::Type::Scan::type_name<DataType>();
            h5pp::Logger::log->critical(error);
            throw std::logic_error(error);
        }
    }

    inline std::vector<hsize_t>
        getDefaultChunkDimensions(const size_t size, const std::vector<hsize_t> &dims, const std::optional<std::vector<hsize_t>> &desiredChunkDims = std::nullopt) {
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
        if(desiredChunkDims.has_value()) {
            // Check that the desired rank matches dims
            if(dims.size() != desiredChunkDims.value().size()) throw std::runtime_error("Mismatch in rank: desired chunk dimensions and data dimensions");
            return desiredChunkDims.value();
        }
        auto                 slice_size = static_cast<size_t>(std::ceil(std::sqrt(size)));
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

    [[nodiscard]] inline Hid::h5s getDataSpace(const hsize_t size, const int ndims, const std::vector<hsize_t> &dims, const H5D_layout_t layout) {
        assert((size_t) ndims == dims.size() and "Dimension mismatch");
        if(size == 0) return H5Screate(H5S_NULL);
        if(layout == H5D_CHUNKED and ndims > 0) {
            // Chunked layout datasets can be extended, which is why their max extent in any dimension is unlimited.
            std::vector<hsize_t> maxDims(ndims);
            std::fill_n(maxDims.begin(), ndims, H5S_UNLIMITED);
            return H5Screate_simple(ndims, dims.data(), maxDims.data());
        } else {
            if(ndims == 0)
                return H5Screate(H5S_SCALAR);
            else {
                return H5Screate_simple(ndims, dims.data(), nullptr); // nullptr ->  maxDims same as dims
            }
        }
    }

    [[nodiscard]] inline Hid::h5s getMemSpace(const hsize_t size, const int ndims, const std::vector<hsize_t> &dims) {
        assert((size_t) ndims == dims.size() and "Dimension mismatch");
        if(size == 0) return H5Screate(H5S_NULL);
        if(ndims == 0)
            return H5Screate(H5S_SCALAR);
        else
            return H5Screate_simple(ndims, dims.data(), nullptr);
    }

    template<typename h5x>
    std::string getName(const h5x &object) {
        static_assert(std::is_convertible_v<h5x, hid_t>);
        std::string buf;
        ssize_t     namesize = H5Iget_name(object, nullptr, 0);
        if(namesize > 0) {
            buf.resize(namesize + 1);
            H5Iget_name(object, buf.data(), namesize + 1);
        }
        return buf;
    }

    template<typename DataType>
    size_t getBytesPerElem() {
        namespace tsc   = h5pp::Type::Scan;
        using DecayType = typename std::decay<DataType>::type;
        if constexpr(std::is_pointer<DecayType>::value) return getBytesPerElem<typename std::remove_pointer<DecayType>::type>();
        if constexpr(std::is_reference<DecayType>::value) return getBytesPerElem<typename std::remove_reference<DecayType>::type>();
        if constexpr(std::is_array<DecayType>::value) return getBytesPerElem<typename std::remove_all_extents<DecayType>::type>();
        if constexpr(tsc::is_StdComplex<DecayType>()) return sizeof(DecayType);
        if constexpr(tsc::is_ScalarN<DecayType>()) return sizeof(DecayType);
        if constexpr(std::is_arithmetic<DecayType>::value) return sizeof(DecayType);
        if constexpr(tsc::hasMember_Scalar<DecayType>::value) return sizeof(typename DecayType::Scalar);
        if constexpr(tsc::hasMember_value_type<DecayType>::value) return sizeof(typename DecayType::value_type);
        return sizeof(std::remove_all_extents_t<DecayType>);
    }

    size_t getBytesPerElem(const Hid::h5t &type) {
        // This works as H5Tget_size but takes care of getting the correct size for char data
        H5T_class_t h5class = H5Tget_class(type);
        if(h5class == H5T_STRING) {
            // We create text-data differently, as scalar (atomic) type of size 1. The result of H5Gget_size will then be the char*size. Therefore
            // we measure the type of a single H5T_C_S1 directly.
            return H5Tget_size(H5T_C_S1);
        } else {
            return H5Tget_size(type);
        }
    }

    template<typename DataType>
    size_t getBytesTotal(const DataType &data) {
        return getBytesPerElem<DataType>() * getSize(data);
    }

    size_t getBytesTotal(const Hid::h5s &space, const Hid::h5t &type) { return H5Sget_simple_extent_npoints(space) * H5Tget_size(type); }

    size_t getBytesTotal(const Hid::h5d &dataset) {
        Hid::h5s space = H5Dget_space(dataset);
        Hid::h5t type  = H5Dget_type(dataset);
        return getBytesTotal(space, type);
    }

    template<typename userDataType>
    bool checkBytesPerElemMatch(const Hid::h5t &hdf5Datatype) {
        size_t dsetTypeSize = getBytesPerElem(hdf5Datatype);
        size_t dataTypeSize = getBytesPerElem<userDataType>();
        if(dataTypeSize != dsetTypeSize) { h5pp::Logger::log->debug("Type size mismatch: dataset type {} bytes | given type {} bytes", dsetTypeSize, dataTypeSize); }
        return dataTypeSize == dsetTypeSize;
    }

    template<typename userDataType>
    void assertBytesPerElemMatch(const Hid::h5t &hdf5Datatype) {
        size_t dsetTypeSize = getBytesPerElem(hdf5Datatype);
        size_t dataTypeSize = getBytesPerElem<userDataType>();
        if(dataTypeSize != dsetTypeSize) {
            throw std::runtime_error("Type size mismatch: dataset type " + std::to_string(dsetTypeSize) + " bytes | given type " + std::to_string(dataTypeSize) + " bytes");
        }
    }

    template<typename DataType>
    bool checkBytesMatchTotal(const DataType &data, const Hid::h5d &dataset) {
        size_t dsetsize = getBytesTotal(dataset);
        size_t datasize = getBytesTotal(data);
        if(datasize != dsetsize) { h5pp::Logger::log->error("Storage size mismatch: hdf5 {} bytes | given {} bytes", dsetsize, datasize); }
        return datasize == dsetsize;
    }

    template<typename DataType>
    void assertBytesMatchTotal(const DataType &data, const Hid::h5d &dataset) {
        size_t dsetsize = getBytesTotal(dataset);
        size_t datasize = getBytesTotal(data);
        if(datasize != dsetsize) {
            throw std::runtime_error("Storage size mismatch: dataset " + std::to_string(dsetsize) + " bytes | given " + std::to_string(datasize) + " bytes");
        }
    }

    template<typename DataType>
    inline hsize_t setStringSize(const DataType &data, const Hid::h5t &datatype) {
        hsize_t size = h5pp::Utils::getSize(data);
        if(h5pp::Type::Scan::is_text<DataType>()) {
            htri_t equaltypes = H5Tequal(datatype, H5T_C_S1);
            if(equaltypes < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Type equality comparison failed");
            }
            if(equaltypes > 0) {
                size          = std::max((hsize_t) 1, size);
                herr_t retval = H5Tset_size(datatype.value(), size);
                if(retval < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to set size: " + std::to_string(size));
                }
                // The following makes sure there is a single "\0" at the end of the string when written to file.
                // Note however that size here is supposed to be the number of characters NOT including null terminator.
                retval = H5Tset_strpad(datatype, H5T_STR_NULLTERM);
                if(retval < 0) {
                    H5Eprint(H5E_DEFAULT, stderr);
                    throw std::runtime_error("Failed to set strpad");
                }
            }
        }
        return size;
    }

    bool checkEqualTypesRecursive(const Hid::h5t &type1, const Hid::h5t &type2) {
        // If types are compound, check recursively that all members have equal types and names
        H5T_class_t dataClass1 = H5Tget_class(type1);
        H5T_class_t dataClass2 = H5Tget_class(type1);
        if(dataClass1 == H5T_COMPOUND and dataClass2 == H5T_COMPOUND) {
            int num_members1 = H5Tget_nmembers(type1);
            int num_members2 = H5Tget_nmembers(type2);
            if(num_members1 != num_members2) return false;
            for(int idx = 0; idx < num_members1; idx++) {
                Hid::h5t         t1    = H5Tget_member_type(type1, idx);
                Hid::h5t         t2    = H5Tget_member_type(type2, idx);
                char *           mem1  = H5Tget_member_name(type1, idx);
                char *           mem2  = H5Tget_member_name(type2, idx);
                std::string_view n1    = mem1;
                std::string_view n2    = mem2;
                bool             equal = n1 == n2;
                H5free_memory(mem1);
                H5free_memory(mem2);
                if(not equal) return false;
                if(not checkEqualTypesRecursive(t1, t2)) return false;
            }
            return true;
        } else if(dataClass1 == dataClass2) {
            if(dataClass1 == H5T_STRING)
                return true;
            else
                return type1 == type2;
        } else {
            return false;
        }
    }

    inline H5D_layout_t decideLayout(const size_t bytes, std::optional<H5D_layout_t> desiredLayout = std::nullopt) {
        /*! Depending on the size of this dataset we may benefint from using either
            a contiguous layout (for big non-extendable non-compressible datasets),
            a chunked layout (for extendable and compressible datasets)
            or a compact layout (for tiny datasets).

            Contiguous
            For big non-extendable non-compressible datasets

            Chunked
            Chunking is required for enabling compression and other filters, as well as for
            creating extendible or unlimited dimension datasets. Note that a chunk always has
            the same rank as the dataset and the chunk's dimensions do not need to be factors
            of the dataset dimensions.

            Compact
            A compact dataset is one in which the raw data is stored in the object header of the dataset.
            This layout is for very small datasets that can easily fit in the object header.
            The compact layout can improve storage and access performance for files that have many very
            tiny datasets. With one I/O access both the header and data values can be read.
            The compact layout reduces the size of a file, as the data is stored with the header which
            will always be allocated for a dataset. However, the object header is 64 KB in size,
            so this layout can only be used for very small datasets.
         */

        // First, we check if the user explicitly asked for an extendable dataset.
        if(desiredLayout) return desiredLayout.value();
        // Otherwise we decide based on size
        if(bytes < h5pp::Constants::maxSizeCompact)
            return H5D_COMPACT;
        else if(bytes < h5pp::Constants::maxSizeContiguous)
            return H5D_CONTIGUOUS;
        else
            return H5D_CHUNKED;
    }

}
