#pragma once
#include <spdlog/spdlog.h>
#include "h5ppType.h"
#include "h5ppTypeCheck.h"



namespace h5pp{
    namespace Utils{
        template <typename DataType, size_t size>
        constexpr size_t getArraySize([[maybe_unused]] const DataType (&arr)[size]){
            if constexpr (std::is_same<char, typename std::decay_t<DataType>>::value)  {
                return strlen(arr);
            }else{
                return size;
            }
        }

        inline hsize_t setStringSize(hid_t datatype, hsize_t size){
            size = std::max((hsize_t) 1, size);
            herr_t retval = H5Tset_size(datatype, size);
            if(retval < 0){
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to set size: " + std::to_string(size));
            }
//            retval  = H5Tset_strpad(datatype,H5T_STR_NULLTERM);
//            retval  = H5Tset_strpad(datatype,H5T_STR_NULLPAD);
            if(retval < 0){
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to set strpad");
            }
            return size;
        }




        template<typename DataType>
        hsize_t getSize(const DataType &data){
            namespace tc = h5pp::Type::Check;
            if constexpr (tc::hasMember_size<DataType>::value)                                              {return data.size();} //Fails on clang?
//            else if constexpr (std::is_same<std::decay_t<char[]>, typename std::decay_t<DataType>>::value)  {return strlen(data);}
            else if constexpr (std::is_array<DataType>::value)                                              {return getArraySize(data);}
            else if constexpr (std::is_arithmetic<DataType>::value)                                         {return 1;}
            else if constexpr (std::is_pod<DataType>::value)                                                {return 1;}
            else if constexpr (tc::is_StdComplex<DataType>())                                               {return 1;}
            else{
                spdlog::warn("WARNING: getSize can't match the type provided: " + std::string(typeid(data).name()));
                return data.size();
            }
        }


        template<typename DataType>
        constexpr int getRank() {
            namespace tc = h5pp::Type::Check;
            if      constexpr(tc::is_eigen_tensor<DataType>::value){return (int) DataType::NumIndices;}
            else if constexpr(tc::is_eigen_core<DataType>::value){return 2; }
            else if constexpr(std::is_arithmetic<DataType>::value){return 1; }
            else if constexpr(tc::is_std_vector<DataType>::value){return 1;}
            else if constexpr(tc::is_std_array<DataType>::value){return 1;}
            else if constexpr(tc::is_ScalarN<DataType>()){return 1;}
            else if constexpr(std::is_same<std::string, DataType>::value){return 1;}
            else if constexpr(std::is_same<const char *,DataType>::value){return 1;}
            else if constexpr(std::is_array<DataType>::value){return 1;}
            else if constexpr(std::is_pod<DataType>::value){return 1;}
            else if constexpr(tc::is_StdComplex<DataType>()){return 1;}
            else {
                tc::print_type_and_exit_compile_time<DataType>();
            }
        }

        template<typename T>
        size_t getSizeOf(){
            namespace tc = h5pp::Type::Check;

            // If userDataType is a container we should check that elements in the container have matching size as the hdf5DataType
            if constexpr (tc::is_StdComplex<T>())               {return sizeof(T);}
            if constexpr (tc::is_ScalarN<T>())                  {return sizeof(T);}
            if constexpr (std::is_arithmetic<T>::value)         {return sizeof(T);}
            if constexpr (tc::is_eigen_type<T>::value)          {return sizeof(typename T::Scalar);}
            if constexpr (tc::is_std_vector<T>::value)          {return sizeof(typename T::value_type);}
            if constexpr (tc::is_std_array<T>::value)           {return sizeof(typename T::value_type);}
            if constexpr (std::is_array<T>::value )             {return sizeof(std::remove_all_extents_t<T>);}
            if constexpr (std::is_same<T,std::string>::value )  {return sizeof(char);}
            else return sizeof(std::remove_all_extents_t<T>);

        }

