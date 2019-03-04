//
// Created by david on 2019-03-04.
//

#ifndef H5PP_INFO_H
#define H5PP_INFO_H
#include <spdlog/spdlog.h>
#include <h5pp/details/h5ppType.h>
#include <h5pp/details/h5ppTypeCheck.h>



namespace h5pp{
    namespace Utils{

        template <typename T>
        auto complex_vector_to_rowmajor(const std::vector<std::complex<T>> &data){
            std::vector<std::complex<T>> new_data = data;
//            auto it1 = data.begin();
//            auto it2 = data.begin();
//            std::advance(it2,1);
//            while(true){
//                new_data.emplace_back(it1->real(), it2->real());
//                std::advance(it1,2);
//                if(it1 == data.end()){break;}
//                std::advance(it2,2);
//                if(it2 == data.end()){break;}
//            }
//            it1 = data.begin();
//            it2 = data.begin();
//            std::advance(it2,1);
//            while(true){
//                new_data.emplace_back(it1->imag(), it2->imag());
//                std::advance(it1,2);
//                if(it1 == data.end()){break;}
//                std::advance(it2,2);
//                if(it2 == data.end()){break;}
//            }

//            while (i < data.size() and j < new_data.size()) {
////                new_data[j].real() = data[i++].real();
////                new_data[j].imag() = data[i++].real();
//                new_data[j] = std::complex<T>(data[i].real(), data[i+1].real());
//                i +=2;
//            }
//            i = 0;
//            while (i < data.size() and  j < new_data.size()) {
////                new_data[j].real() = data[i++].imag();
////                new_data[j].imag() = data[i++].imag();
//                new_data[j] = std::complex<T>(data[i].imag(), data[i+1].imag());
//                j++;
//                i +=2;
//            }
            return new_data;

        }



        template<typename DataType>
        auto convert_complex_data(const DataType &data){
            static_assert(h5pp::Type::Check::isComplex<DataType>());
//            if constexpr (not h5pp::Type::isComplex<DataType>()){
//                throw std::logic_error("Tried to convert non-complex data to complex: " + std::string(typeid(data).name()));
//            }

            if constexpr(h5pp::Type::Check::is_eigen_tensor<DataType>::value or h5pp::Type::Check::is_eigen_matrix_or_array<DataType>()){
                std::vector<h5pp::Type::Complex::H5T_COMPLEX_STRUCT<typename DataType::Scalar>> new_data(data.size());
                for (int i = 0; i < data.size(); i++) {
                    new_data[i].real = data(i).real();
                    new_data[i].imag = data(i).imag();
                }
                return new_data;
//                if constexpr(std::is_same<typename DataType::Scalar, std::complex<double>>::value) {
//
//                }
            }
            else if constexpr(h5pp::Type::Check::is_vector<DataType>::value) {
                using vectorType = typename DataType::value_type;
                using scalarType = typename vectorType::value_type;
                std::vector<h5pp::Type::Complex::H5T_COMPLEX_STRUCT<scalarType>> new_data(data.size());
                for (size_t i = 0; i < data.size(); i++) {
                    new_data[i].real = data[i].real();
                    new_data[i].imag = data[i].imag();
                    std::cout << "real: "  << data[i].real() << " imag: "  << data[i].imag() << std::endl;
                }
                return new_data;

//                if constexpr(std::is_same<typename DataType::value_type, std::complex<double>>::value) {

//                }
            }

            else if  constexpr (std::is_arithmetic<DataType>::value){
                std::vector<h5pp::Type::Complex::H5T_COMPLEX_STRUCT<DataType>> new_data(1);
                new_data[0].real = data.real();
                new_data[0].imag = data.imag();
                return new_data;
            }else{
                //This should never happen, but is here so that we compile successfully.
                assert(NAN == NAN and "Big error! Tried to convert non-complex data to complex");
                return     std::vector<h5pp::Type::Complex::H5T_COMPLEX_STRUCT<std::complex<double>>>();
            }

        }



    }
}


#endif //H5PP_INFO_H
