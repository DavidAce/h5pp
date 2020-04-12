#pragma once
#include "h5ppOptional.h"
#include "h5ppTypeSfinae.h"
#include <cstring>
#include <numeric>

/*!
 * \brief A collection of functions to get information about C++ types passed by the user
 */
namespace h5pp::util {

    template<typename DataType>
    [[nodiscard]] hid::h5t getH5Type(const std::optional<hid::h5t> &h5type = std::nullopt) {
        if(h5type.has_value()) return h5type.value(); // Intercept
        namespace tc = h5pp::type::sfinae;
        /* clang-format off */
        using DecayType    = typename std::decay<DataType>::type;
        if constexpr (std::is_pointer_v<DecayType>)                                return getH5Type<typename std::remove_pointer<DecayType>::type>();
        if constexpr (std::is_reference_v<DecayType>)                              return getH5Type<typename std::remove_reference<DecayType>::type>();
        if constexpr (std::is_array_v<DecayType>)                                  return getH5Type<typename std::remove_all_extents<DecayType>::type>();
        if constexpr (tc::is_std_vector_v<DecayType>)                              return getH5Type<typename DecayType::value_type>();
        if constexpr (std::is_same_v<DecayType, short>)                            return H5Tcopy(H5T_NATIVE_SHORT);
        if constexpr (std::is_same_v<DecayType, int>)                              return H5Tcopy(H5T_NATIVE_INT);
        if constexpr (std::is_same_v<DecayType, long>)                             return H5Tcopy(H5T_NATIVE_LONG);
        if constexpr (std::is_same_v<DecayType, long long>)                        return H5Tcopy(H5T_NATIVE_LLONG);
        if constexpr (std::is_same_v<DecayType, unsigned short>)                   return H5Tcopy(H5T_NATIVE_USHORT);
        if constexpr (std::is_same_v<DecayType, unsigned int>)                     return H5Tcopy(H5T_NATIVE_UINT);
        if constexpr (std::is_same_v<DecayType, unsigned long>)                    return H5Tcopy(H5T_NATIVE_ULONG);
        if constexpr (std::is_same_v<DecayType, unsigned long long >)              return H5Tcopy(H5T_NATIVE_ULLONG);
        if constexpr (std::is_same_v<DecayType, double>)                           return H5Tcopy(H5T_NATIVE_DOUBLE);
        if constexpr (std::is_same_v<DecayType, long double>)                      return H5Tcopy(H5T_NATIVE_LDOUBLE);
        if constexpr (std::is_same_v<DecayType, float>)                            return H5Tcopy(H5T_NATIVE_FLOAT);
        if constexpr (std::is_same_v<DecayType, bool>)                             return H5Tcopy(H5T_NATIVE_HBOOL);
//        if constexpr (std::is_same_v<DecayType, char>)                             return H5Tcopy(H5T_NATIVE_CHAR);
        if constexpr (std::is_same_v<DecayType, std::string>)                      return H5Tcopy(H5T_C_S1);
        if constexpr (std::is_same_v<DecayType, char>)                             return H5Tcopy(H5T_C_S1);
        if constexpr (std::is_same_v<DecayType, std::complex<short>>)              return H5Tcopy(h5pp::type::compound::H5T_COMPLEX_SHORT);
        if constexpr (std::is_same_v<DecayType, std::complex<int>>)                return H5Tcopy(h5pp::type::compound::H5T_COMPLEX_INT);
        if constexpr (std::is_same_v<DecayType, std::complex<long>>)               return H5Tcopy(h5pp::type::compound::H5T_COMPLEX_LONG);
        if constexpr (std::is_same_v<DecayType, std::complex<long long>>)          return H5Tcopy(h5pp::type::compound::H5T_COMPLEX_LLONG);
        if constexpr (std::is_same_v<DecayType, std::complex<unsigned short>>)     return H5Tcopy(h5pp::type::compound::H5T_COMPLEX_USHORT);
        if constexpr (std::is_same_v<DecayType, std::complex<unsigned int>>)       return H5Tcopy(h5pp::type::compound::H5T_COMPLEX_UINT);
        if constexpr (std::is_same_v<DecayType, std::complex<unsigned long>>)      return H5Tcopy(h5pp::type::compound::H5T_COMPLEX_ULONG);
        if constexpr (std::is_same_v<DecayType, std::complex<unsigned long long>>) return H5Tcopy(h5pp::type::compound::H5T_COMPLEX_ULLONG);
        if constexpr (std::is_same_v<DecayType, std::complex<double>>)             return H5Tcopy(h5pp::type::compound::H5T_COMPLEX_DOUBLE);
        if constexpr (std::is_same_v<DecayType, std::complex<long double>>)        return H5Tcopy(h5pp::type::compound::H5T_COMPLEX_LDOUBLE);
        if constexpr (std::is_same_v<DecayType, std::complex<float>>)              return H5Tcopy(h5pp::type::compound::H5T_COMPLEX_FLOAT);
        if constexpr (tc::is_Scalar2_of_type<DecayType, short>())                  return H5Tcopy(h5pp::type::compound::H5T_SCALAR2_SHORT);
        if constexpr (tc::is_Scalar2_of_type<DecayType, int>())                    return H5Tcopy(h5pp::type::compound::H5T_SCALAR2_INT);
        if constexpr (tc::is_Scalar2_of_type<DecayType, long>())                   return H5Tcopy(h5pp::type::compound::H5T_SCALAR2_LONG);
        if constexpr (tc::is_Scalar2_of_type<DecayType, long long>())              return H5Tcopy(h5pp::type::compound::H5T_SCALAR2_LLONG);
        if constexpr (tc::is_Scalar2_of_type<DecayType, unsigned short>())         return H5Tcopy(h5pp::type::compound::H5T_SCALAR2_USHORT);
        if constexpr (tc::is_Scalar2_of_type<DecayType, unsigned int>())           return H5Tcopy(h5pp::type::compound::H5T_SCALAR2_UINT);
        if constexpr (tc::is_Scalar2_of_type<DecayType, unsigned long>())          return H5Tcopy(h5pp::type::compound::H5T_SCALAR2_ULONG);
        if constexpr (tc::is_Scalar2_of_type<DecayType, unsigned long long>())     return H5Tcopy(h5pp::type::compound::H5T_SCALAR2_ULLONG);
        if constexpr (tc::is_Scalar2_of_type<DecayType, double>())                 return H5Tcopy(h5pp::type::compound::H5T_SCALAR2_DOUBLE);
        if constexpr (tc::is_Scalar2_of_type<DecayType, long double>())            return H5Tcopy(h5pp::type::compound::H5T_SCALAR2_LDOUBLE);
        if constexpr (tc::is_Scalar2_of_type<DecayType, float>())                  return H5Tcopy(h5pp::type::compound::H5T_SCALAR2_FLOAT);
        if constexpr (tc::is_Scalar3_of_type<DecayType, short>())                  return H5Tcopy(h5pp::type::compound::H5T_SCALAR3_SHORT);
        if constexpr (tc::is_Scalar3_of_type<DecayType, int>())                    return H5Tcopy(h5pp::type::compound::H5T_SCALAR3_INT);
        if constexpr (tc::is_Scalar3_of_type<DecayType, long>())                   return H5Tcopy(h5pp::type::compound::H5T_SCALAR3_LONG);
        if constexpr (tc::is_Scalar3_of_type<DecayType, long long>())              return H5Tcopy(h5pp::type::compound::H5T_SCALAR3_LLONG);
        if constexpr (tc::is_Scalar3_of_type<DecayType, unsigned short>())         return H5Tcopy(h5pp::type::compound::H5T_SCALAR3_USHORT);
        if constexpr (tc::is_Scalar3_of_type<DecayType, unsigned int>())           return H5Tcopy(h5pp::type::compound::H5T_SCALAR3_UINT);
        if constexpr (tc::is_Scalar3_of_type<DecayType, unsigned long>())          return H5Tcopy(h5pp::type::compound::H5T_SCALAR3_ULONG);
        if constexpr (tc::is_Scalar3_of_type<DecayType, unsigned long long>())     return H5Tcopy(h5pp::type::compound::H5T_SCALAR3_ULLONG);
        if constexpr (tc::is_Scalar3_of_type<DecayType, double>())                 return H5Tcopy(h5pp::type::compound::H5T_SCALAR3_DOUBLE);
        if constexpr (tc::is_Scalar3_of_type<DecayType, long double>())            return H5Tcopy(h5pp::type::compound::H5T_SCALAR3_LDOUBLE);
        if constexpr (tc::is_Scalar3_of_type<DecayType, float>())                  return H5Tcopy(h5pp::type::compound::H5T_SCALAR3_FLOAT);
        if constexpr (tc::has_Scalar_v <DecayType>)                                return getH5Type<typename DecayType::Scalar>();
        if constexpr (tc::has_value_type_v <DecayType>)                            return getH5Type<typename DataType::value_type>();
        if constexpr (std::is_class_v<DataType>) return H5Tcreate(H5T_COMPOUND, sizeof(DataType)); // Last resort

        /* clang-format on */
        h5pp::logger::log->critical("getH5Type could not match the type provided: {}", type::sfinae::type_name<DecayType>());
        throw std::logic_error("getH5Type could not match the type provided: " + std::string(type::sfinae::type_name<DecayType>()));
        return hid_t(0);
    }

