//
// Created by david on 2019-03-03.
//

#ifndef H5PP_DATATYPE_H
#define H5PP_DATATYPE_H
//#include <h5pp/details/h5ppLogger.h>
//#include <h5pp/details/h5ppUtils.h>
#include <h5pp/details/h5ppTypeCheck.h>
#include <h5pp/details/h5ppTypeComplex.h>
//namespace fs = std::experimental::filesystem;


namespace h5pp{
    namespace Type{
        template<typename DataType>
        constexpr hid_t get_DataType() {
            namespace tc = h5pp::Type::Check;
            if constexpr (std::is_same<DataType, int>::value)                         {return  H5Tcopy(H5T_NATIVE_INT);}
            if constexpr (std::is_same<DataType, long>::value)                        {return  H5Tcopy(H5T_NATIVE_LONG);}
            if constexpr (std::is_same<DataType, unsigned int>::value)                {return  H5Tcopy(H5T_NATIVE_UINT);}
            if constexpr (std::is_same<DataType, unsigned long>::value)               {return  H5Tcopy(H5T_NATIVE_ULONG);}
            if constexpr (std::is_same<DataType, double>::value)                      {return  H5Tcopy(H5T_NATIVE_DOUBLE);}
            if constexpr (std::is_same<DataType, float>::value)                       {return  H5Tcopy(H5T_NATIVE_FLOAT);}
            if constexpr (std::is_same<DataType, bool>::value)                        {return  H5Tcopy(H5T_NATIVE_HBOOL);}
            if constexpr (std::is_same<DataType, std::complex<int>>::value)           {return  H5Tcopy(Complex::H5T_COMPLEX_INT);}
            if constexpr (std::is_same<DataType, std::complex<long>>::value)          {return  H5Tcopy(Complex::H5T_COMPLEX_LONG);}
            if constexpr (std::is_same<DataType, std::complex<unsigned int>>::value)  {return  H5Tcopy(Complex::H5T_COMPLEX_UINT);}
            if constexpr (std::is_same<DataType, std::complex<unsigned long>>::value) {return  H5Tcopy(Complex::H5T_COMPLEX_ULONG);}
            if constexpr (std::is_same<DataType, std::complex<double>>::value)        {return  H5Tcopy(Complex::H5T_COMPLEX_DOUBLE);}
            if constexpr (std::is_same<DataType, std::complex<float>>::value)         {return  H5Tcopy(Complex::H5T_COMPLEX_FLOAT);}
//            if constexpr (std::is_same<DataType, H5T_COMPLEX_STRUCT>::value)          {return  H5Tcopy(H5T_COMPLEX_DOUBLE);}
            if constexpr (std::is_same<DataType, char>::value)                        {return  H5Tcopy(H5T_C_S1);}
            if constexpr (std::is_same<DataType, const char *>::value)                {return  H5Tcopy(H5T_C_S1);}
            if constexpr (std::is_same<DataType, std::string>::value)                 {return  H5Tcopy(H5T_C_S1);}
            if constexpr (tc::is_eigen_tensor<DataType>::value)                       {return  get_DataType<typename DataType::Scalar>();}
            if constexpr (tc::is_eigen_matrix<DataType>::value)                       {return  get_DataType<typename DataType::Scalar>();}
            if constexpr (tc::is_eigen_array<DataType>::value)                        {return  get_DataType<typename DataType::Scalar>();}
            if constexpr (tc::is_vector<DataType>::value)                             {return  get_DataType<typename DataType::value_type>();}
            if constexpr (tc::has_member_scalar <DataType>::value)                    {return  get_DataType<typename DataType::Scalar>();}
            if constexpr (tc::has_member_value_type <DataType>::value)                {return  get_DataType<typename DataType::value_type>();}
            spdlog::critical("get_DataType could not match the type provided");
            throw(std::logic_error("get_DataType could not match the type provided"));
        }
        namespace Complex{
            template<typename T>
            hid_t createComplexType(){
                hid_t NEW_COMPLEX_TYPE = H5Tcreate (H5T_COMPOUND, sizeof(H5T_COMPLEX_STRUCT<T>));
                H5Tinsert (NEW_COMPLEX_TYPE, "real", HOFFSET(H5T_COMPLEX_STRUCT<T>,real), get_DataType<T>());
                H5Tinsert (NEW_COMPLEX_TYPE, "imag", HOFFSET(H5T_COMPLEX_STRUCT<T>,imag), get_DataType<T>());
                return H5Tcopy(NEW_COMPLEX_TYPE);
            }

            inline void initTypes(){
                H5T_COMPLEX_INT        = createComplexType<int>();
                H5T_COMPLEX_LONG       = createComplexType<long>();
                H5T_COMPLEX_UINT       = createComplexType<unsigned int>();
                H5T_COMPLEX_ULONG      = createComplexType<unsigned long>();
                H5T_COMPLEX_DOUBLE     = createComplexType<double>();
                H5T_COMPLEX_FLOAT      = createComplexType<float>();
            }

            inline void closeTypes(){
                H5Tclose(H5T_COMPLEX_INT   );
                H5Tclose(H5T_COMPLEX_LONG  );
                H5Tclose(H5T_COMPLEX_UINT  );
                H5Tclose(H5T_COMPLEX_ULONG );
                H5Tclose(H5T_COMPLEX_DOUBLE);
                H5Tclose(H5T_COMPLEX_FLOAT );
            }


        }

    }
}

#endif //H5PP_H5PPDATATYPE_H
