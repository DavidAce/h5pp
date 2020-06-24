#pragma once
#include "h5ppOptional.h"
#include "h5ppTypeSfinae.h"
#include <cstring>
#include <numeric>

/*!
 * \brief A collection of functions to get information about C++ types passed by the user
 */
namespace h5pp::util {

    inline std::string safe_str(std::string_view str) {
        // This function removes null-terminating characters inside of
        // strings. This happens sometimes when strings are concatenated but
        // for various reasons the middle null terminator is not removed.
        // Note that the last null terminator is left where it is.
        if(str.empty()) return std::string(str);
        std::string tmp(str);
        size_t      start_pos = 0;
        while((start_pos = tmp.find('\0', start_pos)) != std::string::npos) {
            tmp.replace(start_pos, 1, "");
            start_pos += 1;
        }
        return tmp;
    }

    template<typename DataType>
    [[nodiscard]] hid::h5t getH5Type() {
        //        if(h5type.has_value()) return h5type.value(); // Intercept
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
        if constexpr (std::is_same_v<DecayType, bool>)                             return H5Tcopy(H5T_NATIVE_UINT8);
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
        throw std::logic_error(h5pp::format("getH5Type could not match the type provided [{}] | size {}", type::sfinae::type_name<DecayType>(), sizeof(DecayType)));
        return hid_t(0);
    }

    template<typename DataType, size_t size>
    [[nodiscard]] constexpr size_t getArraySize([[maybe_unused]] const DataType (&arr)[size]) {
        //        return size; // We want to return the size including the null terminator if it's a char array!
        if constexpr(h5pp::type::sfinae::is_text_v<DataType>) {
            // A C-style char array is a null-terminated array, that has size = characters + 1
            // Here we want to return the number of characters that can fit in the array,
            // and we are not interested in the number of characters currently there.
            return strnlen(arr, size) + 1;
            //            return std::max(strnlen(arr, size), size - 1);
        } else
            return size;
    }
    template<typename DataType, size_t rows, size_t cols>
    [[nodiscard]] constexpr std::array<size_t, 2> getArraySize([[maybe_unused]] const DataType (&arr)[rows][cols]) {
        return {rows, cols};
    }
    template<typename DataType, size_t rows, size_t cols, size_t depth>
    [[nodiscard]] constexpr std::array<size_t, 3> getArraySize([[maybe_unused]] const DataType (&arr)[rows][cols][depth]) {
        return {rows, cols, depth};
    }

    template<typename IntegralOrIterableType = std::initializer_list<hsize_t>, typename = h5pp::type::sfinae::enable_if_is_integral_iterable_or_num<IntegralOrIterableType>>
    [[nodiscard]] std::vector<hsize_t> getDimVector(const IntegralOrIterableType &iterable) {
        if constexpr(std::is_same_v<IntegralOrIterableType, std::vector<hsize_t>>)
            return iterable;
        else {
            if constexpr(h5pp::type::sfinae::is_iterable_v<IntegralOrIterableType>) {
                std::vector<hsize_t> dimVec;
                std::copy(iterable.begin(), iterable.end(), std::back_inserter(dimVec));
                return dimVec;
            } else if constexpr(std::is_integral_v<IntegralOrIterableType>) {
                return std::vector<hsize_t>{static_cast<hsize_t>(iterable)};
            } else
                throw std::logic_error("Wrong dimension type detected. Sfinae has done something wrong");
        }
    }

    template<typename IntegralOrIterableOrOptType = std::initializer_list<hsize_t>,
             typename                             = h5pp::type::sfinae::enable_if_is_integral_iterable_or_nullopt<IntegralOrIterableOrOptType>>
    [[nodiscard]] std::optional<std::vector<hsize_t>> getOptionalDimVector(const IntegralOrIterableOrOptType &iterable) {
        if constexpr(std::is_same_v<IntegralOrIterableOrOptType, std::nullopt_t>)
            return iterable;
        else
            return getDimVector(iterable);
    }

    [[nodiscard]] inline hsize_t getSizeFromDimensions(const std::vector<hsize_t> &dims) {
        return std::accumulate(dims.begin(), dims.end(), static_cast<hsize_t>(1), std::multiplies<>());
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] hsize_t getSize(const DataType &data) {
        if constexpr(h5pp::type::sfinae::is_text_v<DataType>) return static_cast<hsize_t>(1); // Strings and char arrays are treated as a single unit
        if constexpr(h5pp::type::sfinae::has_size_v<DataType>) return static_cast<hsize_t>(data.size());
        if constexpr(std::is_array_v<DataType>) return static_cast<hsize_t>(getArraySize(data));
        if constexpr(std::is_pointer_v<DataType>) throw std::runtime_error("Failed to read data size: Pointer data has no specified dimensions");
        // Add more checks here. As it is, these two checks above handle all cases I have encountered.
        return 1; // All others should be "H5S_SCALAR" of size 1.
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] constexpr int getRank() {
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_tensor_v<DataType>) return static_cast<int>(DataType::NumIndices);
        if constexpr(h5pp::type::sfinae::is_eigen_1d_v<DataType>) return 1;
        if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType>) return 2;