    //    template<typename DataType>
    //    inline void resizeDataLikeDset(DataType &data, MetaData &metaData, const MetaDset &metaDset) {
    //        if(not metaDset.dsetDims) throw std::runtime_error("Failed to resize given data: dataset dimensions have not been initialized");
    //        size_t size_before = 0;
    //        if constexpr(h5pp::type::sfinae::has_size_v<DataType>) size_before = data.size();
    //
    //#ifdef H5PP_EIGEN3
    //        if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType> and h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
    //            if(metaDset.dsetDims.value().size() != 1)
    //                throw std::runtime_error(h5pp::format("Failed to resize 1-dimensional Eigen type: Dataset has dimensions {}", metaDset.dsetDims.value()));
    //            data.resize(metaDset.dsetDims.value()[0]);
    //        } else if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType> and not h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
    //            if(metaDset.dsetDims.value().size() != 2)
    //                throw std::runtime_error(h5pp::format("Failed to resize 2-dimensional Eigen type: Dataset has dimensions {}", metaDset.dsetDims.value()));
    //            data.resize(metaDset.dsetDims.value()[0], metaDset.dsetDims.value()[1]);
    //        } else if constexpr(h5pp::type::sfinae::is_eigen_tensor_v<DataType>) {
    //            if constexpr(h5pp::type::sfinae::has_resizeN_v<DataType, DataType::NumDimensions>) {
    //                if(metaDset.dsetDims.value().size() != DataType::NumDimensions)
    //                    throw std::runtime_error(
    //                        h5pp::format("Failed to resize {}-dimensional Eigen tensor: Dataset has dimensions {}", DataType::NumDimensions, metaDset.dsetDims.value()));
    //                auto eigenDims = eigen::copy_dims<DataType::NumDimensions>(metaDset.dsetDims.value());
    //                data.resize(eigenDims);
    //            }
    //
    //        } else
    //#endif // H5PP_EIGEN3
    //            if constexpr(std::is_same_v<std::string, DataType>) {
    //            if(H5Tget_class(metaDset.h5_type.value()) == H5T_STRING) data.resize(H5Tget_size(metaDset.h5_type.value()));
    //        } else if constexpr(h5pp::type::sfinae::has_resize_v<DataType>)
    //            data.resize(metaDset.dsetDims.value()[0]);
    //
    //        metaData.dataSize = metaDset.dsetSize;
    //        metaData.dataRank = metaDset.dsetRank;
    //        metaData.dataDims = metaDset.dsetDims;
    //        metaData.dataByte = metaDset.dsetByte;
    //
    //        if constexpr(h5pp::type::sfinae::has_size_v<DataType>) {
    //            if(size_before != (size_t) data.size()) h5pp::logger::log->debug("Resized container {} -> {}", size_before, data.size());
    //
    //            size_t real_size = data.size();
    //            size_t dset_size = metaDset.dsetSize.value();
    //            if constexpr(h5pp::type::sfinae::is_text_v<DataType>) dset_size = metaDset.dsetByte.value();
    //            if((hsize_t) real_size != dset_size)
    //                throw std::runtime_error(
    //                    h5pp::format("Size mismatch on data after resize. \n\t data: actual size [{}] | dset size {}  | dims {}", real_size, dset_size,
    //                    metaData.dataDims.value()));
    //        }
    //    }

