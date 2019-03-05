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


        template<typename DataType>
        hsize_t get_Size(const DataType &data){
            namespace tc = h5pp::Type::Check;
            if constexpr (tc::has_member_size<DataType>::value)          {return data.size();} //Fails on clang?
            else if constexpr (std::is_arithmetic<DataType>::value)      {return 1;}
            else if constexpr (std::is_pod<DataType>::value)             {return 1;}
            else if constexpr (tc::isStdComplex<DataType>())             {return 1;}
            else{
                spdlog::warn("WARNING: get_Size can't match the type provided: " + std::string(typeid(data).name()));
                return data.size();
            }
        }


        template<typename DataType>
        constexpr int get_Rank() {
            namespace tc = h5pp::Type::Check;
            if      constexpr(tc::is_eigen_tensor<DataType>::value){return (int) DataType::NumIndices;}
            else if constexpr(tc::is_eigen_core<DataType>::value){return 2; }
            else if constexpr(std::is_arithmetic<DataType>::value){return 1; }
            else if constexpr(tc::is_vector<DataType>::value){return 1;}
            else if constexpr(std::is_same<std::string, DataType>::value){return 1;}
            else if constexpr(std::is_same<const char *,DataType>::value){return 1;}
            else if constexpr(std::is_array<DataType>::value){return 1;}
            else if constexpr(std::is_pod<DataType>::value){return 1;}
            else if constexpr(tc::isStdComplex<DataType>()){return 1;}
            else {
                tc::print_type_and_exit_compile_time<DataType>();
            }
        }


        inline hid_t get_DataSpace_unlimited(int rank){
            std::vector<hsize_t> dims(rank);
            std::vector<hsize_t> max_dims(rank);
            std::fill_n(dims.begin(), rank, 0);
            std::fill_n(max_dims.begin(), rank, H5S_UNLIMITED);
            return H5Screate_simple(rank, dims.data(), max_dims.data());
        }





        template <typename DataType>
        std::vector<hsize_t> get_Dimensions(const DataType &data) {
            namespace tc = h5pp::Type::Check;
            int rank = get_Rank<DataType>();
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
            else if constexpr(std::is_array<DataType>::value){
                dims[0]={data.size()};
                return dims;
            }

            else if constexpr(std::is_same<std::string, DataType>::value){
                // Read more about this step here
                //http://www.astro.sunysb.edu/mzingale/io_tutorial/HDF5_simple/hdf5_simple.c
//            dims[0]={data.size()};
                dims[0]= 1;
                return dims;
            }
            else if constexpr(std::is_same<const char *, DataType>::value){
                // Read more about this step here
                //http://www.astro.sunysb.edu/mzingale/io_tutorial/HDF5_simple/hdf5_simple.c
//            dims[0]={data.size()};
                dims[0]= 1;
                return dims;
            }
            else if constexpr (std::is_arithmetic<DataType>::value or tc::isStdComplex<DataType>()){
                dims[0]= 1;
                return dims;
            }

            else{
                tc::print_type_and_exit_compile_time<DataType>();
                std::string error = "get_Dimensions can't match the type provided: " + std::string(typeid(DataType).name());
                spdlog::critical(error);
                throw(std::logic_error(error));
            }

        }

        template<typename DataType>
        hid_t get_MemSpace(const DataType &data) {
            auto rank = get_Rank<DataType>();
            auto dims = get_Dimensions<DataType>(data);
            if constexpr (std::is_same<std::string, DataType>::value){
                // Read more about this step here
                //http://www.astro.sunysb.edu/mzingale/io_tutorial/HDF5_simple/hdf5_simple.c
//            return H5Screate (H5S_SCALAR);

                return H5Screate_simple(rank, dims.data(), nullptr);
//            return get_DataSpace_unlimited(rank);
//            return H5Screate_simple(rank, dims.data(),maxdims);

            }else{
                return H5Screate_simple(rank, dims.data(), nullptr);
            }
        }


        template<typename DataType>
        auto convertComplexDataToH5T(const DataType &data){
            static_assert(h5pp::Type::Check::hasStdComplex<DataType>() or h5pp::Type::Check::isStdComplex<DataType>(),
                    "Data must be complex for conversion to H5T_COMPLEX_STRUCT");
            if constexpr(h5pp::Type::Check::is_eigen_type<DataType>::value){
                using scalarType  = typename DataType::Scalar;
                using complexType = typename scalarType::value_type;
                std::vector<h5pp::Type::Complex::H5T_COMPLEX_STRUCT<complexType>> new_data;
                new_data.insert(new_data.end(), data.data(), data.data()+data.size());
                return new_data;
            }
            else if constexpr(h5pp::Type::Check::is_vector<DataType>::value) {
                using scalarType  = typename DataType::value_type;
                using complexType = typename scalarType::value_type;
                std::vector<h5pp::Type::Complex::H5T_COMPLEX_STRUCT<complexType>> new_data;
                new_data.insert(new_data.end(), data.data(), data.data()+data.size());
                return new_data;
            }
            else if  constexpr (h5pp::Type::Check::isStdComplex<DataType>()){
                return h5pp::Type::Complex::H5T_COMPLEX_STRUCT<typename DataType::value_type>(data);
            }else{
                //This should never happen.
                throw::std::runtime_error("Unrecognized complex type: " +  std::string(typeid(data).name() ));
            }
        }



    }
}


#endif //H5PP_INFO_H
