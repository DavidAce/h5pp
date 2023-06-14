#pragma once
#include "h5ppConstants.h"
#include "h5ppEigen.h"
#include "h5ppInfo.h"
#include "h5ppOptional.h"
#include "h5ppType.h"
#include "h5ppTypeCast.h"
#include "h5ppTypeCompound.h"
#include "h5ppTypeSfinae.h"
#include <cstring>
#include <numeric>

/*!
 * \brief A collection of functions to get information about HDF5 and C++ types passed by the user
 */
namespace h5pp::util {

    [[nodiscard]] inline std::string safe_str(std::string_view str) {
        // This function removes null-terminating characters inside of strings. For instance
        //      "This is \0 a string with\0 embedded null characters\0"
        // becomes
        //      "This is a string with embedded null characters\0"
        // This sometimes happens for instance when concatenating strings that are "non-standard", i.e.
        // strings where .size() returns the "number of characters + 1", where "+1" accounts for '\0'.
        // This can easily happen by accident when mixing C-style arrays and std::string.
        // Since std::string supports embedded null characters '\0' this is normally not a problem.
        // However, when interfacing with the C-API of HDF5, all C-style strings are terminated
        // at the first occurence of '\0'. Therefore, we have to make sure that there are no embedded '\0'
        // characters in the strings that we pass to the HDF5 C-API.
        // Note that this function leaves alone any null terminator that is technically in the buffer
        // but outside .size() (where it is allowed to be!)
        if(str.empty()) return {};
        std::string tmp(str);
        size_t      start_pos = 0;
        while((start_pos = tmp.find('\0', start_pos)) != std::string::npos) {
            tmp.replace(start_pos, 1, "");
            start_pos += 1;
        }
        return tmp;
    }

    [[nodiscard]] inline std::string_view getParentPath(std::string_view linkPath) {
        size_t pos = linkPath.find_last_of('/');
        if(pos == std::string_view::npos) pos = 0; // No parent path
        return linkPath.substr(0, pos);
    }

    template<typename T>
    [[nodiscard]] bool should_track_vlen_reclaims(const hid::h5t &h5type, const PropertyLists &plists) {
        if(not plists.vlenTrackReclaims) return false;
        if constexpr(type::sfinae::has_vlen_type_v<T>) {
            // In this case, the user has put "using vlen_type = ..." into this data
            // type. This signals to h5pp that this datatype is using one or
            // more h5pp::vstr_t or h5pp::varr_t types, that manage their own
            // memory. Therefore, h5pp doesn't need to reclaim any memory
            // manually later.
            return false;
        } else {
            // In this case we need to do more work to detect whether the type has a variable-length arrays
            // and if so, if they are self-managing h5pp::vstr_t types or h5pp::varr_t types.
            // Remember that the type T could be std::vector<std::byte> during table row transfers
            htri_t h5type_has_vlen = H5Tdetect_class(h5type, H5T_class_t::H5T_VLEN);
            htri_t h5type_is_vstr  = H5Tis_variable_str(h5type);
            bool   is_or_has_varr  = type::sfinae::is_or_has_varr_v<T>;
            bool   is_or_has_vstr  = type::sfinae::is_or_has_vstr_v<T>;
            bool   track_vlen      = h5type_has_vlen and not is_or_has_varr;
            bool   track_vstr      = h5type_is_vstr and not is_or_has_vstr;
            return track_vlen or track_vstr;
        }
    }

    /*! \brief Calculates the python-style negative index. For instance, if num == -1ul and piv == 5ul, this returns 4ul */
    template<typename T>
    [[nodiscard]] T wrapUnsigned(T num, T piv) noexcept {
        static_assert(std::is_unsigned_v<T>);
        if(num >= piv) {
            if(num == std::numeric_limits<T>::max() and piv == std::numeric_limits<T>::min())
                return ~num;                                                      // The last element would be 0 if piv == 0
            if(num >= std::numeric_limits<T>::max() - piv) return piv - ~num - 1; // Rotate around the pivot
            if(num >= std::numeric_limits<std::uint64_t>::max() - piv)
                return static_cast<T>(wrapUnsigned(static_cast<std::uint64_t>(num), static_cast<std::uint64_t>(piv)));
            if(num >= std::numeric_limits<std::uint32_t>::max() - piv)
                return static_cast<T>(wrapUnsigned(static_cast<std::uint32_t>(num), static_cast<std::uint32_t>(piv)));
        }
        return num;
    }

    template<typename PtrType, typename DataType>
    [[nodiscard]] inline PtrType getVoidPointer(DataType &data, size_t offset = 0) noexcept {
        // Get the memory address to a data buffer
        if constexpr(h5pp::type::sfinae::has_data_v<DataType>) return static_cast<PtrType>(data.data() + offset);
        else if constexpr(std::is_pointer_v<DataType> or std::is_array_v<DataType>) return static_cast<PtrType>(data + offset);
        else return static_cast<PtrType>(&data + offset);
    }

    /*! \brief Checks if multiple values are equal to each other
     *   \param args any number of values
     *   \return bool, true if all args are equal
     */
    template<typename First, typename... T>
    [[nodiscard]] bool all_equal(First &&first, T &&...t) noexcept {
        return ((first == t) && ...);
    }