#endif
        if constexpr(h5pp::type::sfinae::is_text_v<DataType>) return 0;
        if constexpr(h5pp::type::sfinae::has_NumIndices_v<DataType>) return static_cast<int>(DataType::NumIndices);
        if constexpr(std::is_array_v<DataType>) return std::rank_v<DataType>;
        if constexpr(h5pp::type::sfinae::has_size_v<DataType>) return 1;
        return 0;
    }

    [[nodiscard]] inline int getRankFromDimensions(const std::vector<hsize_t> &dims) { return static_cast<int>(dims.size()); }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] std::vector<hsize_t> getDimensions(const DataType &data) {
        // Empty vector means H5S_SCALAR, size 1 and rank 0
        if constexpr(h5pp::type::sfinae::is_text_v<DataType>) return {};
        constexpr int rank = getRank<DataType>();
        if constexpr(rank == 0) return {};
        if constexpr(h5pp::type::sfinae::has_dimensions_v<DataType>) {
            std::vector<hsize_t> dims(data.dimensions().begin(),
                                      data.dimensions().end()); // We copy because the vectors may not be assignable or may not be implicitly convertible to hsize_t.
            if(data.dimensions().size() != rank) throw std::runtime_error("given dimensions do not match detected rank");
            if(dims.size() != rank) throw std::runtime_error("copied dimensions do not match detected rank");
            return dims;
        } else if constexpr(h5pp::type::sfinae::has_size_v<DataType> and rank == 1)
            return {static_cast<hsize_t>(data.size())};

#ifdef H5PP_EIGEN3
        else if constexpr(h5pp::type::sfinae::is_eigen_tensor_v<DataType>) {
            assert(data.dimensions().size() == rank and "given dimensions do not match detected rank");
            std::vector<hsize_t> dims(data.dimensions().begin(),
                                      data.dimensions().end()); // We copy because the vectors may not be assignable or may not be implicitly convertible to hsize_t.
            return dims;
        } else if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType>) {
            std::vector<hsize_t> dims(rank);
            dims[0] = static_cast<hsize_t>(data.rows());
            dims[1] = static_cast<hsize_t>(data.cols());
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
        else if constexpr(std::is_class_v<DataType>) {
            h5pp::logger::log->warn("Detected possible unsupported non-POD class. h5pp may fail.");
            return {};
        }
        //        else return {};
        else {
            //            h5pp::type::sfinae::print_type_and_exit_compile_time<DataType>();
            throw std::logic_error(h5pp::format("getDimensions can't match the type provided [{}]", h5pp::type::sfinae::type_name<DataType>()));
        }
    }

    [[nodiscard]] inline std::optional<std::vector<hsize_t>> decideDimensionsMax(const std::vector<hsize_t> &dims, std::optional<H5D_layout_t> h5_layout) {
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
        if(h5_layout and h5_layout.value() != H5D_CHUNKED) return std::nullopt;
        std::vector<hsize_t> maxDims = dims;
        if(h5_layout == H5D_CHUNKED) std::fill_n(maxDims.begin(), dims.size(), H5S_UNLIMITED);
        return maxDims;
    }

    [[nodiscard]] inline hid::h5s
        getDsetSpace(const hsize_t size, const std::vector<hsize_t> &dims, const H5D_layout_t &h5_layout, std::optional<std::vector<hsize_t>> desiredDimsMax = std::nullopt) {
        if(dims.empty() and size > 0)
            return H5Screate(H5S_SCALAR);
        else if(dims.empty() and size == 0)
            return H5Screate(H5S_NULL);
        else {
            auto num_elements = h5pp::util::getSizeFromDimensions(dims);
            if(size != num_elements) throw std::runtime_error(h5pp::format("Number of elements mismatch: size {} | dimensions {}", size, dims));
            if(desiredDimsMax and desiredDimsMax->size() != dims.size())
                throw std::runtime_error(h5pp::format("Number of dimensions (rank) mismatch: dims {} | max dims {}\n"
                                                      "\t Hint: Try giving the data dimensions explicitly, with rank matching the maximum dimensions",
                                                      dims,
                                                      desiredDimsMax.value()));

            // Only and chunked and datasets can be extended. The extension can happen upp to the max dimension specified.
            // If the max dimension is H5S_UNLIMITED, then the dataset can grow to any dimension.
            // Conversely, if the dataset is not H5D_CHUNKED, the dims == max dims must hold always

            std::vector<hsize_t> maxDims = dims;
            if(desiredDimsMax) {
                maxDims = desiredDimsMax.value();
                for(size_t idx = 0; idx < maxDims.size(); idx++) {
                    if(maxDims[idx] == H5S_UNLIMITED and h5_layout != H5D_CHUNKED)
                        throw std::runtime_error(h5pp::format("Max dimensions {} has an H5S_UNLIMITED dimension at index {}. This requires H5D_CHUNKED layout", maxDims, idx));
                }
                if(h5_layout == H5D_COMPACT and dims != maxDims)
                    throw std::runtime_error(h5pp::format("Dimension mismatch: dims {} != max dims {}. Equality is required for H5D_COMPACT layout", dims, maxDims));
                if(h5_layout == H5D_CONTIGUOUS and dims != maxDims)
                    throw std::runtime_error(h5pp::format("Dimension mismatch: dims {} != max dims {}. Equality is required for H5D_CONTIGUOUS layout", dims, maxDims));
            } else if(h5_layout == H5D_CHUNKED) // Here the max dimensions were not given, but H5D_CHUNKED was asked for
                std::fill_n(maxDims.begin(), dims.size(), H5S_UNLIMITED);

            return H5Screate_simple(static_cast<int>(dims.size()), dims.data(), maxDims.data());
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
        if(dims.empty() and size > 0)
            return H5Screate(H5S_SCALAR);
        else if(dims.empty() and size == 0)
            return H5Screate(H5S_NULL);
        else {
            auto num_elements = getSizeFromDimensions(dims);
            if(size != num_elements) throw std::runtime_error(h5pp::format("Number of elements mismatch: size {} | dimensions {}", size, dims));
            return H5Screate_simple(static_cast<int>(dims.size()), dims.data(), nullptr);
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
        if constexpr(sfn::has_value_type_v<DecayType>) return getBytesPerElem<typename DecayType::value_type>();
        return sizeof(std::remove_all_extents_t<DecayType>);
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] size_t getBytesTotal(const DataType &data, std::optional<size_t> size = std::nullopt) {
        if constexpr(h5pp::type::sfinae::is_iterable_v<DataType> and h5pp::type::sfinae::has_value_type_v<DataType>) {
            using value_type = typename DataType::value_type;
            if constexpr(h5pp::type::sfinae::has_size_v<value_type>) { // E.g. std::vector<std::string>
                size_t num      = 0;
                size_t nullterm = h5pp::type::sfinae::is_text_v<value_type> ? 1 : 0;
                for(auto &elem : data) num += elem.size() + nullterm;
                return num * h5pp::util::getBytesPerElem<value_type>();
            } else if constexpr(h5pp::type::sfinae::has_size_v<DataType>) { // E.g. std::string or std::vector<double>
                size_t num      = 0;
                size_t nullterm = h5pp::type::sfinae::is_text_v<DataType> ? 1 : 0;
                num             = data.size() + nullterm;
                return num * h5pp::util::getBytesPerElem<value_type>();
            }
        }
        auto bytesperelem = h5pp::util::getBytesPerElem<DataType>();
        if constexpr(std::is_array_v<DataType>) {
            if(not size) size      = h5pp::util::getArraySize(data);
            return size.value() * bytesperelem;
        }
        if constexpr(std::is_same_v<DataType,const char *> or std::is_same_v<DataType,char *>){
            size = strlen(data);
            return size.value() * bytesperelem;
        }
        if constexpr(std::is_pointer_v<DataType>){
            if(not size) throw std::runtime_error("Could not determine total amount of bytes in buffer: Pointer data has no specified size");
            return size.value() * bytesperelem;
        }

        if(not size) size  = h5pp::util::getSize(data);
        return size.value() * bytesperelem;
    }

    [[nodiscard]] inline H5D_layout_t decideLayout(const size_t totalBytes) {
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
        // Otherwise we decide based on size
        if(totalBytes < h5pp::constants::maxSizeCompact)
            return H5D_COMPACT;
        else if(totalBytes < h5pp::constants::maxSizeContiguous)
            return H5D_CONTIGUOUS;
        else
            return H5D_CHUNKED;
    }

    template<typename DataType>
    [[nodiscard]] inline H5D_layout_t decideLayout(const DataType &data, std::optional<std::vector<hsize_t>> dsetDims, std::optional<std::vector<hsize_t>> dsetDimsMax) {
        // Here we try to compute the maximum number of bytes of this dataset, so we can decide its layout
        hsize_t size = 0;
        if(dsetDimsMax) {
            if(dsetDims and dsetDims.value() != dsetDimsMax.value()) return H5D_CHUNKED;
            for(auto &dim : dsetDimsMax.value())
                if(dim == H5S_UNLIMITED) return H5D_CHUNKED;
            size = getSizeFromDimensions(dsetDimsMax.value());
        } else if(dsetDims)
            size = getSizeFromDimensions(dsetDims.value());
        else
            size = getSize(data);
        auto bytes = size * getBytesPerElem<DataType>();
        return decideLayout(bytes);
    }

    inline std::optional<std::vector<hsize_t>>
        getChunkDimensions(size_t bytesPerElem, const std::vector<hsize_t> &dims, std::optional<std::vector<hsize_t>> &dimsMax, std::optional<H5D_layout_t> layout) {
        // Here we make a naive guess for chunk dimensions
        // We try to make a square in N dimensions with a target byte size of 10kb - 1MB.
        // Here is a great read for chunking considerations https://www.oreilly.com/library/view/python-and-hdf5/9781491944981/ch04.html
        // Hard rules for chunk dimensions:
        //  * A chunk dimension cannot be larger than the corresponding max dimension
        //  * A chunk dimension can be larger or smaller than the dataset dimension
        //  *

        if(layout and layout.value() != H5D_CHUNKED) return std::nullopt;
        if(dims.empty()) return dims; // Scalar
        // We try to compute the maximum dimension to get a good estimate
        // of the data volume
        std::vector<hsize_t> dims_effective = dims;
        // Make sure the chunk dimensions have strictly nonzero volume
        for(auto & dim : dims_effective) dim = std::max<hsize_t>(1,dim);
        if(dimsMax) {
            // If max dims are given, dims that are not H5S_UNLIMITED are used as an upper bound
            // for that dimension
            if(dimsMax->size() != dims.size())
                throw std::runtime_error(h5pp::format("Could not get chunk dimensions: "
                                                      "dims {} and max dims {} have different number of elements",
                                                      dims,
                                                      dimsMax.value()));
            for(size_t idx = 0; idx < dims.size(); idx++) {
                if(dimsMax.value()[idx] < dims[idx])
                    throw std::runtime_error(h5pp::format("Could not get chunk dimensions: "
                                                          "Some elements in dims exceed max dims: "
                                                          "dims {} | max dims {}",
                                                          dims,
                                                          dimsMax.value()));
                if(dimsMax.value()[idx] != H5S_UNLIMITED) dims_effective[idx] = std::max(dims_effective[idx], dimsMax.value()[idx]);
            }
        }

        auto rank             = dims.size();
        auto volumeChunkBytes = static_cast<size_t>(std::pow(*std::max_element(dims_effective.begin(), dims_effective.end()), rank)) * bytesPerElem;
        auto targetChunkBytes = std::max<size_t>(volumeChunkBytes, h5pp::constants::minChunkSize);
        targetChunkBytes      = std::min<size_t>(targetChunkBytes, h5pp::constants::maxChunkSize);
        targetChunkBytes      = static_cast<size_t>(std::pow(2, std::ceil(std::log2(targetChunkBytes)))); // Next nearest power of two
        auto                 linearChunkSize = static_cast<hsize_t>(std::ceil(std::pow<size_t>(targetChunkBytes / bytesPerElem, 1.0 / static_cast<double>(rank))));
        std::vector<hsize_t> chunkDims(rank, linearChunkSize);
        // Now effective dims contains either dims or dimsMax (if not H5S_UNLIMITED) at each position.
        for(size_t idx = 0; idx < chunkDims.size(); idx++) {
            if(dimsMax and dimsMax.value()[idx] == H5S_UNLIMITED)
                chunkDims[idx] = linearChunkSize;
            else if(dimsMax.value()[idx] != H5S_UNLIMITED)
                chunkDims[idx] = std::min(dimsMax.value()[idx], linearChunkSize);
            else
                chunkDims[idx] = linearChunkSize;
        }
        h5pp::logger::log->debug("Chunk dimensions {}, max dims {}, effective dims {}", chunkDims,dimsMax.value(), dims_effective);
        return chunkDims;
    }

    template<typename DataType>
    inline void setStringSize(hsize_t &size, size_t &bytes, std::vector<hsize_t> &dims) {
        // The datatype may either be text or a container of text.
        // Examples of pure text are std::string or char[]
        // Example of a container of text is std::vector<std::string>
        // When
        //      1) it is pure text and dimensions are {}
        //          * Space is H5S_SCALAR because dimensions are {}
        //          * Rank is 0 because dimensions are {}
        //          * Size is 1 because size = prod 1*dim(i) * dim(j)...
        //          * We set size H5T_VARIABLE
        //      2) it is pure text and dimensions were specified other than {}
        //          * Space is H5S_SIMPLE
        //          * Rank is 1 or more because dimensions were given as {i,j,k...}
        //          * Size is n, because size = prod 1*dim(i) * dim(j)...
        //          * Here n is number of chars to get from the string buffer
        //          * We set the string size to n because each element is a char.
        //          * We set the dimension to {1}
        //      3) it is a container of text
        //          * Space is H5S_SIMPLE
        //          * The rank is 1 or more
        //          * The space size is the number of strings in the container
        //          * We set size H5T_VARIABLE
        //          * The dimensions remain the size of the container
        if constexpr(h5pp::type::sfinae::is_text_v<DataType>) {
            if(dims.empty()) {
                // Should properties should already be correctly detected
                // Case 1
                //                retval = H5Tset_size(type, H5T_VARIABLE);
            } else {
                // Case 2
                hsize_t desiredSize = h5pp::util::getSizeFromDimensions(dims);
                //                retval              = H5Tset_size(type, desiredSize);
                dims  = {};
                size  = 1;
                bytes = desiredSize * h5pp::util::getBytesPerElem<DataType>();
            }
        } else if(h5pp::type::sfinae::has_text_v<DataType>) {
            // Case 3
            //            retval = H5Tset_size(type, H5T_VARIABLE);
        }
    }

    template<typename DataType>
    inline void resizeData(DataType &data, const std::vector<hsize_t> &newDims) {
        // This function may shrink a container!
#ifdef H5PP_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType> and h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            auto newSize = getSizeFromDimensions(newDims);
            if(newDims.size() != 1)
                h5pp::logger::log->debug("Resizing given 1-dimensional Eigen type [{}] to fit dataset dimensions {}", type::sfinae::type_name<DataType>(), newDims);
            h5pp::logger::log->debug("Resizing eigen 1d container {} -> {}", std::initializer_list<Eigen::Index>{data.size()}, std::initializer_list<hsize_t>{newSize});
            data.resize(static_cast<Eigen::Index>(newSize));
        } else if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType> and not h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            if(newDims.size() != 2) throw std::runtime_error(h5pp::format("Failed to resize 2-dimensional Eigen type: Dataset has dimensions {}", newDims));
            h5pp::logger::log->debug("Resizing eigen 2d container {} -> {}", std::initializer_list<Eigen::Index>{data.rows(), data.cols()}, newDims);
            data.resize(static_cast<Eigen::Index>(newDims[0]), static_cast<Eigen::Index>(newDims[1]));
        } else if constexpr(h5pp::type::sfinae::is_eigen_tensor_v<DataType>) {
            if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) {
                if(newDims.size() != DataType::NumDimensions)
                    throw std::runtime_error(h5pp::format("Failed to resize {}-dimensional Eigen tensor: Dataset has dimensions {}", DataType::NumDimensions, newDims));
                auto eigenDims = eigen::copy_dims<DataType::NumDimensions>(newDims);
                h5pp::logger::log->debug("Resizing eigen tensor container {} -> {}", data.dimensions(), newDims);
                data.resize(eigenDims);
            } else {
                auto newSize = getSizeFromDimensions(newDims);
                if(data.size() != static_cast<Eigen::Index>(newSize))
                    h5pp::logger::log->warn("Detected non-resizeable tensor container with wrong size: Given size {}. Required size {}", data.size(), newSize);
            }
        } else
#endif // H5PP_EIGEN3
            if constexpr(h5pp::type::sfinae::has_size_v<DataType> and h5pp::type::sfinae::has_resize_v<DataType>) {
            if(newDims.size() > 1) h5pp::logger::log->debug("Given data container is 1-dimensional but the desired dimensions are {}. Resizing to fit all the data", newDims);
                auto newSize = getSizeFromDimensions(newDims);
                h5pp::logger::log->debug("Resizing 1d container {} -> {}", std::initializer_list<size_t>{static_cast<size_t>(data.size())}, newDims);
            data.resize(newSize);
        } else if constexpr(std::is_scalar_v<DataType> or std::is_class_v<DataType>) {
            return;
        } else {
            h5pp::logger::log->debug("Container could not be resized");
        }
    }

}