    template<typename DataType, size_t size>
    [[nodiscard]] constexpr size_t getArraySize([[maybe_unused]] const DataType (&arr)[size]) {
        //        return size; // We want to return the size including the null terminator if it's a char array!
        if constexpr(h5pp::type::sfinae::is_text_v<DataType>) {
            // A C-style char array is a null-terminated array, that has size = characters + 1
            // Here we want to return the number of characters that can fit in the array,
            // and we are not interested in the number of characters currently there.
            return std::max(strnlen(arr, size), size - 1);
        } else
            return size;
    }

    template<typename IterableType = std::initializer_list<hsize_t>,
             typename              = std::enable_if_t<h5pp::type::sfinae::is_integral_num_or_list_v<IterableType>>>
    [[nodiscard]] std::optional<std::vector<hsize_t>> getOptionalIterable(const IterableType &iterable) {
        std::optional<std::vector<hsize_t>> optiter = std::nullopt;
        if constexpr(h5pp::type::sfinae::is_iterable_v<IterableType>) {
            if(iterable.size() > 0) {
                optiter = std::vector<hsize_t>();
                std::copy(iterable.begin(), iterable.end(), std::back_inserter(optiter.value()));
            }
        } else if constexpr(std::is_integral_v<IterableType>) {
            optiter = std::vector<hsize_t>{(hsize_t)iterable};
        }
        return optiter;
    }