    template<typename DataType, size_t size>
    [[nodiscard]] constexpr size_t getArraySize([[maybe_unused]] const DataType (&arr)[size],
                                                [[maybe_unused]] bool countChars = false) noexcept
    /*! Returns the size of a C-style array.
     */
    {
        if constexpr(h5pp::type::sfinae::is_text_v<DataType>) {
            // A C-style char array is a null-terminated array, that has size = characters + 1
            // Here we want to return the number of characters that can fit in the array,
            // and we are not interested in the number of characters currently there.
            // To reverse this behavior, use "countChars == true".
            // To avoid strnlen, use std::string_view constructor to count chars up to (but not including) '\0'.
            if(countChars) return std::min(std::string_view(arr).size(), size) + 1; // Add null-terminator
            else return std::max(std::string_view(arr).size(), size - 1) + 1;       // Include null terminator
        } else {
            return size;
        }
    }
    template<typename DataType, size_t rows, size_t cols>
    [[nodiscard]] constexpr std::array<size_t, 2> getArraySize([[maybe_unused]] const DataType (&arr)[rows][cols]) noexcept {
        return {rows, cols};
    }
    template<typename DataType, size_t rows, size_t cols, size_t depth>
    [[nodiscard]] constexpr std::array<size_t, 3> getArraySize([[maybe_unused]] const DataType (&arr)[rows][cols][depth]) noexcept {
        return {rows, cols, depth};
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] size_t getCharArraySize(const DataType &data, [[maybe_unused]] bool countChars = true) noexcept {
        static_assert(h5pp::type::sfinae::is_text_v<DataType>,
                      "Template function [h5pp::util::getCharArraySize(const DataType & data)] requires type DataType to be "
                      "a text-like type such as [std::string], [std::string_view], [char *] or have a .c_str() member function");
        // With this function we are interested in the number of chars currently in a given buffer,
        // including the null terminator.
        if constexpr(h5pp::type::sfinae::has_size_v<DataType>) return data.size() + 1;      // string and string_view have size without \0
        else if constexpr(std::is_array_v<DataType>) return getArraySize(data, countChars); // getarraysize includes nullterm already
        else if constexpr(h5pp::type::sfinae::has_c_str_v<DataType>) return strlen(data.c_str()) + 1; // strlen does not include \0
        else if constexpr(std::is_pointer_v<DataType>) return strlen(data) + 1;                       // strlen does not include \0
        else return 1;                                                                                // Probably a char?
    }

    template<typename DimType = std::initializer_list<hsize_t>>
    [[nodiscard]] std::vector<hsize_t> getDimVector(const DimType &dims) {
        static_assert(h5pp::type::sfinae::is_integral_iterable_or_num_v<DimType>,
                      "Template function [h5pp::util::getDimVector(const DimType & dims)] requires type DimType to be "
                      "an integral type e.g. [int,long,size_t...] or "
                      "an iterable container of integral types e.g. [std::vector<int>, std::initializer_list<size_t>] ...");
        if constexpr(std::is_same_v<DimType, std::vector<hsize_t>>) {
            return dims;
        } else if constexpr(h5pp::type::sfinae::is_iterable_v<DimType>) {
            std::vector<hsize_t> dimVec;
            std::copy(std::begin(dims), std::end(dims), std::back_inserter(dimVec));
            return dimVec;
        } else if constexpr(std::is_integral_v<DimType>) {
            return std::vector<hsize_t>{type::safe_cast<hsize_t>(dims)};
        } else {
            static_assert(h5pp::type::sfinae::invalid_type_v<DimType>,
                          "Template function [h5pp::util::getDimVector(const DimType & dims)] failed to statically detect "
                          "an invalid type for DimsType. Please submit a bug report.");
        }
    }

    template<typename DimType = std::initializer_list<hsize_t>>
    [[nodiscard]] std::optional<std::vector<hsize_t>> getOptionalDimVector(const DimType &dims) {
        static_assert(h5pp::type::sfinae::is_integral_iterable_num_or_nullopt_v<DimType>,
                      "Template function [h5pp::util::getOptionalDimVector(const DimType & dims)] requires type DimType to be "
                      "an std::nullopt, an integral type e.g. [int,long,size_t...] or "
                      "an iterable container of integral types e.g. [std::vector<int>, std::initializer_list<size_t>] ...");
        if constexpr(std::is_same_v<DimType, std::nullopt_t>) return dims;
        else return getDimVector(dims);
    }

