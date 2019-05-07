//
// Created by david on 2019-03-04.
//

#ifndef H5PP_INFO_H
#define H5PP_INFO_H
#include <spdlog/spdlog.h>
#include "h5ppType.h"
#include "h5ppTypeCheck.h"



namespace h5pp{
    namespace Utils{
        template <typename DataType, size_t Size>
        constexpr size_t getArraySize(const DataType (&arr)[Size]){return Size;}

        inline herr_t setStringSize(hid_t datatype, hsize_t size){
            size = std::max((hsize_t) 1, size);
            herr_t retval = H5Tset_size(datatype, size);
            if(retval < 0){
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to set size: " + std::to_string(size));
            }
            retval  = H5Tset_strpad(datatype,H5T_STR_NULLTERM);
            if(retval < 0){
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to set strpad");
            }

        }




        template<typename DataType>
        hsize_t getSize(const DataType &data){
            namespace tc = h5pp::Type::Check;
            if constexpr (tc::hasMember_size<DataType>::value)          {return data.size();} //Fails on clang?
            else if constexpr (std::is_array<DataType>::value)          {return getArraySize(data);}
            else if constexpr (std::is_arithmetic<DataType>::value)     {return 1;}
            else if constexpr (std::is_pod<DataType>::value)            {return 1;}
            else if constexpr (tc::is_StdComplex<DataType>())           {return 1;}
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
            else if constexpr(tc::is_vector<DataType>::value){return 1;}
            else if constexpr(std::is_same<std::string, DataType>::value){return 1;}
            else if constexpr(std::is_same<const char *,DataType>::value){return 1;}
            else if constexpr(std::is_array<DataType>::value){return 1;}
            else if constexpr(std::is_pod<DataType>::value){return 1;}
            else if constexpr(tc::is_StdComplex<DataType>()){return 1;}
            else {
                tc::print_type_and_exit_compile_time<DataType>();
            }
        }


        inline hid_t getDataSpaceUnlimited(int rank){
            std::vector<hsize_t> dims(rank);
            std::vector<hsize_t> max_dims(rank);
            std::fill_n(dims.begin(), rank, 0);
            std::fill_n(max_dims.begin(), rank, H5S_UNLIMITED);
            return H5Screate_simple(rank, dims.data(), max_dims.data());
        }


        template <typename DataType>
        std::vector<hsize_t> getDimensions(const DataType &data) {
            namespace tc = h5pp::Type::Check;
            int rank = getRank<DataType>();
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
            else if constexpr(tc::is_vector<DataType>::value){
                dims[0]={data.size()};
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

            else if constexpr (std::is_arithmetic<DataType>::value or tc::is_StdComplex<DataType>()){
                dims[0]= 1;
                return dims;
            }

            else{
                tc::print_type_and_exit_compile_time<DataType>();
                std::string error = "getDimensions can't match the type provided: " + std::string(typeid(DataType).name());
                spdlog::critical(error);
                throw(std::logic_error(error));
            }

        }

        template<typename DataType>
        hid_t getMemSpace(const DataType &data) {
            auto rank = getRank<DataType>();
            auto dims = getDimensions<DataType>(data);
            return H5Screate_simple(rank, dims.data(), nullptr);
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
            else if constexpr(h5pp::Type::Check::is_vector<DataType>::value) {
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

    }
}


#endif //H5PP_INFO_H