    template<typename IterableType = std::initializer_list<hsize_t>,
             typename              = std::enable_if_t<h5pp::type::sfinae::is_integral_num_or_list_v<IterableType>>>
    [[nodiscard]] std::optional<std::vector<hsize_t>> getOptionalIterable(const std::optional<IterableType> &iterable) {
        if(iterable)
            return getOptionalIterable(iterable.value());
        else
            return std::nullopt;
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] hsize_t getSize(const DataType &data) {
        //        if constexpr(h5pp::type::sfinae::is_text_v<DataType>) return 1;
        if constexpr(h5pp::type::sfinae::has_size_v<DataType>) return data.size();
        if constexpr(std::is_array_v<DataType>) return getArraySize(data);

        // Add more checks here. As it is, these two checks above handle all cases I have encountered.
        return 1; // All others should be "H5S_SCALAR" of size 1.
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] constexpr int getRank() {
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_tensor_v<DataType>) return (int) DataType::NumIndices;
        if constexpr(h5pp::type::sfinae::is_eigen_1d_v<DataType>) return 1;
        if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType>) return 2;
#endif
        if constexpr(h5pp::type::sfinae::has_NumIndices_v<DataType>) return (int) DataType::NumIndices;
        if constexpr(std::is_array_v<DataType>) return std::rank_v<DataType>;
        if constexpr(h5pp::type::sfinae::has_size_v<DataType>) return 1;
        return 0;
    }
    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] int getRank(std::optional<std::vector<hsize_t>> desiredDims) {
        if(desiredDims)
            return (int) desiredDims->size();
        else
            return getRank<DataType>();
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] std::vector<hsize_t> getDimensions(const DataType &data, std::optional<std::vector<hsize_t>> desiredDims = std::nullopt) {
        // Empty vector means scalar
        if(desiredDims) {
            // Check that the desired dimensions add up to the data size
            auto   dataSize    = getSize(data);
            size_t desiredSize = std::accumulate(desiredDims->begin(), desiredDims->end(), 1, std::multiplies<>());
            if(dataSize != desiredSize) throw std::runtime_error(h5pp::format("Desired dimensions [{}] do not match the given data size [{}]", desiredDims.value(), dataSize));
            return desiredDims.value();
        }
        constexpr int rank = getRank<DataType>();
        if constexpr(rank == 0) return {};
        if constexpr(h5pp::type::sfinae::has_dimensions_v<DataType>) {
            std::vector<hsize_t> dims(data.dimensions().begin(),
                                      data.dimensions().end()); // We copy because the vectors may not be assignable or may not be implicitly convertible to hsize_t.
            if(data.dimensions().size() != rank) throw std::runtime_error("given dimensions do not match detected rank");
            if(dims.size() != rank) throw std::runtime_error("copied dimensions do not match detected rank");
            return dims;
        } else if constexpr(h5pp::type::sfinae::has_size_v<DataType> and rank == 1)
            return {(hsize_t) data.size()};

#ifdef H5PP_EIGEN3
        else if constexpr(h5pp::type::sfinae::is_eigen_tensor_v<DataType>) {
            assert(data.dimensions().size() == rank and "given dimensions do not match detected rank");
            std::vector<hsize_t> dims(data.dimensions().begin(),
                                      data.dimensions().end()); // We copy because the vectors may not be assignable or may not be implicitly convertible to hsize_t.
            return dims;
        } else if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType>) {
            std::vector<hsize_t> dims(rank);
            dims[0] = (hsize_t) data.rows();
            dims[1] = (hsize_t) data.cols();
            return dims;
        }