    [[nodiscard]] inline hsize_t getSizeFromDimensions(const std::vector<hsize_t> &dims) {
        return std::accumulate(std::begin(dims), std::end(dims), type::safe_cast<hsize_t>(1), std::multiplies<>());
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] hsize_t getSize(const DataType &data) {
        /* clang-format off */
        // variable-length arrays and text such as std::string/char arrays are scalar,
        if constexpr(h5pp::type::sfinae::is_text_v<DataType> or
                     h5pp::type::sfinae::is_varr_v<DataType> or
                     h5pp::type::sfinae::is_vstr_v<DataType> or
                     h5pp::type::sfinae::is_fstr_v<DataType>)       return type::safe_cast<hsize_t>(1);
        else if constexpr(h5pp::type::sfinae::has_size_v<DataType>) return type::safe_cast<hsize_t>(data.size());
        else if constexpr(std::is_array_v<DataType>)                return type::safe_cast<hsize_t>(getArraySize(data));
        else if constexpr(std::is_pointer_v<DataType>)
            throw h5pp::runtime_error("Failed to read data size: Pointer data has no specified dimensions");
        else return 1; // All others can be "H5S_SCALAR" of size 1.
        /* clang-format on */
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] constexpr int getRank() {
        /* clang-format off */

        if constexpr(h5pp::type::sfinae::is_text_v<DataType> or
                     h5pp::type::sfinae::is_varr_v<DataType> or
                     h5pp::type::sfinae::is_vstr_v<DataType> or
                     h5pp::type::sfinae::is_fstr_v<DataType>
                     )               return 0;
#ifdef H5PP_USE_EIGEN3
        else if constexpr(h5pp::type::sfinae::is_eigen_tensor_v<DataType>)  return type::safe_cast<int>(DataType::NumIndices);
        else if constexpr(h5pp::type::sfinae::is_eigen_1d_v<DataType>)      return 1;
        else if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType>)   return 2;
#endif
        else if constexpr(h5pp::type::sfinae::has_NumIndices_v<DataType>)   return type::safe_cast<int>(DataType::NumIndices);
        else if constexpr(std::is_array_v<DataType>)                        return std::rank_v<DataType>;
        else if constexpr(h5pp::type::sfinae::has_size_v<DataType>)         return 1;
        else                                                                return 0;
        /* clang-format on */
    }

    [[nodiscard]] inline int getRankFromDimensions(const std::vector<hsize_t> &dims) { return type::safe_cast<int>(dims.size()); }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] std::vector<hsize_t> getDimensions(const DataType &data) {
        /* clang-format off */
        // Empty vector means H5S_SCALAR, size 1 and rank 0
        namespace sfn      = h5pp::type::sfinae;
        constexpr int rank = getRank<DataType>();
        if constexpr(rank == 0)                       return {};
        else if constexpr(std::is_array_v<DataType>)  return {getArraySize(data)};
        else if constexpr(sfn::has_dimensions_v<DataType>) {
            // We copy because the vectors may not be assignable or may not be implicitly convertible to hsize_t.
            std::vector<hsize_t> dims(std::begin(data.dimensions()), std::end(data.dimensions()));
            if(data.dimensions().size() != rank) throw h5pp::runtime_error("given dimensions do not match detected rank");
            if(dims.size() != rank) throw h5pp::runtime_error("copied dimensions do not match detected rank");
            return dims;
        } else if constexpr(sfn::has_size_v<DataType> and rank == 1) return {type::safe_cast<hsize_t>(data.size())};
#ifdef H5PP_USE_EIGEN3
        else if constexpr(sfn::is_eigen_tensor_v<DataType>) {
            if(data.dimensions().size() != rank) throw h5pp::runtime_error("given dimensions do not match detected rank");
            // We copy because the vectors may not be assignable or may not be implicitly convertible to hsize_t.
            return std::vector<hsize_t>(std::begin(data.dimensions()), std::end(data.dimensions()));
        } else if constexpr(sfn::is_eigen_dense_v<DataType>) return {type::safe_cast<hsize_t>(data.rows()), type::safe_cast<hsize_t>(data.cols())};
#endif
        else throw h5pp::runtime_error("getDimensions can't match the type provided [{}]", sfn::type_name<DataType>());
        /* clang-format on */
    }

    [[nodiscard]] inline std::optional<std::vector<hsize_t>> decideDimensionsMax(const std::vector<hsize_t> &dims,
                                                                                 std::optional<H5D_layout_t> h5_layout) {
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
        if(h5_layout and h5_layout.value() == H5D_CHUNKED) return std::vector<hsize_t>(dims.size(), H5S_UNLIMITED);
        else if(h5_layout and h5_layout.value() != H5D_CHUNKED) return std::nullopt;
        else return dims;
    }

    template<typename T = hsize_t>
    void ind2sub(const std::vector<T> &dims, size_t idx, std::vector<T> &coord) {
        static_assert(std::is_integral_v<T>);
        if(dims.size() != coord.size()) throw h5pp::runtime_error("dims.size [{}] != coord.size [{}]", dims.size(), coord.size());
        auto rank    = dims.size();
        auto dimprod = std::accumulate(dims.begin(), dims.end(), type::safe_cast<size_t>(1), std::multiplies<>());
        if(idx >= dimprod) throw h5pp::runtime_error("linearIndex {} out of range for dims with size {}", idx, dimprod);
        for(size_t i = rank - 1; i < rank; --i) {
            coord[i] = idx % dims[i];
            idx -= coord[i];
            idx /= dims[i];
        }
    }

    template<typename T = hsize_t>
    [[nodiscard]] std::vector<T> ind2sub(const std::vector<T> &dims, size_t idx) {
        static_assert(std::is_integral_v<T>);
        std::vector<T> coord(dims.size(), 0);
        ind2sub(dims, idx, coord);
        return coord;
    }

    [[nodiscard]] inline size_t sub2ind(const std::vector<hsize_t> &dims, const std::vector<hsize_t> &coords) {
        if(dims.size() != coords.size())
            throw h5pp::runtime_error("dims and coords do not have the same rank: {} != {}", dims.size(), coords.size());
        size_t  rank  = dims.size();
        hsize_t index = 0;
        hsize_t j     = 1; // current
        for(size_t i = 0; i < rank; i++) {
            if(coords[i] >= dims[i]) throw h5pp::runtime_error("coords out of bounds: coords {} |  dims {}", coords, dims);
            hsize_t dimprod = std::accumulate(dims.begin() + type::safe_cast<long>(j++), dims.end(), hsize_t(1), std::multiplies<>());
            index += dimprod * coords[i];
        }
        return type::safe_cast<size_t>(index);
    }

    [[nodiscard]] inline hid::h5s getDsetSpace(const hsize_t                       size,
                                               const std::vector<hsize_t>         &dims,
                                               const H5D_layout_t                 &h5Layout,
                                               std::optional<std::vector<hsize_t>> dimsMax = std::nullopt) {
        if(dims.empty() and size > 0) {
            return H5Screate(H5S_SCALAR);
        } else if(dims.empty() and size == 0) {
            return H5Screate(H5S_NULL);
        } else {
            auto num_elements = h5pp::util::getSizeFromDimensions(dims);
            if(size != num_elements) throw h5pp::runtime_error("Number of elements mismatch: size {} | dimensions {}", size, dims);

            // Only and chunked and datasets can be extended. The extension can happen up to the max dimension specified.
            // If the max dimension is H5S_UNLIMITED, then the dataset can grow to any dimension.
            // Conversely, if the dataset is not H5D_CHUNKED, then dims == max dims must hold always

            if(dimsMax) {
                // Here dimsMax was given by the user, and we have to do some sanity checks
                // Check that the ranks match
                if(dimsMax and dimsMax->size() != dims.size()) {
                    throw h5pp::runtime_error("Number of dimensions (rank) mismatch: dims {} | max dims {}\n"
                                              "\t Hint: Dimension lists must have the same number of elements",
                                              dims,
                                              dimsMax.value());
                }
                // Check that H5S_UNLIMITED is only given to H5D_CHUNKED datasets
                for(size_t idx = 0; idx < dimsMax->size(); idx++) {
                    if(dimsMax.value()[idx] == H5S_UNLIMITED and h5Layout != H5D_CHUNKED) {
                        throw h5pp::runtime_error(
                            "Max dimensions {} has an H5S_UNLIMITED dimension at index {}. This requires H5D_CHUNKED layout",
                            dimsMax.value(),
                            idx);
                    }
                }

                if(dimsMax.value() != dims) {
                    // Only H5D_CHUNKED layout can have since dimsMax != dims.
                    // Therefore, give an informative error if not H5D_CHUNKED
                    if(h5Layout == H5D_COMPACT) {
                        throw h5pp::runtime_error("Dimension mismatch: dims {} != max dims {}. Equality is required for H5D_COMPACT layout",
                                                  dims,
                                                  dimsMax.value());
                    }
                    if(h5Layout == H5D_CONTIGUOUS) {
                        throw h5pp::runtime_error(
                            "Dimension mismatch: dims {} != max dims {}. Equality is required for H5D_CONTIGUOUS layout",
                            dims,
                            dimsMax.value());
                    }
                }
            } else if(h5Layout == H5D_CHUNKED) {
                // Here dimsMax was not given, but H5D_CHUNKED was asked for
                dimsMax = dims;
                std::fill_n(dimsMax->begin(), dims.size(), H5S_UNLIMITED);
            } else {
                dimsMax = dims;
            }

            return H5Screate_simple(type::safe_cast<int>(dims.size()), dims.data(), dimsMax->data());
        }
    }

    /*
     * memspace is a description of the buffer in memory (i.e. where read elements will go).
     * If there is no data conversion, then data is read directly into the user supplied buffer.
     * If there is data conversion, HDF5 uses a 1 MB buffer to do the conversions,
     * but we still use the user's buffer for reading data in the first place.
     * Also, you can adjust the 1 MB default conversion buffer size. (see H5Pset_buffer)
     */
    [[nodiscard]] inline hid::h5s getMemSpace(const hsize_t size, const std::vector<hsize_t> &dims) {
        if(dims.empty() and size > 0) {
            return H5Screate(H5S_SCALAR);
        } else if(dims.empty() and size == 0) {
            return H5Screate(H5S_NULL);
        } else {
            auto num_elements = getSizeFromDimensions(dims);
            if(size != num_elements) throw h5pp::runtime_error("Number of elements mismatch: size {} | dimensions {}", size, dims);
            return H5Screate_simple(type::safe_cast<int>(dims.size()), dims.data(), nullptr);
        }
    }

    template<typename DataType, size_t depth = 0>
    [[nodiscard]] constexpr size_t getBytesPerElem() {
        /* clang-format off */
        static_assert(not type::sfinae::is_h5pp_id<DataType>);
        namespace sfn   = h5pp::type::sfinae;
        using DecayType = typename std::decay<DataType>::type;
        if constexpr(std::is_pointer_v<DecayType>)                         return getBytesPerElem<typename std::remove_pointer<DecayType>::type, depth+1>();
        else if constexpr(std::is_reference_v<DecayType>)                  return getBytesPerElem<typename std::remove_reference<DecayType>::type, depth+1>();
        else if constexpr(std::is_array_v<DecayType> and depth == 0)       return getBytesPerElem<typename std::remove_all_extents<DecayType>::type,  depth+1>();
        else if constexpr(sfn::is_std_complex_v<DecayType>)                return sizeof(DecayType);
        else if constexpr(sfn::is_ScalarN_v<DecayType>)                    return sizeof(DecayType);
        else if constexpr(std::is_arithmetic_v<DecayType>)                 return sizeof(DecayType);
        else if constexpr(sfn::has_Scalar_v<DecayType> )                   return sizeof(typename DecayType::Scalar);
        else if constexpr(sfn::has_value_type_v<DecayType> and depth == 0) return getBytesPerElem<typename DecayType::value_type, depth+1>();
        else                                                return sizeof(std::remove_all_extents_t<DecayType>);
        /* clang-format on */
    }

    template<typename DataType, typename = std::enable_if_t<not std::is_base_of_v<hid::hid_base<DataType>, DataType>>>
    [[nodiscard]] size_t getBytesTotal(const DataType &data, std::optional<size_t> size = std::nullopt) {
        if constexpr(h5pp::type::sfinae::is_or_has_varr_v<DataType> or h5pp::type::sfinae::is_or_has_vstr_v<DataType>) {
            return h5pp::util::getSize(data) * h5pp::util::getBytesPerElem<DataType>();
        } else if constexpr(h5pp::type::sfinae::is_iterable_v<DataType> and h5pp::type::sfinae::has_value_type_v<DataType>) {
            using value_type = typename DataType::value_type;
            if constexpr(h5pp::type::sfinae::has_size_v<value_type>) { // E.g. std::vector<std::string>
                // Count all the null terminators
                size_t num      = 0;
                size_t nullterm = h5pp::type::sfinae::is_text_v<value_type> ? 1 : 0;
                for(auto &elem : data) num += elem.size() + nullterm;
                return num * h5pp::util::getBytesPerElem<value_type>();
            } else if constexpr(h5pp::type::sfinae::has_size_v<DataType>) { // E.g. std::string or std::vector<double>
                size_t num      = 0;
                size_t nullterm = h5pp::type::sfinae::is_text_v<DataType> ? 1 : 0;
                num             = type::safe_cast<size_t>(data.size()) + nullterm; // Static cast because Eigen uses long
                return num * h5pp::util::getBytesPerElem<value_type>();
            }
        } else if constexpr(h5pp::type::sfinae::is_text_v<DataType>) { // E.g. char[n]
            if(not size) size = getCharArraySize(data) + 1;            // Add null terminator
            return size.value() * h5pp::util::getBytesPerElem<DataType>();
        } else if constexpr(std::is_array_v<DataType>) {
            if(not size) size = h5pp::util::getArraySize(data);
            return size.value() * h5pp::util::getBytesPerElem<DataType>();
        } else if constexpr(std::is_pointer_v<DataType>) {
            if(not size)
                throw h5pp::runtime_error("Could not determine total amount of bytes in buffer: Pointer data has no specified size");
            return size.value() * h5pp::util::getBytesPerElem<DataType>();
        } else {
            if(not size) size = h5pp::util::getSize(data);
            return size.value() * h5pp::util::getBytesPerElem<DataType>();
        }
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
        // Otherwise, we decide based on size
        if(totalBytes < h5pp::constants::maxSizeCompact) {
            h5pp::logger::log->trace("Selected layout H5D_COMPACT because byte size {} < {}", totalBytes, h5pp::constants::maxSizeCompact);
            return H5D_COMPACT;
        } else if(totalBytes < h5pp::constants::maxSizeContiguous) {
            h5pp::logger::log->trace("Selected layout H5D_CONTIGUOUS because byte size {} is between {} and {}",
                                     totalBytes,
                                     h5pp::constants::maxSizeCompact,
                                     h5pp::constants::maxSizeContiguous);
            return H5D_CONTIGUOUS;
        } else {
            h5pp::logger::log->trace("Selected layout H5D_CHUNKED because byte size {} > {}",
                                     totalBytes,
                                     h5pp::constants::maxSizeContiguous);
            return H5D_CHUNKED;
        }
    }

    template<typename DataType>
    [[nodiscard]] inline H5D_layout_t
        decideLayout(const DataType &data, std::optional<std::vector<hsize_t>> dsetDims, std::optional<std::vector<hsize_t>> dsetDimsMax) {
        // Here we try to compute the maximum number of bytes of this dataset, so we can decide its layout
        hsize_t size = 0;
        if(dsetDimsMax) {
            if(dsetDims and dsetDims.value() != dsetDimsMax.value()) return H5D_CHUNKED;
            for(auto &dim : dsetDimsMax.value())
                if(dim == H5S_UNLIMITED) return H5D_CHUNKED;
            size = getSizeFromDimensions(dsetDimsMax.value());
        } else if(dsetDims) {
            size = getSizeFromDimensions(dsetDims.value());
        } else {
            size = getSize(data);
        }
        auto bytes = size * getBytesPerElem<DataType>();
        return decideLayout(bytes);
    }

    [[nodiscard]] inline std::optional<std::vector<hsize_t>> getChunkDimensions(size_t                              bytesPerElem,
                                                                                const std::vector<hsize_t>         &dims,
                                                                                std::optional<std::vector<hsize_t>> dimsMax,
                                                                                std::optional<H5D_layout_t>         layout) {
        // Here we make a naive guess for chunk dimensions
        // We try to make a square in N dimensions with a target byte size of 10 kb - 1 MB.
        // Here is a great read for chunking considerations https://www.oreilly.com/library/view/python-and-hdf5/9781491944981/ch04.html
        // Hard rules for chunk dimensions:
        //  * A chunk dimension cannot be larger than the corresponding max dimension
        //  * A chunk dimension can be larger or smaller than the dataset dimension

        if(layout and layout.value() != H5D_CHUNKED) return std::nullopt;
        if(dims.empty()) return dims; // Scalar
        // We try to compute the maximum dimension to get a good estimate
        // of the data volume
        std::vector<hsize_t> dims_effective = dims;
        // Make sure the chunk dimensions have strictly nonzero volume
        for(auto &dim : dims_effective) dim = std::max<hsize_t>(1, dim);
        if(dimsMax) {
            // If max dims are given, dims that are not H5S_UNLIMITED are used as an upper bound
            // for that dimension
            if(dimsMax->size() != dims.size()) {
                throw h5pp::runtime_error("Could not get chunk dimensions: "
                                          "dims {} and max dims {} have different number of elements",
                                          dims,
                                          dimsMax.value());
            }
            for(size_t idx = 0; idx < dims.size(); idx++) {
                if(dimsMax.value()[idx] < dims[idx]) {
                    throw h5pp::runtime_error("Could not get chunk dimensions: "
                                              "Some elements in dims exceed max dims: "
                                              "dims {} | max dims {}",
                                              dims,
                                              dimsMax.value());
                }
                if(dimsMax.value()[idx] != H5S_UNLIMITED) dims_effective[idx] = std::max(dims_effective[idx], dimsMax.value()[idx]);
            }
        }

        auto rank             = dims.size();
        auto maxDimension     = *std::max_element(dims_effective.begin(), dims_effective.end());
        auto volumeChunkBytes = std::pow(maxDimension, rank) * static_cast<double>(bytesPerElem);
        auto targetChunkBytes = std::clamp<double>(volumeChunkBytes,
                                                   std::max<double>(static_cast<double>(bytesPerElem), h5pp::constants::minChunkBytes),
                                                   std::max<double>(static_cast<double>(bytesPerElem), h5pp::constants::maxChunkBytes));
        targetChunkBytes      = std::pow(2, std::ceil(std::log2(targetChunkBytes)));         // Next nearest power of two
        auto linearChunkSize  = std::ceil(std::pow(targetChunkBytes / static_cast<double>(bytesPerElem), 1.0 / static_cast<double>(rank)));
        auto chunkSize        = std::max<hsize_t>(1, static_cast<hsize_t>(linearChunkSize)); // Make sure the chunk size is positive
        std::vector<hsize_t> chunkDims(rank, chunkSize);
        // Now effective dims contains either dims or dimsMax (if not H5S_UNLIMITED) at each position.
        for(size_t idx = 0; idx < chunkDims.size(); idx++)
            if(dimsMax.has_value() and dimsMax.value()[idx] != H5S_UNLIMITED) chunkDims[idx] = std::min(dimsMax.value()[idx], chunkSize);
        h5pp::logger::log->debug("Estimated reasonable chunk dimensions: {}", chunkDims);
        return chunkDims;
    }

    template<typename DataType>
    inline void setStringSize(const DataType &data, hsize_t &size, size_t &bytes, std::vector<hsize_t> &dims) {
        // Case 1: data is actual text, such as char* or std::string
        // Case 1A: No dimensions were given, so the dataset is scalar (no extents)
        // Case 1B: Dimensions were given. Force into scalar but only take as many bytes
        //          as implied from given dimensions
        // Case 2: data is a container of strings such as std::vector<std::string>
        if constexpr(h5pp::type::sfinae::is_text_v<DataType>) {
            if(dims.empty()) {
                // Case 1A
                bytes = h5pp::util::getBytesTotal(data);
            } else {
                // Case 1B
                hsize_t desiredSize = h5pp::util::getSizeFromDimensions(dims);
                dims                = {};
                size                = 1;
                bytes               = desiredSize * h5pp::util::getBytesPerElem<DataType>();
            }
        } else if constexpr(h5pp::type::sfinae::has_text_v<DataType>) {
            // Case 2
            bytes = h5pp::util::getBytesTotal(data);
        }
    }

    template<typename DataType>
    inline void resizeData(DataType &data, const std::vector<hsize_t> &newDims) {
        // This function may shrink a container!
#ifdef H5PP_USE_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType> and h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            auto newSize = getSizeFromDimensions(newDims);
            if(newDims.size() != 1) {
                h5pp::logger::log->debug("Resizing given 1-dimensional Eigen type [{}] to fit dataset dimensions {}",
                                         type::sfinae::type_name<DataType>(),
                                         newDims);
            }
            h5pp::logger::log->debug("Resizing eigen 1d container {} -> {}",
                                     std::initializer_list<Eigen::Index>{data.size()},
                                     std::initializer_list<hsize_t>{newSize});
            data.resize(type::safe_cast<Eigen::Index>(newSize));
        } else if constexpr(h5pp::type::sfinae::is_eigen_dense_v<DataType> and not h5pp::type::sfinae::is_eigen_1d_v<DataType>) {
            if(newDims.size() != 2)
                throw h5pp::runtime_error("Failed to resize 2-dimensional Eigen type: Dataset has dimensions {}", newDims);
            h5pp::logger::log->debug("Resizing eigen 2d container {} -> {}",
                                     std::initializer_list<Eigen::Index>{data.rows(), data.cols()},
                                     newDims);
            data.resize(type::safe_cast<Eigen::Index>(newDims[0]), type::safe_cast<Eigen::Index>(newDims[1]));
        } else if constexpr(h5pp::type::sfinae::is_eigen_tensor_v<DataType>) {
            if constexpr(h5pp::type::sfinae::has_resize_v<DataType>) {
                if(newDims.size() != DataType::NumDimensions) {
                    throw h5pp::runtime_error("Failed to resize {}-dimensional Eigen tensor: Dataset has dimensions {}",
                                              DataType::NumDimensions,
                                              newDims);
                }
                auto eigenDims = eigen::copy_dims<DataType::NumDimensions>(newDims);
                h5pp::logger::log->debug("Resizing eigen tensor container {} -> {}", data.dimensions(), newDims);
                data.resize(eigenDims);
            } else {
                auto newSize = getSizeFromDimensions(newDims);
                if(data.size() != type::safe_cast<Eigen::Index>(newSize)) {
                    h5pp::logger::log->warn("Detected non-resizeable tensor container with wrong size: Given size {}. Required size {}",
                                            data.size(),
                                            newSize);
                }
            }
        } else
