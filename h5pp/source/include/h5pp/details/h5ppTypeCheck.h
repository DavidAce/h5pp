//
// Created by david on 2018-02-06.
//

#ifndef H5PP_TYPECHECK_H
#define H5PP_TYPECHECK_H
#include <experimental/type_traits>
#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>
#include "h5ppTypeComplex.h"

namespace h5pp{
    namespace Type{
        namespace Check{

            template <typename T> using Data_t          = decltype(std::declval<T>().data());
            template <typename T> using Size_t          = decltype(std::declval<T>().size());
            template <typename T> using Cstr_t          = decltype(std::declval<T>().c_str());
            template <typename T> using Imag_t          = decltype(std::declval<T>().imag());
            template <typename T> using Scal_t          = typename T::Scalar;
            template <typename T> using Valt_t          = typename T::value_type;
            template <typename T> using x_t             = decltype(std::declval<T>().x);
            template <typename T> using y_t             = decltype(std::declval<T>().y);
            template <typename T> using z_t             = decltype(std::declval<T>().z);

            template <typename T> using hasMember_data         = std::experimental::is_detected<Data_t, T>;
            template <typename T> using hasMember_size         = std::experimental::is_detected<Size_t, T>;
            template <typename T> using hasMember_scalar       = std::experimental::is_detected<Scal_t , T>;
            template <typename T> using hasMember_value_type   = std::experimental::is_detected<Valt_t , T>;
            template <typename T> using hasMember_c_str        = std::experimental::is_detected<Cstr_t , T>;
            template <typename T> using hasMember_imag         = std::experimental::is_detected<Imag_t , T>;
            template <typename T> using hasMember_x            = std::experimental::is_detected<x_t    , T>;
            template <typename T> using hasMember_y            = std::experimental::is_detected<y_t    , T>;
            template <typename T> using hasMember_z            = std::experimental::is_detected<z_t    , T>;


            template<typename Test, template<typename...> class Ref>
            struct is_specialization : std::false_type {};
            template<template<typename...> class Ref, typename... Args>
            struct is_specialization<Ref<Args...>, Ref>: std::true_type {};


            template<typename T> struct is_vector : public std::false_type {};
            template<typename T> struct is_vector<std::vector<T>> : public std::true_type {};
//            template<typename T> constexpr bool is_vector(){return is_specialization<T,std::vector>::value;}

            template<typename T> struct is_eigen_tensor : public std::false_type {};
            template<typename Scalar, int rank, int storage, typename IndexType>
            struct is_eigen_tensor<Eigen::Tensor<Scalar, rank, storage,IndexType>> : public std::true_type{};
            template<typename Derived>
            struct is_eigen_tensor<Eigen::TensorMap<Derived>> : public std::true_type{};
            template<typename Derived>
            struct is_eigen_tensor<Eigen::TensorBase<Derived,Eigen::ReadOnlyAccessors>> : public std::true_type{};
            //////

            template<typename T> struct is_eigen_matrix : public std::false_type {};
            template<typename T, int rows, int cols, int StorageOrder> struct is_eigen_matrix<Eigen::Matrix<T,rows,cols,StorageOrder>> : public std::true_type {};

            template<typename T> struct is_eigen_array : public std::false_type {};
            template<typename T, int rows, int cols, int StorageOrder> struct is_eigen_array<Eigen::Array<T,rows,cols,StorageOrder>> : public std::true_type {};


            template<typename T> struct is_eigen_core : public std::false_type {};
            template<typename T, int rows, int cols, int StorageOrder> struct is_eigen_core<Eigen::Matrix<T,rows,cols,StorageOrder>> : public std::true_type {};
            template<typename T, int rows, int cols, int StorageOrder> struct is_eigen_core<Eigen::Array<T,rows,cols,StorageOrder>> : public std::true_type {};


