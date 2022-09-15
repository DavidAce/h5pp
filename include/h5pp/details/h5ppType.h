#pragma once
#include "h5ppError.h"
#include "h5ppTypeSfinae.h"
#include <H5Tpublic.h>
#include <cstddef>

namespace tc = h5pp::type::sfinae;
namespace h5pp::type {
    template<typename DataType>
    [[nodiscard]] hid::h5t getH5NativeType() {
        //        if(h5type.has_value()) return h5type.value(); // Intercept
        /* clang-format off */
        using DecayType    = typename std::decay<DataType>::type;
        if constexpr (std::is_pointer_v<DecayType>)                      return getH5NativeType<typename std::remove_pointer<DecayType>::type>();
        if constexpr (std::is_reference_v<DecayType>)                    return getH5NativeType<typename std::remove_reference<DecayType>::type>();
        if constexpr (std::is_array_v<DecayType>)                        return getH5NativeType<typename std::remove_all_extents<DecayType>::type>();
        if constexpr (std::is_same_v<DecayType, short>)                  return H5Tcopy(H5T_NATIVE_SHORT);
        if constexpr (std::is_same_v<DecayType, int>)                    return H5Tcopy(H5T_NATIVE_INT);
        if constexpr (std::is_same_v<DecayType, long>)                   return H5Tcopy(H5T_NATIVE_LONG);
        if constexpr (std::is_same_v<DecayType, long long>)              return H5Tcopy(H5T_NATIVE_LLONG);
        if constexpr (std::is_same_v<DecayType, unsigned short>)         return H5Tcopy(H5T_NATIVE_USHORT);
        if constexpr (std::is_same_v<DecayType, unsigned int>)           return H5Tcopy(H5T_NATIVE_UINT);
        if constexpr (std::is_same_v<DecayType, unsigned long>)          return H5Tcopy(H5T_NATIVE_ULONG);
        if constexpr (std::is_same_v<DecayType, unsigned long long >)    return H5Tcopy(H5T_NATIVE_ULLONG);
        if constexpr (std::is_same_v<DecayType, double>)                 return H5Tcopy(H5T_NATIVE_DOUBLE);
        if constexpr (std::is_same_v<DecayType, long double>)            return H5Tcopy(H5T_NATIVE_LDOUBLE);
        if constexpr (std::is_same_v<DecayType, float>)                  return H5Tcopy(H5T_NATIVE_FLOAT);
        if constexpr (std::is_same_v<DecayType, bool>)                   return H5Tcopy(H5T_NATIVE_UINT8);
        if constexpr (std::is_same_v<DecayType, std::string>)            return H5Tcopy(H5T_C_S1);
        if constexpr (std::is_same_v<DecayType, char>)                   return H5Tcopy(H5T_C_S1);
        if constexpr (std::is_same_v<DecayType, std::byte>)              return H5Tcopy(H5T_NATIVE_B8);
        if constexpr (std::is_same_v<DecayType, int8_t>)                 return H5Tcopy(H5T_NATIVE_INT8);
        if constexpr (std::is_same_v<DecayType, int16_t>)                return H5Tcopy(H5T_NATIVE_INT16);
        if constexpr (std::is_same_v<DecayType, int32_t>)                return H5Tcopy(H5T_NATIVE_INT32);
        if constexpr (std::is_same_v<DecayType, int64_t>)                return H5Tcopy(H5T_NATIVE_INT64);
        if constexpr (std::is_same_v<DecayType, uint8_t>)                return H5Tcopy(H5T_NATIVE_UINT8);
        if constexpr (std::is_same_v<DecayType, uint16_t>)               return H5Tcopy(H5T_NATIVE_UINT16);
        if constexpr (std::is_same_v<DecayType, uint32_t>)               return H5Tcopy(H5T_NATIVE_UINT32);
        if constexpr (std::is_same_v<DecayType, uint64_t>)               return H5Tcopy(H5T_NATIVE_UINT64);
        if constexpr (tc::has_Scalar_v <DecayType>)                      return getH5NativeType<typename DecayType::Scalar>();
        if constexpr (tc::has_value_type_v <DecayType>)                  return getH5NativeType<typename DataType::value_type>();
        /* clang-format on */
        throw h5pp::runtime_error(h5pp::format("getH5ValueType could not match the type provided [{}] | size {}",
                                               type::sfinae::type_name<DecayType>(),
                                               sizeof(DecayType)));
        return hid_t(0);
    }
}