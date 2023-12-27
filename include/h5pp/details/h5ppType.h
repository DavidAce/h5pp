#pragma once
#include "h5ppEnums.h"
#include "h5ppError.h"
#include "h5ppHid.h"
#include "h5ppTypeCast.h"
#include "h5ppTypeCompound.h"
#include "h5ppTypeCustom.h"
#include "h5ppTypeSfinae.h"
#include "h5ppVarr.h"
#include "h5ppVstr.h"
#include <cstddef>
#include <H5Tpublic.h>
#include <H5version.h>
#include <typeindex>

namespace tc = h5pp::type::sfinae;
namespace h5pp::type {

    [[nodiscard]] inline std::string getH5ClassName(H5T_class_t h5class) {
        switch(h5class) {
            case H5T_class_t::H5T_INTEGER: return "H5T_INTEGER";
            case H5T_class_t::H5T_FLOAT: return "H5T_FLOAT";
            case H5T_class_t::H5T_TIME: return "H5T_TIME";
            case H5T_class_t::H5T_STRING: return "H5T_STRING";
            case H5T_class_t::H5T_BITFIELD: return "H5T_BITFIELD";
            case H5T_class_t::H5T_OPAQUE: return "H5T_OPAQUE";
            case H5T_class_t::H5T_COMPOUND: return "H5T_COMPOUND";
            case H5T_class_t::H5T_REFERENCE: return "H5T_REFERENCE";
            case H5T_class_t::H5T_ENUM: return "H5T_ENUM";
            case H5T_class_t::H5T_VLEN: return "H5T_VLEN";
            case H5T_class_t::H5T_ARRAY: return "H5T_ARRAY";
            default: return "H5T_UNKNOWN_CLASS";
        }
    }

    [[nodiscard]] inline std::string getH5ClassName(const hid::h5t &h5type) { return getH5ClassName(H5Tget_class(h5type)); }

    [[nodiscard]] inline std::string getH5TypeName(const hid::h5t &h5type) {
        /* clang-format off */
        if(H5Tequal(h5type, H5T_NATIVE_SHORT))             return "H5T_NATIVE_SHORT";
        if(H5Tequal(h5type, H5T_NATIVE_INT))               return "H5T_NATIVE_INT";
        if(H5Tequal(h5type, H5T_NATIVE_LONG))              return "H5T_NATIVE_LONG";
        if(H5Tequal(h5type, H5T_NATIVE_LLONG))             return "H5T_NATIVE_LLONG";
        if(H5Tequal(h5type, H5T_NATIVE_USHORT))            return "H5T_NATIVE_USHORT";
        if(H5Tequal(h5type, H5T_NATIVE_UINT))              return "H5T_NATIVE_UINT";
        if(H5Tequal(h5type, H5T_NATIVE_ULONG))             return "H5T_NATIVE_ULONG";
        if(H5Tequal(h5type, H5T_NATIVE_ULLONG))            return "H5T_NATIVE_ULLONG";
        if(H5Tequal(h5type, H5T_NATIVE_FLOAT))             return "H5T_NATIVE_FLOAT";
        if(H5Tequal(h5type, H5T_NATIVE_DOUBLE))            return "H5T_NATIVE_DOUBLE";
        if(H5Tequal(h5type, H5T_NATIVE_LDOUBLE))           return "H5T_NATIVE_LDOUBLE";
        if(H5Tequal(h5type, H5T_NATIVE_INT8))              return "H5T_NATIVE_INT8";
        if(H5Tequal(h5type, H5T_NATIVE_INT16))             return "H5T_NATIVE_INT16";
        if(H5Tequal(h5type, H5T_NATIVE_INT32))             return "H5T_NATIVE_INT32";
        if(H5Tequal(h5type, H5T_NATIVE_INT64))             return "H5T_NATIVE_INT64";
        if(H5Tequal(h5type, H5T_NATIVE_UINT8))             return "H5T_NATIVE_UINT8";
        if(H5Tequal(h5type, H5T_NATIVE_UINT16))            return "H5T_NATIVE_UINT16";
        if(H5Tequal(h5type, H5T_NATIVE_UINT32))            return "H5T_NATIVE_UINT32";
        if(H5Tequal(h5type, H5T_NATIVE_UINT64))            return "H5T_NATIVE_UINT64";
        if(H5Tequal(h5type, H5T_NATIVE_UINT8))             return "H5T_NATIVE_UINT8";
        if(H5Tequal(h5type, H5T_C_S1))                     return "H5T_C_S1";
#if defined(H5PP_USE_FLOAT128)
        if(type::custom::H5T_FLOAT<__float128>::equal(h5type)) return type::custom::H5T_FLOAT<__float128>::h5name();
#endif
        if(H5Tget_class(h5type) == H5T_class_t::H5T_ENUM)      return h5pp::format("H5T_ENUM{}",H5Tget_size(h5type));
        if(H5Tget_class(h5type) == H5T_class_t::H5T_ARRAY)     return fmt::format("H5T_ARRAY<{}>", getH5TypeName(H5Tget_super(h5type)));
        if(H5Tis_variable_str(h5type))                         return "H5T_VARIABLE";
        if(H5Tcommitted(h5type) == 0){
            hid::h5t h5native = H5Tget_native_type(h5type, H5T_direction_t::H5T_DIR_DEFAULT);
            if(H5Tequal(h5native, h5type) == 0)
                return fmt::format("{}<{}>",getH5ClassName(h5type), getH5TypeName(h5native));
            // Just get the class and size...
            return h5pp::format("{}:{}",getH5ClassName(h5type), H5Tget_size(h5type));
        }
        /* clang-format on */
        // Read about the buffer size inconsistency here
        // http://hdf-forum.184993.n3.nabble.com/H5Iget-name-inconsistency-td193143.html
        std::string buf;
        ssize_t     bufSize = H5Iget_name(h5type, nullptr, 0); // Size in bytes of the object name (NOT including \0)
        if(bufSize < 0) throw h5pp::runtime_error("H5Iget_name failed");

        if(bufSize > 0) {
            buf.resize(type::safe_cast<size_t>(bufSize) + 1);                      // We allocate space for the null terminator with +1
            H5Iget_name(h5type, buf.data(), type::safe_cast<size_t>(bufSize + 1)); // Read name including \0 with +1
        }
        return buf.c_str(); // Use .c_str() to convert to a "standard" std::string, i.e. one where .size() does not include \0
    }