#endif
        else if constexpr(h5pp::type::sfinae::is_ScalarN<DataType>())
            return {};
        else if constexpr(std::is_array_v<DataType>)
            return {getArraySize(data)};
        else if constexpr(std::is_arithmetic_v<DataType> or h5pp::type::sfinae::is_std_complex_v<DataType>)
            return {};
        else if constexpr(std::is_pod_v<DataType>)
            return {};
        else if constexpr(std::is_standard_layout_v<DataType>)
            return {};
        else {
            h5pp::type::sfinae::print_type_and_exit_compile_time<DataType>();
            std::string error = "getDimensions can't match the type provided: " + h5pp::type::sfinae::type_name<DataType>();
            h5pp::logger::log->critical(error);
            throw std::logic_error(error);
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] std::vector<hsize_t>
        getDimensionsMax(const DataType &data, H5D_layout_t h5_layout, const std::vector<hsize_t> &dims, std::optional<std::vector<hsize_t>> desiredDimsMax = std::nullopt) {
        /* From the docs
         *  Any element of current_dims can be 0 (zero).
         *  Note that no data can be written to a dataset if the size of any dimension of its current dataspace is 0.
         *  This is sometimes a useful initial state for a dataset.
         *  Maximum_dims may be the null pointer, in which case the upper limit is the same as current_dims.
         *  Otherwise, no element of maximum_dims should be smaller than the corresponding element of current_dims.
         *  If an element of maximum_dims is H5S_UNLIMITED, the maximum size of the corresponding dimension is unlimited.
         *  Any dataset with an unlimited dimension must also be chunked; see H5Pset_chunk.
         *  Similarly, a dataset must be chunked if current_dims does not equal maximum_dims.
         */

        if(desiredDimsMax) {
            // Check that the desired max dimensions are larger than the dimensions
            if(desiredDimsMax->size() != dims.size())
                throw std::runtime_error(h5pp::format("Rank mismatch. The dimensions {} and given max dimensions {} are have different rank", dims, desiredDimsMax.value()));
            // Sanity check
            for(size_t i = 0; i < dims.size(); i++) {
                if(h5_layout == H5D_CHUNKED) {
                    if(desiredDimsMax->at(i) != H5S_UNLIMITED and desiredDimsMax->at(i) <= dims[i])
                        throw std::runtime_error(h5pp::format("Given max dimensions {} are smaller than the current dimensions {}", desiredDimsMax.value(), dims));
                } else {
                    if(desiredDimsMax->at(i) != dims[i]) throw std::runtime_error("A dataset must be H5D_CHUNKED if dimensions do not equal max dimensions");
                }
            }
            return desiredDimsMax.value();
        }
        std::vector<hsize_t> maxDims = dims;
        if(h5_layout == H5D_CHUNKED) std::fill_n(maxDims.begin(), dims.size(), H5S_UNLIMITED);
        return maxDims;
    }

    [[nodiscard]] inline hid::h5s getDataSpace(const hsize_t size, const std::vector<hsize_t> &dims, const H5D_layout_t h5_layout, std::optional<std::vector<hsize_t>> desiredDimsMax = std::nullopt) {
//        size_t size = std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<>());
//        int    rank = (int) dims.size();
        if(dims.empty() and size > 0)
            return H5Screate(H5S_SCALAR);
        else if(dims.empty() and size == 0)
            return H5Screate(H5S_NULL);
        else {
            hsize_t num_elements = std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<>());
            if(size != num_elements) throw std::runtime_error(h5pp::format("Number of elements mismatch: size {} | dimensions {}", size,dims));

            // Chunked layout datasets can be extended, which is why their max extent in any dimension may be unlimited.
            std::vector<hsize_t> maxDims = dims;
            if(desiredDimsMax)
                maxDims = desiredDimsMax.value();
            else if(h5_layout == H5D_CHUNKED)
                std::fill_n(maxDims.begin(), dims.size(), H5S_UNLIMITED);
            // Sanity check
            for(size_t i = 0; i < dims.size(); i++) {
                if(h5_layout == H5D_CHUNKED) {
                    if(maxDims[i] != H5S_UNLIMITED and maxDims[i] <= dims[i])
                        throw std::runtime_error(h5pp::format("Given max dimensions {} are smaller than the current dimensions {}", maxDims, dims));
                } else {
                    if(maxDims[i] != dims[i]) throw std::runtime_error("A dataset must be H5D_CHUNKED if dimensions do not equal max dimensions");
                }
            }
            return H5Screate_simple((int) dims.size(), dims.data(), maxDims.data());
        }
    }

    /*
     * memspace is a description of the buffer in memory (i.e. where read elements will go).
     * If there is no data conversion, then data is read directly into the user supplied buffer.
     * If there is data conversion, HDF5 uses a 1MB buffer to do the conversions,
     * but we still use the user's buffer for reading data in the first place.
     * Also, you can adjust the 1MB default conversion buffer size. (see H5Pset_buffer)
     */
    [[nodiscard]] inline hid::h5s getMemSpace(const hsize_t size, const std::vector<hsize_t> &dims) {

        if(dims.empty() and size > 0) return H5Screate(H5S_SCALAR);
        else if (dims.empty() and size == 0)
            return H5Screate(H5S_NULL);
        else {
            hsize_t num_elements = std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<>());
            if(size != num_elements) throw std::runtime_error(h5pp::format("Number of elements mismatch: size {} | dimensions {}", size,dims));
            return H5Screate_simple((int) dims.size(), dims.data(), nullptr);
        }
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] size_t getBytesPerElem() {
        namespace sfn   = h5pp::type::sfinae;
        using DecayType = typename std::decay<DataType>::type;
        if constexpr(std::is_pointer_v<DecayType>) return getBytesPerElem<typename std::remove_pointer<DecayType>::type>();
        if constexpr(std::is_reference_v<DecayType>) return getBytesPerElem<typename std::remove_reference<DecayType>::type>();
        if constexpr(std::is_array_v<DecayType>) return getBytesPerElem<typename std::remove_all_extents<DecayType>::type>();
        if constexpr(sfn::is_std_complex<DecayType>()) return sizeof(DecayType);
        if constexpr(sfn::is_ScalarN<DecayType>()) return sizeof(DecayType);
        if constexpr(std::is_arithmetic_v<DecayType>) return sizeof(DecayType);
        if constexpr(sfn::has_Scalar_v<DecayType>) return sizeof(typename DecayType::Scalar);
        if constexpr(sfn::has_value_type_v<DecayType>) return sizeof(typename DecayType::value_type);
        return sizeof(std::remove_all_extents_t<DecayType>);
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] size_t getBytesTotal(const DataType &data) {
        if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, std::string>) {
            size_t num_elems = 0;
            for(auto &elem : data) num_elems += elem.size();
            return num_elems * h5pp::util::getBytesPerElem<std::string>();
        } else if constexpr(h5pp::type::sfinae::is_container_of_v<DataType, char[]>) {
            size_t num_elems = 0;
            for(auto &elem : data) num_elems += getArraySize(elem);
            return num_elems * h5pp::util::getBytesPerElem<char[]>();
        }
        auto size         = h5pp::util::getSize(data);
        auto bytesperelem = h5pp::util::getBytesPerElem<DataType>();
        return size * bytesperelem;
        //        return h5pp::utils::getBytesPerElem<DataType>() * h5pp::utils::getSize(data);
    }

    [[nodiscard]] inline H5D_layout_t decideLayout(const size_t bytes, std::optional<H5D_layout_t> desiredLayout = std::nullopt) {
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
        if(bytes < h5pp::constants::maxSizeCompact)
            return H5D_COMPACT;
        else if(bytes < h5pp::constants::maxSizeContiguous)
            return H5D_CONTIGUOUS;
        else
            return H5D_CHUNKED;
    }

    [[nodiscard]] inline std::vector<hsize_t>
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
            while(chunkDims[dim] > (hsize_t) 1) {
                size_t chunkSize = std::accumulate(chunkDims.begin(), chunkDims.end(), (hsize_t) 1, std::multiplies<>());
                if(chunkSize <= slice_size)
                    break;
                else
                    chunkDims[dim]--;
            }
        }
        return chunkDims;
    }

    template<typename DataType>
    inline void resizeData(DataType &data, MetaData &metaData, const std::vector<hsize_t> &newDims) {
        // This function may shrink a container!
        size_t size_before = 0;
        if constexpr(h5pp::type::sfinae::has_size_v<DataType>) size_before = data.size();

#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType> and h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            if(newDims.size() != 1) throw std::runtime_error(h5pp::format("Failed to resize 1-dimensional Eigen type: Dataset has dimensions {}", newDims));
            data.resize(newDims[0]);
        } else if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType> and not h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            if(newDims.size() != 2) throw std::runtime_error(h5pp::format("Failed to resize 2-dimensional Eigen type: Dataset has dimensions {}", newDims));
            data.resize(newDims[0], newDims[1]);
        } else if constexpr(h5pp::type::sfinae::is_eigen_tensor_v<DataType>) {
            if constexpr(h5pp::type::sfinae::has_resizeN_v<DataType, DataType::NumDimensions>) {
                if(newDims.size() != DataType::NumDimensions)
                    throw std::runtime_error(h5pp::format("Failed to resize {}-dimensional Eigen tensor: Dataset has dimensions {}", DataType::NumDimensions, newDims));
                auto eigenDims = eigen::copy_dims<DataType::NumDimensions>(newDims);
                data.resize(eigenDims);
            }

        } else