        template<typename userDataType>
        bool typeSizesMatch( [[maybe_unused]]  hid_t hdf5Datatype){
            size_t hdf5DataTypeSize;
            size_t userDataTypeSize;
            if constexpr (std::is_same<userDataType,std::string>::value or std::is_same<userDataType,char[]>::value){
                hdf5DataTypeSize = sizeof(char);
                userDataTypeSize = getSizeOf<userDataType>();
            }else{
                hdf5DataTypeSize = H5Tget_size(hdf5Datatype);
                userDataTypeSize = getSizeOf<userDataType>();
            }

            if (userDataTypeSize != hdf5DataTypeSize){
                h5pp::Logger::log->error("Type size mismatch: given {} bytes | inferred {} bytes", userDataTypeSize,hdf5DataTypeSize);
                return false;
            }else{
                return true;
            }
        }

        template <typename DataType>
        std::vector<hsize_t> getDimensions(const DataType &data) {
            namespace tc = h5pp::Type::Check;
            constexpr int rank = getRank<DataType>();
            std::vector<hsize_t> dims(rank);
            if constexpr (tc::is_eigen_tensor<DataType>::value){
                std::copy(data.dimensions().begin(), data.dimensions().end(), dims.begin());
                return dims;
            }
            else if constexpr (tc::is_eigen_core <DataType>::value) {
                dims[0] = (hsize_t) data.rows();
                dims[1] = (hsize_t) data.cols();
                return dims;
            }
            else if constexpr(tc::is_std_vector<DataType>::value){
                dims[0]={data.size()};
                return dims;
            }
            else if constexpr(tc::is_std_array<DataType>::value){
                dims[0] = data.size();
                return dims;
            }
            else if constexpr(std::is_same<std::string, DataType>::value or std::is_same<char *, typename std::decay<DataType>::type>::value){
                // Read more about this step here
                //http://www.astro.sunysb.edu/mzingale/io_tutorial/HDF5_simple/hdf5_simple.c
                dims[0]= 1;
                return dims;
            }
            else if constexpr(std::is_array<DataType>::value){
                dims[0] = getArraySize(data);
                return dims;
            }
            else if constexpr(tc::hasMember_size<DataType>::value and rank == 1){
                dims[0] = 1;
                return dims;
            }
            else if constexpr (std::is_arithmetic<DataType>::value or tc::is_StdComplex<DataType>()){
                dims[0]= 1;
                return dims;
            }

            else{
//                static_assert(false, "getDimensions can't match the type provided");
                tc::print_type_and_exit_compile_time<DataType>();
                std::string error = "getDimensions can't match the type provided: " + std::string(typeid(DataType).name());
                spdlog::critical(error);
                throw(std::logic_error(error));
            }

        }





        inline hid_t getDataSpace(const int rank, const std::vector<hsize_t> &dims, const bool unlimited = false){
            std::vector<hsize_t> max_dims(rank);
            if (unlimited){
                std::fill_n(max_dims.begin(), rank, H5S_UNLIMITED);
                return H5Screate_simple(rank, dims.data(), max_dims.data());
            }else{
                if(rank == 0){
                    return H5Screate(H5S_SCALAR);
                }else{
                    max_dims = dims;
                    return H5Screate_simple(rank, dims.data(), max_dims.data());
                }
            }
        }



        inline hid_t getMemSpace(const int rank, const std::vector<hsize_t> &dims) {
            if(rank == 0){
                return H5Screate(H5S_SCALAR);
            }else{
                return H5Screate_simple(rank, dims.data(), nullptr);
            }
        }


        template<typename DataType>
        auto convertComplexDataToH5T(const DataType &data){
            static_assert(h5pp::Type::Check::hasStdComplex<DataType>() or h5pp::Type::Check::is_StdComplex<DataType>(),
                    "Data must be complex for conversion to H5T_COMPLEX_STRUCT");
            if constexpr(h5pp::Type::Check::is_eigen_type<DataType>::value){
                using scalarType  = typename DataType::Scalar;
                using complexType = typename scalarType::value_type;
                std::vector<h5pp::Type::Complex::H5T_COMPLEX_STRUCT<complexType>> newData;
                newData.insert(newData.end(), data.data(), data.data()+data.size());
                return newData;
            }
            else if constexpr(h5pp::Type::Check::is_std_vector<DataType>::value) {
                using scalarType  = typename DataType::value_type;
                using complexType = typename scalarType::value_type;
                std::vector<h5pp::Type::Complex::H5T_COMPLEX_STRUCT<complexType>> newData;
                newData.insert(newData.end(), data.data(), data.data()+data.size());
                return newData;
            }
            else if  constexpr (h5pp::Type::Check::is_StdComplex<DataType>()){
                return h5pp::Type::Complex::H5T_COMPLEX_STRUCT<typename DataType::value_type>(data);
            }else{
                //This should never happen.
                throw::std::runtime_error("Unrecognized complex type: " +  std::string(typeid(data).name() ));
            }
        }