    template<typename h5t1, typename h5t2>
    [[nodiscard]] inline bool H5Tequal_recurse(const h5t1 &type1, const h5t2 &type2) {
        static_assert(type::sfinae::is_hdf5_type_id<h5t1> and type::sfinae::is_hdf5_type_id<h5t2>,
                      "Template function [h5pp::hdf5::H5Tequal_recurse<h5t>(const h5t1 & type1, const h5t2 & type2)]\n"
                      "requires type h5t1 and h5t2 to be: [h5pp::hid::h5t] or [hid_t]");
        // If types are compound, check recursively that all members have equal types and names
        if constexpr(type::sfinae::are_same_v<hid_t, h5t1, h5t2>) {
            if(type1 == type2) return true;
        } else if constexpr(type::sfinae::are_same_v<hid::h5t, h5t1, h5t2>) {
            if(type1.value() == type2.value()) return true;
        }

        H5T_class_t dataClass1 = H5Tget_class(type1);
        H5T_class_t dataClass2 = H5Tget_class(type2);
        if(dataClass1 != dataClass2) return false;
        if(dataClass1 == H5T_STRING) {
            return true;
        } else if(dataClass1 == H5T_COMPOUND and dataClass2 == H5T_COMPOUND) {
            size_t sizeType1 = H5Tget_size(type1);
            size_t sizeType2 = H5Tget_size(type2);
            if(sizeType1 != sizeType2) return false;
            auto nMembers1 = H5Tget_nmembers(type1);
            auto nMembers2 = H5Tget_nmembers(type2);
            if(nMembers1 != nMembers2) return false;
            for(int idx = 0; idx < nMembers1; idx++) {
                hid::h5t t1 = H5Tget_member_type(type1, static_cast<unsigned int>(idx));
                hid::h5t t2 = H5Tget_member_type(type2, static_cast<unsigned int>(idx));
                if(not H5Tequal_recurse(t1, t2)) return false;
            }
            return true;
        } else if constexpr(type::sfinae::is_h5pp_type_id<h5t1> and type::sfinae::is_h5pp_type_id<h5t2>) {
            return type1 == type2;
        } else {
            htri_t res = H5Tequal(type1, type2);
            if(res < 0) throw h5pp::runtime_error("Failed to check type equality");

            return res > 0;
        }
    }

