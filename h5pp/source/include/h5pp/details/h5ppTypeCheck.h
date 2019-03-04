//
// Created by david on 2018-02-06.
//

#ifndef NMSPC_TYPE_CHECK_H
#define NMSPC_TYPE_CHECK_H
#include <experimental/type_traits>
#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>
#include <h5pp/details/h5ppTypeComplex.h>

namespace h5pp{
    namespace Type{
        namespace Check{
            template <typename T> using Data_t          = decltype(std::declval<T>().data());
            template <typename T> using Size_t          = decltype(std::declval<T>().size());
            template <typename T> using Cstr_t          = decltype(std::declval<T>().c_str());
            template <typename T> using Imag_t          = decltype(std::declval<T>().imag());
            template <typename T> using Scal_t          = typename T::Scalar;
            template <typename T> using Valt_t          = typename T::value_type;

            template <typename T> using has_member_data         = std::experimental::is_detected<Data_t, T>;
            template <typename T> using has_member_size         = std::experimental::is_detected<Size_t, T>;
            template <typename T> using has_member_scalar       = std::experimental::is_detected<Scal_t , T>;
            template <typename T> using has_member_value_type   = std::experimental::is_detected<Valt_t , T>;
            template <typename T> using has_member_c_str        = std::experimental::is_detected<Cstr_t , T>;
            template <typename T> using has_member_imag         = std::experimental::is_detected<Imag_t , T>;


            template<typename T> struct is_vector : public std::false_type {};
            template<typename T> struct is_vector<std::vector<T>> : public std::true_type {};

            template<typename Test, template<typename...> class Ref>
            struct is_specialization : std::false_type {};
            template<template<typename...> class Ref, typename... Args>
            struct is_specialization<Ref<Args...>, Ref>: std::true_type {};

            template<typename T> struct is_eigen_tensor : public std::false_type {};
            template<typename Scalar, int rank, int storage, typename IndexType>
            struct is_eigen_tensor<Eigen::Tensor<Scalar, rank, storage,IndexType>> : public std::true_type{};


        //////

            template<typename T> struct is_eigen_matrix : public std::false_type {};
            template<typename T, int rows, int cols, int StorageOrder> struct is_eigen_matrix<Eigen::Matrix<T,rows,cols,StorageOrder>> : public std::true_type {};

            template<typename T> struct is_eigen_array : public std::false_type {};
            template<typename T, int rows, int cols, int StorageOrder> struct is_eigen_array<Eigen::Array<T,rows,cols,StorageOrder>> : public std::true_type {};

            template<typename T>
            constexpr bool is_eigen_matrix_or_array(){
                if constexpr(is_eigen_matrix<T>::value or
                             is_eigen_array<T>::value ){
                    return true;
                }else{
                    return false;
                }
            }

            template <typename...> struct print_type_and_exit_compile_time;


            template <class T>
            constexpr
            std::string_view
            type_name()
            {
                using namespace std;
                #ifdef __clang__
                string_view p = __PRETTY_FUNCTION__;
                return string_view(p.data() + 34, p.size() - 34 - 1);
                #elif defined(__GNUC__)
                string_view p = __PRETTY_FUNCTION__;
                #  if __cplusplus < 201402
                return string_view(p.data() + 36, p.size() - 36 - 1);
                #  else
                return string_view(p.data() + 49, p.find(';', 49) - 49);
                #  endif
                #elif defined(_MSC_VER)
                string_view p = __FUNCSIG__;
                return string_view(p.data() + 84, p.size() - 84 - 7);
                #endif
            }



            //This does not work for "non-type" class template parameters.
            //In fact it doesn't seem to work very well at all...
        //    template < template <typename...> class Template, typename T >
        //    struct is_instance_of : std::false_type {};
        //
        //    template < template <typename...> class Template, typename... Args >
        //    struct is_instance_of< Template, Template<Args...> > : std::true_type {};
        //
        //    template <typename T> using is_ofEigen              = is_instance_of<Eigen::EigenBase,T>;


    //        inline bool isComplexH5T(hid_t H5T_SOMETYPE){
    //            if      (H5Tequal(H5T_SOMETYPE, h5pp::Type::H5T_COMPLEX_INT)) {return true;}
    //            else if (H5Tequal(H5T_SOMETYPE, h5pp::Type::H5T_COMPLEX_LONG)) {return true;}
    //            else if (H5Tequal(H5T_SOMETYPE, h5pp::Type::H5T_COMPLEX_UINT)) {return true;}
    //            else if (H5Tequal(H5T_SOMETYPE, h5pp::Type::H5T_COMPLEX_ULONG)) {return true;}
    //            else if (H5Tequal(H5T_SOMETYPE, h5pp::Type::H5T_COMPLEX_DOUBLE)) {return true;}
    //            else if (H5Tequal(H5T_SOMETYPE, h5pp::Type::H5T_COMPLEX_FLOAT)) {return true;}
    //            else {return false;}
    //        }

            template<typename T>
            constexpr bool isComplex(){
    //            namespace tc = h5pp::TypeCheck;
                if constexpr (std::is_same<T, std::complex<int>>::value)           {return true;}
                if constexpr (std::is_same<T, std::complex<long>>::value)          {return true;}
                if constexpr (std::is_same<T, std::complex<unsigned int>>::value)  {return true;}
                if constexpr (std::is_same<T, std::complex<unsigned long>>::value) {return true;}
                if constexpr (std::is_same<T, std::complex<double>>::value)        {return true;}
                if constexpr (std::is_same<T, std::complex<float>>::value)         {return true;}
                if constexpr (is_eigen_tensor<T>::value)                           {return isComplex<typename T::Scalar>();}
                if constexpr (is_eigen_matrix<T>::value)                           {return isComplex<typename T::Scalar>();}
                if constexpr (is_eigen_array<T>::value)                            {return isComplex<typename T::Scalar>();}
                if constexpr (is_vector<T>::value)                                 {return isComplex<typename T::value_type>();}
                if constexpr (has_member_scalar <T>::value)                        {return isComplex<typename T::Scalar>();}
                if constexpr (has_member_value_type <T>::value)                    {return isComplex<typename T::value_type>();}
                return false;
            }



            template<typename T>
            constexpr bool isVectorOf_H5T_COMPLEX_STRUCT(){
                if constexpr (is_vector<T>::value){
                    //                if constexpr(is_specialization<h5pp::Type::H5T_COMPLEX_STRUCT>){return true;}
                    if constexpr(std::is_same<typename T::value_type, Complex::H5T_COMPLEX_STRUCT<int>>::value){return true;}
                    if constexpr(std::is_same<typename T::value_type, Complex::H5T_COMPLEX_STRUCT<long>>::value){return true;}
                    if constexpr(std::is_same<typename T::value_type, Complex::H5T_COMPLEX_STRUCT<unsigned int>>::value){return true;}
                    if constexpr(std::is_same<typename T::value_type, Complex::H5T_COMPLEX_STRUCT<unsigned long>>::value){return true;}
                    if constexpr(std::is_same<typename T::value_type, Complex::H5T_COMPLEX_STRUCT<double>>::value){return true;}
                    if constexpr(std::is_same<typename T::value_type, Complex::H5T_COMPLEX_STRUCT<float>>::value){return true;}
                    return false;
                }
            }

        }
    }
}
#endif //PT_NMSPC_TYPE_CHECK_H