        template<typename DataType>
        auto convertScalar2DataToH5T(const DataType &data){
            static_assert(h5pp::Type::Check::hasScalar2<DataType>() or h5pp::Type::Check::is_Scalar2<DataType>(),
                          "Data must be a struct of two scalars for conversion to H5T_SCALAR2");
            if constexpr(h5pp::Type::Check::is_eigen_type<DataType>::value){
                using internalType  = decltype(DataType::x);
                std::vector<h5pp::Type::Complex::H5T_SCALAR2<internalType>> newData;
                newData.insert(newData.end(), data.data(), data.data()+data.size());
                return newData;
            }
            else if constexpr(h5pp::Type::Check::is_std_vector<DataType>::value) {
                using scalarType  = typename DataType::value_type;
                using internalType  = decltype(scalarType::x);
                std::vector<h5pp::Type::Complex::H5T_SCALAR2<internalType>> newData;
                newData.insert(newData.end(), data.data(), data.data()+data.size());
                return newData;
            }
            else if  constexpr (h5pp::Type::Check::is_Scalar2<DataType>()){
                using internalType  = decltype(DataType::x);
                return h5pp::Type::Complex::H5T_SCALAR2<internalType>(data);
            }else{
                //This should never happen.
                throw::std::runtime_error("Unrecognized Scalar2 type: " +  std::string(typeid(data).name() ));
            }
        }


        template<typename DataType>
        auto convertScalar3DataToH5T(const DataType &data){
            static_assert(h5pp::Type::Check::hasScalar3<DataType>() or h5pp::Type::Check::is_Scalar3<DataType>(),
                          "Data must be a struct of three scalars for conversion to H5T_SCALAR3");
            if constexpr(h5pp::Type::Check::is_eigen_type<DataType>::value){
                using internalType  = decltype(DataType::x);
                std::vector<h5pp::Type::Complex::H5T_SCALAR3<internalType>> newData;
                newData.insert(newData.end(), data.data(), data.data()+data.size());
                return newData;
            }
            else if constexpr(h5pp::Type::Check::is_std_vector<DataType>::value) {
                using scalarType  = typename DataType::value_type;
                using internalType  = decltype(scalarType::x);
                std::vector<h5pp::Type::Complex::H5T_SCALAR3<internalType>> newData;
                newData.insert(newData.end(), data.data(), data.data()+data.size());
                return newData;
            }
            else if  constexpr (h5pp::Type::Check::is_Scalar3<DataType>()){
                using internalType  = decltype(DataType::x);
                return h5pp::Type::Complex::H5T_SCALAR3<internalType>(data);
            }else{
                //This should never happen.
                throw::std::runtime_error("Unrecognized Scalar3 type: " +  std::string(typeid(data).name() ));
            }
        }

        template<typename DataType>
        auto getByteSize(const DataType &data){
            hsize_t num_elems = getSize(data);
            hsize_t typesize = sizeof(data);
            if constexpr (h5pp::Type::Check::hasMember_data<DataType>::value){typesize = sizeof(data.data()[0]);}
            if constexpr (h5pp::Type::Check::hasMember_c_str<DataType>::value){typesize = sizeof(data.c_str()[0]);}
            if constexpr (h5pp::Type::Check::hasMember_Scalar<DataType>::value){ typesize = sizeof(typename DataType::Scalar);}
            if constexpr (std::is_array<DataType>::value){typesize = sizeof(data[0]);}
            return num_elems*typesize;
        }

    }


}