    template<typename DataType, size_t depth = 0>
    [[nodiscard]] hid::h5t getH5Type() {
        //        if(h5type.has_value()) return h5type.value(); // Intercept
        namespace tc    = h5pp::type::sfinae;
        using DecayType = typename std::decay<DataType>::type;

        /* clang-format off */
        if constexpr      (std::is_pointer_v<DecayType>)                     return getH5Type<typename std::remove_pointer<DecayType>::type, depth+1>();
        else if constexpr (std::is_reference_v<DecayType>)                   return getH5Type<typename std::remove_reference<DecayType>::type, depth+1>();
        else if constexpr (std::is_array_v<DecayType>)                       return getH5Type<typename std::remove_all_extents<DecayType>::type, depth+1>();
        else if constexpr (std::is_same_v<DecayType, short>)                 return H5Tcopy(H5T_NATIVE_SHORT);
        else if constexpr (std::is_same_v<DecayType, int>)                   return H5Tcopy(H5T_NATIVE_INT);
        else if constexpr (std::is_same_v<DecayType, long>)                  return H5Tcopy(H5T_NATIVE_LONG);
        else if constexpr (std::is_same_v<DecayType, long long>)             return H5Tcopy(H5T_NATIVE_LLONG);
        else if constexpr (std::is_same_v<DecayType, unsigned short>)        return H5Tcopy(H5T_NATIVE_USHORT);
        else if constexpr (std::is_same_v<DecayType, unsigned int>)          return H5Tcopy(H5T_NATIVE_UINT);
        else if constexpr (std::is_same_v<DecayType, unsigned long>)         return H5Tcopy(H5T_NATIVE_ULONG);
        else if constexpr (std::is_same_v<DecayType, unsigned long long >)   return H5Tcopy(H5T_NATIVE_ULLONG);
        else if constexpr (std::is_same_v<DecayType, float>)                 return H5Tcopy(H5T_NATIVE_FLOAT);
        else if constexpr (std::is_same_v<DecayType, double>)                return H5Tcopy(H5T_NATIVE_DOUBLE);
        else if constexpr (std::is_same_v<DecayType, long double>)           return H5Tcopy(H5T_NATIVE_LDOUBLE);
        #if defined(H5PP_USE_FLOAT128)
        else if constexpr(std::is_same_v<DecayType, __float128>)             return H5Tcopy(type::custom::H5T_FLOAT<__float128>::h5type());
        #endif
        else if constexpr (std::is_same_v<DecayType, int8_t>)                return H5Tcopy(H5T_NATIVE_INT8);
        else if constexpr (std::is_same_v<DecayType, int16_t>)               return H5Tcopy(H5T_NATIVE_INT16);
        else if constexpr (std::is_same_v<DecayType, int32_t>)               return H5Tcopy(H5T_NATIVE_INT32);
        else if constexpr (std::is_same_v<DecayType, int64_t>)               return H5Tcopy(H5T_NATIVE_INT64);
        else if constexpr (std::is_same_v<DecayType, uint8_t>)               return H5Tcopy(H5T_NATIVE_UINT8);
        else if constexpr (std::is_same_v<DecayType, uint16_t>)              return H5Tcopy(H5T_NATIVE_UINT16);
        else if constexpr (std::is_same_v<DecayType, uint32_t>)              return H5Tcopy(H5T_NATIVE_UINT32);
        else if constexpr (std::is_same_v<DecayType, uint64_t>)              return H5Tcopy(H5T_NATIVE_UINT64);
        else if constexpr (std::is_same_v<DecayType, bool>)                  return H5Tcopy(H5T_NATIVE_UINT8);
        else if constexpr (std::is_same_v<DecayType, std::string>)           return H5Tcopy(H5T_C_S1);
        else if constexpr (std::is_same_v<DecayType, std::string_view>)      return H5Tcopy(H5T_C_S1);
        else if constexpr (std::is_same_v<DecayType, char>)                  return H5Tcopy(H5T_C_S1);
        else if constexpr (std::is_same_v<DecayType, std::byte>)             return H5Tcopy(H5T_NATIVE_UCHAR);
        else if constexpr (tc::is_varr_v<DecayType>)                         return DecayType::get_h5type();
        else if constexpr (tc::is_vstr_v<DecayType>)                         return DecayType::get_h5type();
        else if constexpr (tc::is_fstr_v<DecayType>)                         return DecayType::get_h5type();
        else if constexpr (tc::is_std_complex_v<DecayType>)                  return H5Tcopy(type::compound::H5T_COMPLEX<typename DecayType::value_type>::h5type());
        else if constexpr (tc::is_Scalar2_v<DecayType>)                      return H5Tcopy(type::compound::H5T_SCALAR2<tc::get_Scalar2_t<DecayType>>::h5type());
        else if constexpr (tc::is_Scalar3_v<DecayType>)                      return H5Tcopy(type::compound::H5T_SCALAR3<tc::get_Scalar3_t<DecayType>>::h5type());
        else if constexpr (tc::is_std_array_v<DecayType> and depth == 0 )      return getH5Type<typename DecayType::value_type, depth+1>();
        else if constexpr (tc::is_std_array_v<DecayType> and depth == 1 ){
            constexpr std::array<hsize_t, 1> dims = {std::tuple_size<DecayType>::value};
            return H5Tarray_create(getH5Type<typename DecayType::value_type, depth+1>(), dims.size(), dims.data()) ;
        }
        else if constexpr (tc::has_Scalar_v<DecayType>)                      return getH5Type<typename DecayType::Scalar, depth+1>();
        else if constexpr (tc::has_value_type_v <DecayType>)                 return getH5Type<typename DecayType::value_type, depth+1>();
        else if constexpr (std::is_same_v<DecayType, hvl_t>)                 return H5Tvlen_create(H5T_NATIVE_OPAQUE); // Last resort ... user should provide a h5 type at runtime
        else if constexpr (std::is_enum_v<DecayType>)                        return getH5Type<std::underlying_type_t<DecayType>>(); // Last resort ... user should provide a h5 type at runtime
        else if constexpr (std::is_class_v<DecayType>)                       return H5Tcreate(H5T_COMPOUND, sizeof(DecayType)); // Last resort ... user should provide a h5 type at runtime
        else static_assert(type::sfinae::unrecognized_type_v<DecayType> and "h5pp could not match the given C++ type to an HDF5 type.");
        /* clang-format on */
        throw h5pp::runtime_error("getH5Type could not match the type provided [{}] | size {}",
                                  type::sfinae::type_name<DecayType>(),
                                  sizeof(DecayType));
        return hid_t(0);
    }

