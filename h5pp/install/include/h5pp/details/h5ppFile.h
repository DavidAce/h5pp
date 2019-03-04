//
// Created by david on 2019-03-01.
//

#ifndef H5PP_H5PPFILE_H
#define H5PP_H5PPFILE_H
#include <hdf5.h>
#include <hdf5_hl.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <experimental/filesystem>
#include <experimental/type_traits>
#include <h5pp/details/h5ppTypeCheck.h>
#include <h5pp/details/nmspc_tensor_extra.h>

#include <h5pp/details/h5ppLogger.h>
#include <h5pp/details/h5ppType.h>
#include <h5pp/details/h5ppDatasetProperties.h>
#include <h5pp/details/h5ppAttributeProperties.h>


namespace fs = std::experimental::filesystem;
namespace tc = TypeCheck;

namespace h5pp{


/*!
 \brief Writes and reads data to a binary hdf5-file.
*/

    class File {
    private:
        herr_t      retval;
        fs::path    outputFilename;
        fs::path    outputFolder;
        fs::path    outputFileFullPath;
        bool        createDir;
        bool        overwrite;
        bool        resume;

        std::shared_ptr<spdlog::logger> logger;
        void set_output_file_path();

        //Mpi related constants
        hid_t plist_facc;
        hid_t plist_xfer;
        hid_t plist_lncr;
        hid_t plist_lapl;

        struct H5T_COMPLEX_STRUCT {
            double real;   /*real part*/
            double imag;   /*imaginary part*/
        };
        hid_t H5T_COMPLEX_DOUBLE;
    public:
//    hid_t       file;
        hid_t open_file(){return H5Fopen(outputFileFullPath.c_str(), H5F_ACC_RDWR, plist_facc);}

        enum class FileMode {OPEN,TRUNCATE,RENAME};
        FileMode fileMode;
        explicit File(const std::string output_filename_, const std::string output_dirname_ ="", bool overwrite_ = false, bool resume_ = true,bool create_dir_ = true);

        ~File(){
            H5Pclose(plist_facc);
            H5Pclose(plist_xfer);
            H5Pclose(plist_lncr);
            H5Tclose(H5T_COMPLEX_DOUBLE);
        }

        std::string get_file_name(){return outputFileFullPath.filename().string();}
        std::string get_file_path(){return outputFileFullPath.string();}
        bool file_is_valid();
        bool file_is_valid(fs::path  some_hdf5_filename);
        fs::path get_new_filename(fs::path some_hdf5_filename);

        void write_symbolic_link(const std::string &src_path, const std::string &tgt_path);

        template <typename DataType>
        void write_dataset(const DataType &data, const std::string &dataset_relative_name);

        template <typename DataType>
        void read_dataset(DataType &data, const std::string &dataset_relative_name);


        void create_group_link(const std::string &group_relative_name);



        template <typename AttrType>
        void write_attribute_to_file(const AttrType &attribute, const std::string attribute_name);

        template <typename AttrType>
        void write_attribute_to_dataset(const std::string &dataset_relative_name, const AttrType &attribute,
                                        const std::string &attribute_name);


        template <typename AttrType>
        void write_attribute_to_group(const std::string &group_relative_name, const AttrType &attribute,
                                      const std::string &attribute_name);


        std::vector<std::string> print_contents_of_group(std::string group_name);


        bool link_exists(std::string link){
            hid_t file = open_file();
            bool exists = check_link_exists_recursively(file,link);
            H5Fclose(file);
            return exists;
        }

    private:
        void initialize();

        void extend_dataset(hid_t file,const std::string & dataset_relative_name, const int dim, const int extent );

        template<typename DataType>
        void extend_dataset(const DataType &data, const std::string & dataset_relative_name);

        void set_extent_dataset(hid_t file, const DatasetProperties &props);

        bool check_link_exists_recursively(hid_t file, std::string path);

        bool check_if_attribute_exists(hid_t file, const std::string &link_name,
                                       const std::string &attribute_name);

        void create_dataset_link(hid_t file, const DatasetProperties &props);

        void select_hyperslab(const hid_t &filespace, const hid_t &memspace);