#endif // H5PP_EIGEN3
            if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) {
            auto newSize = std::accumulate(newDims.begin(), newDims.end(), 1, std::multiplies<>());
            if(newDims.size() > 1) h5pp::logger::log->debug("Given data container is 1-dimensional but the desired dimensions are {}. Resizing to fit all the data", newDims);
            data.resize(newSize);
        }

        //        metaData.dataDims = newDims;
        //        metaData.dataSize = getSize(data);
        //        metaData.dataRank = getRank<DataType>(newDims);
        //        metaData.dataByte = getBytesTotal(data);

        if constexpr(h5pp::type::sfinae::has_size_v<DataType>)
            if(size_before != (size_t) data.size()) h5pp::logger::log->debug("Resized container {} -> {}", size_before, data.size());
    }

    template<typename DataType>
    inline void resizeData(DataType &data, MetaData &metaData, const hid::h5s &space, const hid::h5t &type) {
        // This function is used when reading data from file into memory.
        // It resizes the data so the space in memory can fit the data read from file.
        // Note that this resizes the data to fit the bounding box of the data selected in the fileSpace.
        // A selection of elements in memory space must occurr after calling this function.

        if(H5Tget_class(type) == H5T_STRING) {
            resizeData(data, metaData, {(hsize_t) H5Tget_size(type)});
        } else if(H5Sget_simple_extent_type(space) == H5S_SCALAR) {
            resizeData(data, metaData, {(hsize_t) 1});
        } else {
            //            if(H5Sget_simple_extent_type(space) == H5S_class_t::H5S_SCALAR) return;
            // Reaching this point means that the space is "simple", i.e. one or more rectangular regions in memory.

            int                  rank = H5Sget_simple_extent_ndims(space); // This will define the bounding box of the selected elements in the file space
            std::vector<hsize_t> extent(rank, 0);
            H5Sget_simple_extent_dims(space, extent.data(), nullptr);
            resizeData(data, metaData, extent);
        }
    }

}