#endif // H5PP_USE_EIGEN3
            if constexpr(h5pp::type::sfinae::has_size_v<DataType> and h5pp::type::sfinae::has_resize_v<DataType>) {
                if(newDims.size() > 1) {
                    h5pp::logger::log->debug(
                        "Given data container is 1-dimensional but the desired dimensions are {}. Resizing to fit all the data",
                        newDims);
                }
                auto newSize = getSizeFromDimensions(newDims);
                h5pp::logger::log->debug("Resizing 1d container {} -> {} of type [{}]",
                                         std::initializer_list<size_t>{type::safe_cast<size_t>(data.size())},
                                         newDims,
                                         h5pp::type::sfinae::type_name<DataType>());
                data.resize(newSize);
            } else if constexpr(std::is_scalar_v<DataType> or std::is_class_v<DataType>) {
                return;
            } else {
                h5pp::logger::log->debug("Container could not be resized");
            }
    }

    template<typename h5xa,
             typename h5xb,
             // enable_if so the compiler doesn't think it can use overload with fs::path those arguments
             typename = std::enable_if_t<type::sfinae::is_hdf5_loc_id<h5xa>>,
             typename = std::enable_if_t<type::sfinae::is_hdf5_loc_id<h5xb>>>
    [[nodiscard]] inline bool onSameFile(const h5xa &loca, const h5xb &locb, LocationMode locMode = LocationMode::DETECT) {
        switch(locMode) {
            case LocationMode::SAME_FILE: return true;
            case LocationMode::OTHER_FILE: return false;
            case LocationMode::DETECT: {
                hid::h5f filea;
                hid::h5f fileb;
                if constexpr(std::is_same_v<h5xa, hid::h5f>) filea = loca;
                else filea = H5Iget_file_id(loca);
                if constexpr(std::is_same_v<h5xb, hid::h5f>) fileb = locb;
                else fileb = H5Iget_file_id(locb);
                return filea == fileb;
            }
            default: throw h5pp::runtime_error("Unhandled switch case for locMode");
        }
    }

    [[nodiscard]] inline bool
        onSameFile(const h5pp::fs::path &patha, const h5pp::fs::path &pathb, LocationMode locMode = LocationMode::DETECT) {
        switch(locMode) {
            case LocationMode::SAME_FILE: return true;
            case LocationMode::OTHER_FILE: return false;
            case LocationMode::DETECT: {
                return h5pp::fs::equivalent(patha, pathb);
            }
        }
    }

    [[nodiscard]] inline LocationMode getLocationMode(const h5pp::fs::path &patha, const h5pp::fs::path &pathb) {
        if(h5pp::fs::equivalent(patha, pathb)) return LocationMode::SAME_FILE;
        else return LocationMode::OTHER_FILE;
    }

    inline std::vector<size_t> getFieldIndices(const TableInfo &info, const std::vector<std::string> &fieldNames) {
        // Compute the field indices
        info.assertReadReady();
        std::vector<size_t> fieldIndices;
        for(const auto &fieldName : fieldNames) {
            auto it = std::find(info.fieldNames->begin(), info.fieldNames->end(), fieldName);
            if(it == info.fieldNames->end()) {
                throw h5pp::runtime_error("getFieldIndices: could not find field [{}] in table [{}]: \n"
                                          "Available field names are \n{}",
                                          fieldName,
                                          info.tablePath.value(),
                                          info.fieldNames.value());
            } else {
                fieldIndices.emplace_back(type::safe_cast<size_t>(std::distance(info.fieldNames->begin(), it)));
            }
        }
        return fieldIndices;
    }

    inline hid::h5t getFieldTypeId(const TableInfo &info, const std::vector<size_t> &fieldIndices) {
        // Build the field sizes and offsets of the given read buffer based on the corresponding quantities on file
        info.assertReadReady();
        size_t                   tgtFieldSizeSum = 0;
        std::vector<size_t>      srcFieldOffsets; // Offsets on file
        std::vector<size_t>      tgtFieldOffsets; // Offsets on new subset type
        std::vector<size_t>      tgtFieldSizes;
        std::vector<std::string> tgtFieldNames;
        srcFieldOffsets.reserve(fieldIndices.size());
        tgtFieldOffsets.reserve(fieldIndices.size());
        tgtFieldSizes.reserve(fieldIndices.size());
        tgtFieldNames.reserve(fieldIndices.size());

        for(const auto &idx : fieldIndices) {
            srcFieldOffsets.emplace_back(info.fieldOffsets.value()[idx]);
            tgtFieldOffsets.emplace_back(tgtFieldSizeSum);
            tgtFieldSizes.emplace_back(info.fieldSizes.value()[idx]);
            tgtFieldNames.emplace_back(info.fieldNames.value()[idx]);
            tgtFieldSizeSum += tgtFieldSizes.back();
        }

        /* Create a special tgtTypeId for reading a subset of a table record with the following properties:
         *      - tgtTypeId has the size of the given field selection i.e. fieldSizeSum.
         *      - only the fields to read are defined in it
         *      - the corresponding field types are copied from info.fieldTypes.
         *      Then H5Dread will take care of only reading the relevant fields of the record.
         *      H5Dread will find the correct fields by comparing the field names.
         */
        hid::h5t typeId = H5Tcreate(H5T_COMPOUND, tgtFieldSizeSum);
        for(size_t tgtIdx = 0; tgtIdx < fieldIndices.size(); tgtIdx++) {
            size_t srcIdx = fieldIndices.at(tgtIdx);
            H5Tinsert(typeId, tgtFieldNames.at(tgtIdx).c_str(), tgtFieldOffsets.at(tgtIdx), info.fieldTypes->at(srcIdx));
        }
        return typeId;
    }

    inline hid::h5t getFieldTypeId(const TableInfo &info, const std::vector<std::string> &fieldNames) {
        return getFieldTypeId(info, getFieldIndices(info, fieldNames));
    }

    inline std::vector<std::string> getFieldNames(const hid::h5t &fieldId) {
        H5T_class_t h5tclass = H5Tget_class(fieldId);
        if(h5tclass != H5T_COMPOUND) throw h5pp::runtime_error("fieldId for reading table fields must be H5T_COMPOUND");
        int nmembers = H5Tget_nmembers(fieldId);
        if(nmembers < 0) throw h5pp::runtime_error("Failed to read nmembers for fieldId");
        auto                     nmembers_ul = type::safe_cast<size_t>(nmembers);
        std::vector<std::string> fieldNames;
        fieldNames.reserve(nmembers_ul);
        for(size_t idx = 0; idx < nmembers_ul; idx++) {
            char *name = H5Tget_member_name(fieldId, type::safe_cast<unsigned int>(idx));
            fieldNames.emplace_back(name);
            H5free_memory(name);
        }
        return fieldNames;
    }

    /*! \brief Use to parse the table selection into offset,extent pair
     */
    template<typename DataType>
    inline std::pair<size_t, size_t> parseTableSelection(DataType             &data,
                                                         TableSelection       &selection,
                                                         std::optional<size_t> numRecords,
                                                         std::optional<size_t> recordBytes) {
        if(not numRecords) throw h5pp::runtime_error("parseTableSelection: undefined table field [numRecords]");
        if(not recordBytes) throw h5pp::runtime_error("parseTableSelection: undefined table field [recordBytes]");
        // Used when reading from file into data
        size_t      offset = 0;
        size_t      extent = 1;
        std::string select;
        switch(selection) {
            case h5pp::TableSelection::ALL: {
                offset = 0;
                extent = numRecords.value();
                select = "ALL";
                break;
            }

            case h5pp::TableSelection::FIRST: {
                offset = 0;
                extent = 1;
                select = "FIRST";
                break;
            }
            case h5pp::TableSelection::LAST: {
                offset = numRecords.value() - 1;
                extent = 1;
                select = "LAST";
                break;
            }
        }

        if constexpr(not type::sfinae::has_resize_v<DataType>) {
            // The given buffer is not resizeable. Make sure it can handle extent
            auto dataSize = util::getBytesTotal(data);
            if(dataSize < extent * recordBytes.value()) {
                throw h5pp::runtime_error("Given buffer [{}] can't fit table selection {}:\n"
                                          " offset               : {}\n"
                                          " extent               : {}\n"
                                          " bytes per record     : {}\n"
                                          " bytes to read        : {}\n"
                                          " size of buffer       : {}\n"
                                          " buffer has .resize() : {}",
                                          type::sfinae::type_name<DataType>(),
                                          select,
                                          offset,
                                          extent,
                                          recordBytes.value(),
                                          extent * recordBytes.value(),
                                          dataSize,
                                          type::sfinae::has_resize_v<DataType>);
            }
        }
        return {offset, extent};
    }

    template<typename DataType>
    inline std::pair<size_t, size_t>
        parseTableSelection(DataType &data, TableSelection &selection, const std::vector<size_t> &fieldIndices, const TableInfo &info) {
        info.assertReadReady();
        // Used when reading from file into data
        size_t      offset = 0;
        size_t      extent = 1;
        std::string select;
        switch(selection) {
            case h5pp::TableSelection::ALL: {
                offset = 0;
                extent = info.numRecords.value();
                select = "ALL";
                break;
            }
            case h5pp::TableSelection::FIRST: {
                offset = 0;
                extent = 1;
                select = "FIRST";
                break;
            }
            case h5pp::TableSelection::LAST: {
                offset = info.numRecords.value() - 1;
                extent = 1;
                select = "LAST";
                break;
            }
        }

        std::vector<size_t> fieldSizes;
        fieldSizes.reserve(fieldIndices.size());
        for(const auto &idx : fieldIndices) fieldSizes.emplace_back(info.fieldSizes.value().at(idx));
        size_t fieldSizeTotal = std::accumulate(fieldSizes.begin(), fieldSizes.end(), type::safe_cast<size_t>(0));

        if constexpr(not type::sfinae::has_resize_v<DataType>) {
            // The given buffer is not resizeable. Make sure it can handle extent
            std::vector<std::string> fieldNames;
            fieldNames.reserve(fieldIndices.size());
            for(const auto &idx : fieldIndices) fieldNames.emplace_back(info.fieldNames.value().at(idx));

            auto dataSize = util::getBytesTotal(data);
            if(dataSize < extent * fieldSizeTotal) {
                throw h5pp::runtime_error("Given buffer [{}] can't fit table selection {}:\n"
                                          " offset               : {}\n"
                                          " extent               : {}\n"
                                          " field names          : {}\n"
                                          " field sizes          : {}\n"
                                          " bytes per record     : {}\n"
                                          " bytes to read        : {}\n"
                                          " size of buffer       : {}\n"
                                          " buffer has .resize() : {}",
                                          type::sfinae::type_name<DataType>(),
                                          select,
                                          offset,
                                          extent,
                                          fieldNames,
                                          fieldSizes,
                                          fieldSizeTotal,
                                          extent * fieldSizeTotal,
                                          dataSize,
                                          type::sfinae::has_resize_v<DataType>);
            }
        }
        return {offset, extent};
    }

    template<typename DataType>
    inline std::pair<size_t, size_t>
        parseTableSelection(DataType &data, TableSelection &selection, const std::vector<std::string> &fieldNames, const TableInfo &info) {
        return parseTableSelection(data, selection, getFieldIndices(info, fieldNames), info);
    }

    template<typename DataType>
    inline std::pair<size_t, size_t>
        parseTableSelection(DataType &data, TableSelection &selection, const hid::h5t &fieldId, const TableInfo &info) {
        return parseTableSelection(data, selection, getFieldNames(fieldId), info);
    }

    inline std::pair<size_t, size_t> parseTableSelection(TableSelection &selection, std::optional<size_t> numRecords) {
        if(not numRecords) throw h5pp::runtime_error("parseTableSelection: undefined table field [numRecords]");
        // Used when reading from file into data
        size_t offset = 0;
        size_t extent = 1;
        switch(selection) {
            case h5pp::TableSelection::ALL: {
                offset = 0;
                extent = numRecords.value();
                break;
            }

            case h5pp::TableSelection::FIRST: {
                offset = 0;
                extent = 1;
                break;
            }
            case h5pp::TableSelection::LAST: {
                offset = numRecords.value() - 1;
                extent = 1;
                break;
            }
        }
        return {offset, extent};
    }
}