        template <typename DataType>
        void write_dataset(const DataType &data, const DatasetProperties &props);

        template <typename AttrType>
        void write_attribute_to_dataset(const AttrType &attribute, const AttributeProperties &aprops);

        template <typename AttrType>
        void write_attribute_to_group(const AttrType &attribute, const AttributeProperties &aprops);

        template<typename DataType>
        std::vector<H5T_COMPLEX_STRUCT> convert_complex_data(const DataType &data);


        template<typename DataType>
        constexpr hid_t get_DataType() const {
            if constexpr (std::is_same<DataType, int>::value)                 {return  H5Tcopy(H5T_NATIVE_INT);}
            if constexpr (std::is_same<DataType, long>::value)                {return  H5Tcopy(H5T_NATIVE_LONG);}
            if constexpr (std::is_same<DataType, unsigned int>::value)        {return  H5Tcopy(H5T_NATIVE_UINT);}
            if constexpr (std::is_same<DataType, unsigned long>::value)       {return  H5Tcopy(H5T_NATIVE_ULONG);}
            if constexpr (std::is_same<DataType, double>::value)              {return  H5Tcopy(H5T_NATIVE_DOUBLE);}
            if constexpr (std::is_same<DataType, float>::value)               {return  H5Tcopy(H5T_NATIVE_FLOAT);}
            if constexpr (std::is_same<DataType, bool>::value)                {return  H5Tcopy(H5T_NATIVE_HBOOL);}
            if constexpr (std::is_same<DataType, std::complex<double>>::value){return  H5Tcopy(H5T_COMPLEX_DOUBLE);}
            if constexpr (std::is_same<DataType, H5T_COMPLEX_STRUCT>::value)  {return  H5Tcopy(H5T_COMPLEX_DOUBLE);}
            if constexpr (std::is_same<DataType, char>::value)                {return  H5Tcopy(H5T_C_S1);}
            if constexpr (std::is_same<DataType, const char *>::value)        {return  H5Tcopy(H5T_C_S1);}
            if constexpr (std::is_same<DataType, std::string>::value)         {return  H5Tcopy(H5T_C_S1);}
            if constexpr (tc::is_eigen_tensor<DataType>::value)               {return  get_DataType<typename DataType::Scalar>();}
            if constexpr (tc::is_eigen_matrix<DataType>::value)               {return  get_DataType<typename DataType::Scalar>();}
            if constexpr (tc::is_eigen_array<DataType>::value)                {return  get_DataType<typename DataType::Scalar>();}
            if constexpr (tc::is_vector<DataType>::value)                     {return  get_DataType<typename DataType::value_type>();}
            if constexpr (tc::has_member_scalar <DataType>::value)            {return  get_DataType<typename DataType::Scalar>();}
            if constexpr (tc::has_member_value_type <DataType>::value)        {return  get_DataType<typename DataType::value_type>();}
            logger->critical("get_DataType could not match the type provided");
            throw(std::logic_error("get_DataType could not match the type provided"));
        }



        template<typename DataType>
        hsize_t get_Size(const DataType &data)const{
            if constexpr (tc::has_member_size<DataType>::value)          {return data.size();} //Fails on clang?
            else if constexpr (std::is_arithmetic<DataType>::value)      {return 1;}
            else if constexpr (std::is_pod<DataType>::value)             {return 1;}
            else if constexpr (std::is_same<std::string,DataType>::value){return data.size();}
            else if constexpr (tc::is_eigen_tensor<DataType>::value)     {return data.size();}
            else if constexpr (tc::is_eigen_matrix<DataType>::value)     {return data.size();}
            else if constexpr (tc::is_eigen_array<DataType>::value)      {return data.size();}
            else if constexpr (tc::is_vector<DataType>::value)           {return data.size();}
            else{
                std::cerr << "WARNING: get_Size can't match the type provided: " << typeid(data).name() << '\n';
                return data.size();
            }
        }
        template<typename DataType>
        constexpr int get_Rank() const {
            if      constexpr (tc::is_eigen_tensor<DataType>::value){return (int) DataType::NumIndices;}
            else if constexpr (tc::is_eigen_matrix<DataType>::value){return 2; }
            else if constexpr (tc::is_eigen_array<DataType>::value){return 2; }
            else if constexpr (std::is_arithmetic<DataType>::value){return 1; }
            else if constexpr(tc::is_vector<DataType>::value){return 1;}
            else if constexpr(std::is_same<std::string, DataType>::value){return 1;}
            else if constexpr(std::is_same<const char *,DataType>::value){return 1;}
            else if constexpr(std::is_array<DataType>::value){return 1;}
            else {
                logger->critical("get_Rank can't match the type provided: {}" << tc::type_name<DataType>());
                tc::print_type_and_exit_compile_time<DataType>();
            }
        }


