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
#include <h5pp/details/h5ppUtils.h>
#include <h5pp/details/h5ppType.h>
#include <h5pp/details/h5ppTypeCheck.h>
#include <h5pp/details/h5ppDatasetProperties.h>
#include <h5pp/details/h5ppAttributeProperties.h>
#include <h5pp/details/h5ppHdf5.h>



namespace h5pp{

    namespace fs = std::experimental::filesystem;
    namespace tc = h5pp::Type::Check;
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

//        std::shared_ptr<spdlog::logger> logger;
        void set_output_file_path();

        //Mpi related constants
        hid_t plist_facc;
        hid_t plist_xfer;
        hid_t plist_lncr;
        hid_t plist_lapl;

    public:
//    hid_t       file;
        hid_t open_file(){return H5Fopen(outputFileFullPath.c_str(), H5F_ACC_RDWR, plist_facc);}
        herr_t close_file(hid_t file){return H5Fclose(file);}

        enum class FileMode {OPEN,TRUNCATE,RENAME};
        FileMode fileMode;
        explicit File(const std::string output_filename_, const std::string output_dirname_ ="", bool overwrite_ = false, bool resume_ = true,bool create_dir_ = true);

        ~File(){
            H5Pclose(plist_facc);
            H5Pclose(plist_xfer);
            H5Pclose(plist_lncr);
            h5pp::Type::Complex::closeTypes();
        }

        std::string get_file_name(){return outputFileFullPath.filename().string();}
        std::string get_file_path(){return outputFileFullPath.string();}

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


        bool check_if_link_exists_recursively(std::string link){
            hid_t file = open_file();
            bool exists = h5pp::Hdf5::check_if_link_exists_recursively(file, link);
            H5Fclose(file);
            return exists;
        }

    private:
        void initialize();

        bool file_is_valid();
        bool file_is_valid(fs::path  some_hdf5_filename);
        fs::path get_new_filename(fs::path some_hdf5_filename);



        void create_dataset_link(hid_t file, const DatasetProperties &props);

        void select_hyperslab(const hid_t &filespace, const hid_t &memspace);

        template <typename DataType>
        void write_dataset(const DataType &data, const DatasetProperties &props);

        template <typename AttrType>
        void write_attribute_to_dataset(const AttrType &attribute, const AttributeProperties &aprops);

        template <typename AttrType>
        void write_attribute_to_group(const AttrType &attribute, const AttributeProperties &aprops);
    };
    
}



template <typename DataType>
void h5pp::File::write_dataset(const DataType &data, const DatasetProperties &props){
    hid_t file = open_file();
    create_dataset_link(file,props);
    h5pp::Hdf5::set_extent_dataset(file,props);
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
    else{
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
    props.datatype   = h5pp::Type::get_DataType<DataType>();
    props.memspace   = h5pp::Utils::get_MemSpace(data);
    props.size       = h5pp::Utils::get_Size<DataType>(data);
    props.ndims      = h5pp::Utils::get_Rank<DataType>();
    props.dims       = h5pp::Utils::get_Dimensions<DataType>(data);
    props.chunk_size = props.dims;
    props.dset_name  = dataset_relative_name;



    if constexpr(h5pp::Type::Check::hasStdComplex<DataType>() or h5pp::Type::Check::isStdComplex<DataType>()) {
        if constexpr(tc::is_eigen_type<DataType>::value) {
            auto temp_rowm = Textra::to_RowMajor(data); //Convert to Row Major first;
            auto temp_cplx = h5pp::Utils::convertComplexDataToH5T(temp_rowm); // Convert to vector<H5T_COMPLEX_STRUCT<>>
            write_dataset(temp_cplx, props);
        } else {
            auto temp_cplx = h5pp::Utils::convertComplexDataToH5T(data);
            write_dataset(temp_cplx, props);
        }
    }

    else{
        if constexpr(tc::is_eigen_type<DataType>::value) {
            auto temp_rowm = Textra::to_RowMajor(data); //Convert to Row Major first;
            write_dataset(temp_rowm, props);
        } else {
            if(H5Tequal(props.datatype, H5T_C_S1)){
                // Read more about this step here
                //http://www.astro.sunysb.edu/mzingale/io_tutorial/HDF5_simple/hdf5_simple.c
                retval = H5Tset_size  (props.datatype, props.size);
                retval = H5Tset_strpad(props.datatype,H5T_STR_NULLPAD);
                write_dataset(data, props);
            }else{
                write_dataset(data, props);
            }
        }
    }

}


template <typename DataType>
void h5pp::File::read_dataset(DataType &data, const std::string &dataset_relative_name){
    hid_t file = open_file();
    if (h5pp::Hdf5::check_if_link_exists_recursively(file, dataset_relative_name)) {
        try{
            hid_t dataset   = H5Dopen(file, dataset_relative_name.c_str(), H5P_DEFAULT);
            hid_t memspace  = H5Dget_space(dataset);
            hid_t datatype  = H5Dget_type(dataset);
            int ndims       = H5Sget_simple_extent_ndims(memspace);
            std::vector<hsize_t> dims(ndims);
            H5Sget_simple_extent_dims(memspace, dims.data(), NULL);
            if constexpr(tc::is_eigen_core<DataType>::value) {
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
    hid_t datatype          = h5pp::Type::get_DataType<AttrType>();
    hid_t memspace          = h5pp::Utils::get_MemSpace(attribute);
    auto size               = h5pp::Utils::get_Size(attribute);
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
    if (h5pp::Hdf5::check_if_link_exists_recursively(file, aprops.link_name) ) {
        if (not h5pp::Hdf5::check_if_attribute_exists(file,aprops.link_name,aprops.attr_name)) {
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
        spdlog::critical(error);
        throw(std::logic_error(error));
    }
    H5Fclose(file);
}


template <typename AttrType>
void h5pp::File::write_attribute_to_dataset(const std::string &dataset_relative_name, const AttrType &attribute,
                                                 const std::string &attribute_name){
    AttributeProperties aprops;
    aprops.datatype  = h5pp::Type::get_DataType<AttrType>();
    aprops.memspace  = h5pp::Utils::get_MemSpace(attribute);
    aprops.size      = h5pp::Utils::get_Size(attribute);
    aprops.ndims     = h5pp::Utils::get_Rank<AttrType>();
    aprops.dims      = h5pp::Utils::get_Dimensions(attribute);
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
    if (h5pp::Hdf5::check_if_link_exists_recursively(file, aprops.link_name)) {
        if (not h5pp::Hdf5::check_if_attribute_exists(file,aprops.link_name, aprops.attr_name)){
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
    aprops.datatype  = h5pp::Type::get_DataType<AttrType>();
    aprops.memspace  = h5pp::Utils::get_MemSpace(attribute);
    aprops.size      = h5pp::Utils::get_Size(attribute);
    aprops.attr_name = attribute_name;
    aprops.link_name = group_relative_name;
    aprops.ndims     = h5pp::Utils::get_Rank<AttrType>();
    aprops.dims      = h5pp::Utils::get_Dimensions(attribute);
    write_attribute_to_dataset(attribute,aprops);
}














#endif //H5PP_H5PPFILE_H