            template<typename T> struct is_eigen_type : public std::false_type {};
            template<typename T, int rows, int cols, int StorageOrder> struct is_eigen_type<Eigen::Matrix<T,rows,cols,StorageOrder>> : public std::true_type {};
            template<typename T, int rows, int cols, int StorageOrder> struct is_eigen_type<Eigen::Array<T,rows,cols,StorageOrder>> : public std::true_type {};
            template<typename Scalar, int rank, int storage, typename IndexType> struct is_eigen_type<Eigen::Tensor<Scalar, rank, storage,IndexType>> : public std::true_type{};




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

            template<typename T>
            constexpr bool is_StdComplex(){
                return is_specialization<T,std::complex>::value;
            }

            template<typename T>
            constexpr bool hasStdComplex(){
                if constexpr (is_StdComplex<T>())                         {return false;}
                else if constexpr (hasMember_scalar <T>::value)         {return is_StdComplex<typename T::Scalar>();}
                else if constexpr (hasMember_value_type <T>::value)     {return is_StdComplex<typename T::value_type>();}
                return false;
            }

            template<typename T>
            constexpr bool is_Scalar2(){
                if constexpr(hasMember_x<T>::value and hasMember_y<T>::value){
                    constexpr size_t t_size = sizeof(T);
                    constexpr size_t x_size = sizeof(T::x);
                    constexpr size_t y_size = sizeof(T::y);
                    return t_size == x_size + y_size;
                }else{
                    return false;
                }
            }

            template<typename T1, typename T2>
            constexpr bool is_Scalar2_of_type(){
                if constexpr(is_Scalar2<T1>()){
                    return  std::is_same<decltype(T1::x),T2>::value;
                }else{
                    return false;
                }
           }

            template<typename T>
            constexpr bool hasScalar2(){
                if constexpr (is_Scalar2<T>())                          {return false;}
                else if constexpr (hasMember_scalar <T>::value)         {return is_Scalar2<typename T::Scalar>();}
                else if constexpr (hasMember_value_type <T>::value)     {return is_Scalar2<typename T::value_type>();}
                return false;
            }


            // Scalar 3
            template<typename T>
            constexpr bool is_Scalar3(){
                if constexpr(hasMember_x<T>::value and hasMember_y<T>::value and hasMember_z<T>::value){
                    constexpr size_t t_size = sizeof(T);
                    constexpr size_t x_size = sizeof(T::x);
                    constexpr size_t y_size = sizeof(T::y);
                    constexpr size_t z_size = sizeof(T::z);
                    return t_size == x_size + y_size + z_size;
                }else{
                    return false;
                }
            }

            template<typename T1, typename T2>
            constexpr bool is_Scalar3_of_type(){
                if constexpr(is_Scalar3<T1>()){
                    return  std::is_same<decltype(T1::x),T2>::value;
                }else{
                    return false;
                }
            }

            template<typename T>
            constexpr bool hasScalar3(){
                if constexpr (is_Scalar3<T>())                          {return false;}
                else if constexpr (hasMember_scalar <T>::value)         {return is_Scalar3<typename T::Scalar>();}
                else if constexpr (hasMember_value_type <T>::value)     {return is_Scalar3<typename T::value_type>();}
                return false;
            }

            template<typename T>
            constexpr bool is_H5T_COMPLEX_STRUCT(){
                return is_specialization<T,Complex::H5T_COMPLEX_STRUCT>::value;
            }

            template<typename T>
            constexpr bool is_H5T_SCALAR2(){
                return is_specialization<T,Complex::H5T_SCALAR2>::value;
            }

            template<typename T>
            constexpr bool is_H5T_SCALAR3(){
                return is_specialization<T,Complex::H5T_SCALAR3>::value;
            }


            template<typename T>
            constexpr bool isVectorOf_H5T_COMPLEX_STRUCT(){
                if constexpr (is_vector<T>::value and is_H5T_SCALAR2<T::value_type>()){return true;}
                return false;
            }




        }
    }
}
#endif //H5PP_TYPECHECK_H