        hid_t get_DataSpace_unlimited(int rank) const{
            std::vector<hsize_t> dims(rank);
            std::vector<hsize_t> max_dims(rank);
            std::fill_n(dims.begin(), rank, 0);
            std::fill_n(max_dims.begin(), rank, H5S_UNLIMITED);
            return H5Screate_simple(rank, dims.data(), max_dims.data());
        }

        template<typename DataType>
        hid_t get_MemSpace(const DataType &data) const {
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


        template <typename DataType>
        std::vector<hsize_t> get_Dimensions(const DataType &data) const {
            int rank = get_Rank<DataType>();
            std::vector<hsize_t> dims(rank);
            if constexpr (tc::is_eigen_tensor<DataType>::value){
                std::copy(data.dimensions().begin(), data.dimensions().end(), dims.begin());
                return dims;
            }
            else if constexpr (std::is_arithmetic<DataType>::value){
                dims[0]={1};
                return dims;
            }
            else if constexpr (tc::is_eigen_matrix <DataType>::value) {
                dims[0] = (hsize_t) data.rows();
                dims[1] = (hsize_t) data.cols();
                return dims;
            }
            else if constexpr(tc::is_vector<DataType>::value){
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
//        else if constexpr(std::is_array<DataType>::value){
//            // Read more about this step here
//            //http://www.astro.sunysb.edu/mzingale/io_tutorial/HDF5_simple/hdf5_simple.c
////            dims[0]={data.size()};
//            dims[0]= 1;
//            return dims;
//        }
            else{
                tc::print_type_and_exit_compile_time<DataType>();
                std::string error = "get_Dimensions can't match the type provided: " + std::string(typeid(DataType).name());
                logger->critical(error);
                throw(std::logic_error(error));
            }

        }


    };
    
}





// Definitions






template<typename DataType>
void h5pp::File::extend_dataset(const DataType &data, const std::string & dataset_relative_name){
    if constexpr (tc::is_eigen_matrix_or_array<DataType>()){
        hid_t file = open_file();
        extend_dataset(dataset_relative_name, 0, data.rows());
        hid_t dataset   = H5Dopen(file, dataset_relative_name.c_str(), H5P_DEFAULT);
        hid_t filespace = H5Dget_space(dataset);
        int ndims = H5Sget_simple_extent_ndims(filespace);
        std::vector<hsize_t> dims(ndims);
        H5Sget_simple_extent_dims(filespace,dims.data(),NULL);
        H5Dclose(dataset);
        H5Sclose(filespace);
        if (dims[1] < (hsize_t) data.cols()){
            extend_dataset(dataset_relative_name, 1, data.cols());
        }
        H5Fclose(file);
    }
    else{
        extend_dataset(dataset_relative_name, 0, get_Size(data));
    }
}


template<typename DataType>
std::vector<h5pp::File::H5T_COMPLEX_STRUCT> h5pp::File::convert_complex_data(const DataType &data){

    if constexpr(tc::is_eigen_tensor<DataType>::value or tc::is_eigen_matrix_or_array<DataType>()){
        if constexpr(std::is_same<typename DataType::Scalar, std::complex<double>>::value) {
            std::vector<H5T_COMPLEX_STRUCT> new_data(data.size());
            for (int i = 0; i < data.size(); i++) {
                new_data[i].real = data(i).real();
                new_data[i].imag = data(i).imag();
            }
            return new_data;
        }
    }
    if constexpr(tc::is_vector<DataType>::value) {
        if constexpr(std::is_same<typename DataType::value_type, std::complex<double>>::value) {
            std::vector<H5T_COMPLEX_STRUCT> new_data(data.size());
            for (size_t i = 0; i < data.size(); i++) {
                new_data[i].real = data[i].real();
                new_data[i].imag = data[i].imag();
            }
            return new_data;
        }
    }

    if constexpr(std::is_arithmetic<DataType>::value and std::is_same<DataType, std::complex<double>>::value){
        std::vector<H5T_COMPLEX_STRUCT> new_data(1);
        new_data[0].real = data.real();
        new_data[0].imag = data.imag();
        return new_data;
    }
    //This should never happen, but is here so that we compile successfully.
    assert(NAN == NAN and "Big error! Tried to convert non-complex data to complex");
    return     std::vector<H5T_COMPLEX_STRUCT>();
}


template <typename DataType>
void h5pp::File::write_dataset(const DataType &data, const DatasetProperties &props){
    hid_t file = open_file();
    create_dataset_link(file,props);
    set_extent_dataset(file,props);
    hid_t dataset   = H5Dopen(file,props.dset_name.c_str(), H5P_DEFAULT);
    hid_t filespace = H5Dget_space(dataset);
    select_hyperslab(filespace,props.memspace);
    if constexpr (tc::has_member_c_str<DataType>::value){
//        retval = H5Dwrite(dataset, props.datatype, H5S_ALL, filespace,
//                       H5P_DEFAULT, data.c_str());
        retval = H5Dwrite(dataset, props.datatype, props.memspace, filespace, H5P_DEFAULT, data.c_str());
        if(retval < 0) throw std::runtime_error("Failed to write text to file");
    }
    else if constexpr(tc::has_member_data<DataType>::value){
        retval = H5Dwrite(dataset, props.datatype, props.memspace, filespace, H5P_DEFAULT, data.data());
        if(retval < 0) throw std::runtime_error("Failed to write data to file");
    }
    else if constexpr(std::is_arithmetic<DataType>::value){
        retval = H5Dwrite(dataset, props.datatype, props.memspace, filespace, H5P_DEFAULT, &data);
        if(retval < 0) throw std::runtime_error("Failed to write number to file");
    }
    H5Dclose(dataset);
    H5Fflush(file,H5F_SCOPE_GLOBAL);
    H5Fclose(file);
}

template <typename DataType>
void h5pp::File::write_dataset(const DataType &data, const std::string &dataset_relative_name){
    DatasetProperties props;
    props.datatype   = get_DataType<DataType>();
    props.memspace   = get_MemSpace(data);
    props.size       = get_Size<DataType>(data);
    props.dset_name  = dataset_relative_name;
    props.ndims      = get_Rank<DataType>();
    props.dims       = get_Dimensions<DataType>(data);
    props.chunk_size = props.dims;

    if constexpr(tc::is_eigen_tensor<DataType>::value or tc::is_eigen_matrix_or_array<DataType>()){
        auto temp = Textra::to_RowMajor(data); //Convert to Row Major first;
        if (H5Tequal(get_DataType<DataType>(), H5T_COMPLEX_DOUBLE) and not std::is_same<std::vector<H5T_COMPLEX_STRUCT>, DataType>::value) {
            //If complex, convert data to complex struct H5T_COMPLEX_DOUBLE
            auto new_temp = convert_complex_data(temp);
            write_dataset(new_temp, props);
        }else{
            write_dataset(temp, props);
        }
    }else{
        if(H5Tequal(props.datatype, H5T_C_S1)){
            // Read more about this step here
            //http://www.astro.sunysb.edu/mzingale/io_tutorial/HDF5_simple/hdf5_simple.c
            retval = H5Tset_size  (props.datatype, props.size);
            retval = H5Tset_strpad(props.datatype,H5T_STR_NULLPAD);
            write_dataset(data, props);
        }else if (H5Tequal(get_DataType<DataType>(), H5T_COMPLEX_DOUBLE) and not std::is_same<std::vector<H5T_COMPLEX_STRUCT>, DataType>::value) {
            //If complex, convert data to complex struct H5T_COMPLEX_DOUBLE
            auto new_data = convert_complex_data(data);
            write_dataset(new_data, props);
        }else{
            write_dataset(data, props);
        }
    }
}


template <typename DataType>
void h5pp::File::read_dataset(DataType &data, const std::string &dataset_relative_name){
    hid_t file = open_file();
    if (check_link_exists_recursively(file,dataset_relative_name)) {
        try{
            hid_t dataset   = H5Dopen(file, dataset_relative_name.c_str(), H5P_DEFAULT);
            hid_t memspace  = H5Dget_space(dataset);
            hid_t datatype  = H5Dget_type(dataset);
            int ndims       = H5Sget_simple_extent_ndims(memspace);
            std::vector<hsize_t> dims(ndims);
            H5Sget_simple_extent_dims(memspace, dims.data(), NULL);
            if constexpr(tc::is_eigen_matrix_or_array<DataType>()) {
                Eigen::Matrix<typename DataType::Scalar, Eigen::Dynamic, Eigen::Dynamic,Eigen::RowMajor> matrix_rowmajor;
                matrix_rowmajor.resize(dims[0], dims[1]); // Data is transposed in HDF5!
                H5LTread_dataset(file, dataset_relative_name.c_str(), datatype, matrix_rowmajor.data());
                data = matrix_rowmajor;
            }
            else if constexpr(tc::is_eigen_tensor<DataType>()){
                Eigen::DSizes<long, DataType::NumDimensions> test;
                // Data is rowmajor in HDF5, so we need to convert back to ColMajor.
                Eigen::Tensor<typename DataType::Scalar,DataType::NumIndices, Eigen::RowMajor> tensor_rowmajor;
                std::copy(dims.begin(),dims.end(),test.begin());
                tensor_rowmajor.resize(test);
                H5LTread_dataset(file, dataset_relative_name.c_str(), datatype, tensor_rowmajor.data());
                data = Textra::to_ColMajor(tensor_rowmajor);
            }

            else if constexpr(tc::is_vector<DataType>::value) {
                assert(ndims == 1 and "Vector cannot take 2D datasets");
                data.resize(dims[0]);
                H5LTread_dataset(file, dataset_relative_name.c_str(), datatype, data.data());
            }
            else if constexpr(std::is_same<std::string,DataType>::value) {
                assert(ndims == 1 and "std string needs to have 1 dimension");
                hsize_t stringsize  = H5Dget_storage_size(dataset);
                data.resize(stringsize);
                H5LTread_dataset(file, dataset_relative_name.c_str(), datatype, data.data());
            }
            else if constexpr(std::is_arithmetic<DataType>::value){
                H5LTread_dataset(file, dataset_relative_name.c_str(), datatype, &data);
            }else{
                std::cerr << "Attempted to read dataset of unknown type: " << dataset_relative_name << "[" << typeid(data).name() << "]" << std::endl;
                exit(1);
            }
            H5Dclose(dataset);
            H5Sclose(memspace);
            H5Tclose(datatype);
        }catch(std::exception &ex){
            throw std::runtime_error("Failed to read dataset [ " + dataset_relative_name + " ] :" + std::string(ex.what()) );
        }
    }
    else{
        std::cerr << "Attempted to read dataset that doesn't exist: " << dataset_relative_name << std::endl;
    }
    H5Fclose(file);
}





template <typename AttrType>
void h5pp::File::write_attribute_to_file(const AttrType &attribute, const std::string attribute_name){
    hid_t file = open_file();
    hid_t datatype          = get_DataType<AttrType>();
    hid_t memspace          = get_MemSpace(attribute);
    auto size               = get_Size(attribute);
    if constexpr (tc::has_member_c_str<AttrType>::value){
        retval                  = H5Tset_size(datatype, size);
        retval                  = H5Tset_strpad(datatype,H5T_STR_NULLTERM);
    }

    hid_t attribute_id      = H5Acreate(file, attribute_name.c_str(), datatype, memspace, H5P_DEFAULT, H5P_DEFAULT );
    if constexpr (tc::has_member_c_str<AttrType>::value){
        retval                  = H5Awrite(attribute_id, datatype, attribute.c_str());
    }
    else if constexpr (tc::has_member_data<AttrType>::value){
        retval                  = H5Awrite(attribute_id, datatype, attribute.data());
    }
    else{
        retval                  = H5Awrite(attribute_id, datatype, &attribute);
    }

    H5Sclose(memspace);
    H5Tclose(datatype);
    H5Aclose(attribute_id);
    H5Fclose(file);
}



template <typename AttrType>
void h5pp::File::write_attribute_to_dataset(const AttrType &attribute, const AttributeProperties &aprops){
    hid_t file = open_file();
    if (check_link_exists_recursively(file,aprops.link_name) ) {
        if (not check_if_attribute_exists(file,aprops.link_name,aprops.attr_name)) {
            hid_t dataset = H5Dopen(file, aprops.link_name.c_str(), H5P_DEFAULT);
            hid_t attribute_id = H5Acreate(dataset, aprops.attr_name.c_str(), aprops.datatype, aprops.memspace,
                                           H5P_DEFAULT, H5P_DEFAULT);

            if constexpr (tc::has_member_c_str<AttrType>::value) {
                retval = H5Awrite(attribute_id, aprops.datatype, attribute.c_str());
            } else if constexpr (tc::has_member_data<AttrType>::value) {
                retval = H5Awrite(attribute_id, aprops.datatype, attribute.data());
            } else {
                retval = H5Awrite(attribute_id, aprops.datatype, &attribute);
            }

            H5Dclose(dataset);
            H5Aclose(attribute_id);
            H5Fflush(file, H5F_SCOPE_GLOBAL);
        }
    }
    else{
        std::string error = "Link " + aprops.link_name + " does not exist, yet attribute is being written.";
        H5Fclose(file);
        logger->critical(error);
        throw(std::logic_error(error));
    }
    H5Fclose(file);
}


template <typename AttrType>
void h5pp::File::write_attribute_to_dataset(const std::string &dataset_relative_name, const AttrType &attribute,
                                                 const std::string &attribute_name){
    AttributeProperties aprops;
    aprops.datatype  = get_DataType<AttrType>();
    aprops.memspace  = get_MemSpace(attribute);
    aprops.size      = get_Size(attribute);
    aprops.ndims     = get_Rank<AttrType>();
    aprops.dims      = get_Dimensions(attribute);
    aprops.attr_name = attribute_name;
    aprops.link_name = dataset_relative_name;
    if constexpr (tc::has_member_c_str<AttrType>::value){
        retval                  = H5Tset_size(aprops.datatype, aprops.size);
        retval                  = H5Tset_strpad(aprops.datatype,H5T_STR_NULLTERM);
    }
    write_attribute_to_dataset(attribute,aprops);
}

template <typename AttrType>
void h5pp::File::write_attribute_to_group(const AttrType &attribute,
                                               const AttributeProperties &aprops){
    hid_t file = open_file();
    if (check_link_exists_recursively(file,aprops.link_name)) {
        if (not check_if_attribute_exists(file,aprops.link_name, aprops.attr_name)){
            hid_t group = H5Gopen(file, aprops.link_name.c_str(), H5P_DEFAULT);
            hid_t attribute_id = H5Acreate(group, aprops.link_name.c_str(), aprops.datatype, aprops.memspace, H5P_DEFAULT, H5P_DEFAULT);
            retval = H5Awrite(attribute_id, aprops.datatype, &attribute);
            H5Gclose(group);
            H5Aclose(attribute_id);
            H5Fflush(file,H5F_SCOPE_GLOBAL);
        }
    }
    H5Fclose(file);
}
template <typename AttrType>
void h5pp::File::write_attribute_to_group(const std::string &group_relative_name,
                                               const AttrType &attribute,
                                               const std::string &attribute_name){

    AttributeProperties aprops;
    aprops.datatype  = get_DataType<AttrType>();
    aprops.memspace  = get_MemSpace(attribute);
    aprops.size      = get_Size(attribute);
    aprops.attr_name = attribute_name;
    aprops.link_name = group_relative_name;
    aprops.ndims     = get_Rank<AttrType>();
    aprops.dims      = get_Dimensions(attribute);
    write_attribute_to_dataset(attribute,aprops);
}














#endif //H5PP_H5PPFILE_H