    template<typename T>
    [[nodiscard]] std::tuple<std::type_index, std::string, size_t> getCppType() {
        return {typeid(T), std::string(type::sfinae::type_name<T>()), sizeof(T)};
    }

    [[nodiscard]] inline std::tuple<std::type_index, std::string, size_t> getCppType(const hid::h5t &type) {
        using namespace h5pp::type::compound;
        auto h5class = H5Tget_class(type);

        /* clang-format off */
        if(h5class == H5T_class_t::H5T_INTEGER){
            auto h5sign  = H5Tget_sign(type);
            if(h5sign  == H5T_sign_t::H5T_SGN_ERROR) throw h5pp::runtime_error("H5Tget_sign() failed on integral type");
            if(h5sign == H5T_sign_t::H5T_SGN_NONE){
                if(H5Tequal(type, H5T_NATIVE_USHORT))   return getCppType<unsigned short>();
                if(H5Tequal(type, H5T_NATIVE_UINT))     return getCppType<unsigned int>();
                if(H5Tequal(type, H5T_NATIVE_ULONG))    return getCppType<unsigned long>();
                if(H5Tequal(type, H5T_NATIVE_ULLONG))   return getCppType<unsigned long long>();
            }
            else if(h5sign == H5T_sign_t::H5T_SGN_2){
               if(H5Tequal(type, H5T_NATIVE_SHORT))   return getCppType<short>();
               if(H5Tequal(type, H5T_NATIVE_INT))     return getCppType<int>();
               if(H5Tequal(type, H5T_NATIVE_LONG))    return getCppType<long>();
               if(H5Tequal(type, H5T_NATIVE_LLONG))   return getCppType<long long>();
            }
        }else if (h5class == H5T_class_t::H5T_FLOAT){
            if(H5Tequal(type, H5T_NATIVE_FLOAT))            return getCppType<float>();
            if(H5Tequal(type, H5T_NATIVE_DOUBLE))           return getCppType<double>();
            if(H5Tequal(type, H5T_NATIVE_LDOUBLE))          return getCppType<long double>();
        }else if  (h5class == H5T_class_t::H5T_STRING){
            if(H5Tequal(type, H5T_NATIVE_CHAR))             return getCppType<char>();
            if(H5Tequal_recurse(type, H5Tcopy(H5T_C_S1)))   return getCppType<std::string>();
            if(H5Tequal(type, H5T_NATIVE_SCHAR))            return getCppType<signed char>();
            if(H5Tequal(type, H5T_NATIVE_UCHAR))            return getCppType<unsigned char>();
        }else if (h5class == H5T_class_t::H5T_VLEN){
            return getCppType<hvl_t>();
        }else if (h5class == H5T_COMPOUND){
            if(is_complex(type)){
                auto h5mtype = H5Tget_member_type(type,0);
                auto h5mclass = H5Tget_class(h5mtype);
                if(h5mclass == H5T_class_t::H5T_FLOAT){
                    if (H5T_COMPLEX<float>::equal(type))            return getCppType<std::complex<float>>();
                    if (H5T_COMPLEX<double>::equal(type))           return getCppType<std::complex<double>>();
                    if (H5T_COMPLEX<long double>::equal(type))      return getCppType<std::complex<long double>>();
                }
                if(h5mclass == H5T_class_t::H5T_INTEGER){
                    auto h5msign  = H5Tget_sign(h5mtype);
                    if(h5msign  == H5T_sign_t::H5T_SGN_ERROR) throw h5pp::runtime_error("H5Tget_sign() failed on integral type");
                    if(h5msign == H5T_sign_t::H5T_SGN_NONE){
                        if (H5T_COMPLEX<unsigned short>::equal(type))       return getCppType<std::complex<unsigned short>>();
                        if (H5T_COMPLEX<unsigned int>::equal(type))         return getCppType<std::complex<unsigned int>>();
                        if (H5T_COMPLEX<unsigned long>::equal(type))        return getCppType<std::complex<unsigned long>>();
                        if (H5T_COMPLEX<unsigned long long>::equal(type))   return getCppType<std::complex<unsigned long long>>();
                    }
                    if(h5msign == H5T_sign_t::H5T_SGN_2){
                        if (H5T_COMPLEX<short>::equal(type))       return getCppType<std::complex<short>>();
                        if (H5T_COMPLEX<int>::equal(type))         return getCppType<std::complex<int>>();
                        if (H5T_COMPLEX<long>::equal(type))        return getCppType<std::complex<long>>();
                        if (H5T_COMPLEX<long long>::equal(type))   return getCppType<std::complex<long long>>();
                    }
                }
            }
            if(is_scalar2(type)){
                auto h5mtype = H5Tget_member_type(type,0);
                auto h5mclass = H5Tget_class(h5mtype);
                if(h5mclass == H5T_class_t::H5T_FLOAT){
                    if (H5T_SCALAR2<float>::equal(type))            return getCppType<h5pp::type::compound::Scalar2<float>>();
                    if (H5T_SCALAR2<double>::equal(type))           return getCppType<h5pp::type::compound::Scalar2<double>>();
                    if (H5T_SCALAR2<long double>::equal(type))      return getCppType<h5pp::type::compound::Scalar2<long double>>();
                }
                if(h5mclass == H5T_class_t::H5T_INTEGER){
                    auto h5msign  = H5Tget_sign(h5mtype);
                    if(h5msign  == H5T_sign_t::H5T_SGN_ERROR) throw h5pp::runtime_error("H5Tget_sign() failed on integral type");
                    if(h5msign == H5T_sign_t::H5T_SGN_NONE){
                        if (H5T_SCALAR2<unsigned short>::equal(type))       return getCppType<h5pp::type::compound::Scalar2<unsigned short>>();
                        if (H5T_SCALAR2<unsigned int>::equal(type))         return getCppType<h5pp::type::compound::Scalar2<unsigned int>>();
                        if (H5T_SCALAR2<unsigned long>::equal(type))        return getCppType<h5pp::type::compound::Scalar2<unsigned long>>();
                        if (H5T_SCALAR2<unsigned long long>::equal(type))   return getCppType<h5pp::type::compound::Scalar2<unsigned long long>>();
                    }
                    if(h5msign == H5T_sign_t::H5T_SGN_2){
                        if (H5T_SCALAR2<short>::equal(type))       return getCppType<h5pp::type::compound::Scalar2<short>>();
                        if (H5T_SCALAR2<int>::equal(type))         return getCppType<h5pp::type::compound::Scalar2<int>>();
                        if (H5T_SCALAR2<long>::equal(type))        return getCppType<h5pp::type::compound::Scalar2<long>>();
                        if (H5T_SCALAR2<long long>::equal(type))   return getCppType<h5pp::type::compound::Scalar2<long long>>();
                    }
                }
            }
            if(is_scalar3(type)){
                auto h5mtype = H5Tget_member_type(type,0);
                auto h5mclass = H5Tget_class(h5mtype);
                if(h5mclass == H5T_class_t::H5T_FLOAT){
                    if (H5T_SCALAR3<float>::equal(type))            return getCppType<h5pp::type::compound::Scalar3<float>>();
                    if (H5T_SCALAR3<double>::equal(type))           return getCppType<h5pp::type::compound::Scalar3<double>>();
                    if (H5T_SCALAR3<long double>::equal(type))      return getCppType<h5pp::type::compound::Scalar3<long double>>();
                }
                if(h5mclass == H5T_class_t::H5T_INTEGER){
                    auto h5msign  = H5Tget_sign(h5mtype);
                    if(h5msign  == H5T_sign_t::H5T_SGN_ERROR) throw h5pp::runtime_error("H5Tget_sign() failed on integral type");
                    if(h5msign == H5T_sign_t::H5T_SGN_NONE){
                        if (H5T_SCALAR3<unsigned short>::equal(type))       return getCppType<h5pp::type::compound::Scalar3<unsigned short>>();
                        if (H5T_SCALAR3<unsigned int>::equal(type))         return getCppType<h5pp::type::compound::Scalar3<unsigned int>>();
                        if (H5T_SCALAR3<unsigned long>::equal(type))        return getCppType<h5pp::type::compound::Scalar3<unsigned long>>();
                        if (H5T_SCALAR3<unsigned long long>::equal(type))   return getCppType<h5pp::type::compound::Scalar3<unsigned long long>>();
                    }
                    if(h5msign == H5T_sign_t::H5T_SGN_2){
                        if (H5T_SCALAR3<short>::equal(type))       return getCppType<h5pp::type::compound::Scalar3<short>>();
                        if (H5T_SCALAR3<int>::equal(type))         return getCppType<h5pp::type::compound::Scalar3<int>>();
                        if (H5T_SCALAR3<long>::equal(type))        return getCppType<h5pp::type::compound::Scalar3<long>>();
                        if (H5T_SCALAR3<long long>::equal(type))   return getCppType<h5pp::type::compound::Scalar3<long long>>();
                    }
                }
            }

            // type is some other compound type.
            auto h5size  = H5Tget_size(type);
            auto h5type = H5Tget_native_type(type, H5T_direction_t::H5T_DIR_DEFAULT); // Unrolls nested compound types

            auto nmembers = H5Tget_nmembers(h5type);
            auto nmembers_ul = type::safe_cast<unsigned int>(nmembers);

            std::vector<std::string> cpptypenames(nmembers_ul);
            for(unsigned int idx = 0; idx < nmembers_ul; idx++ ){
                auto cpptype = getCppType(H5Tget_member_type(h5type, idx));
                cpptypenames[idx] = std::get<1>(cpptype);
            }
            return {typeid(std::vector<std::byte>), h5pp::format("{}", cpptypenames), h5size};
        }
        if(H5Tequal(type, H5T_NATIVE_HBOOL))            return getCppType<hbool_t>();
        if(H5Tequal(type, H5T_NATIVE_B8))               return getCppType<std::byte>();

        auto name = getH5ClassName(type);
        /* clang-format on */
        auto h5size = H5Tget_size(type);
        if(H5Tcommitted(type) > 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            if(h5pp::logger::log->level() == 0) h5pp::logger::log->trace("No C++ type match for HDF5 type [{}]", getH5TypeName(type));
        } else {
            h5pp::logger::log->trace("No known C++ type matches HDF5 type of class [ {} | {} bytes ]. This is usually not a problem",
                                     h5pp::enum2str(h5class),
                                     h5size);
        }
        return {typeid(nullptr), name, h5size};
    }

}