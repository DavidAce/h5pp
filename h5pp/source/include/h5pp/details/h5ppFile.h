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
#include "h5ppTypeCheck.h"
#include "nmspc_tensor_extra.h"
#include "h5ppLogger.h"
#include "h5ppUtils.h"
#include "h5ppType.h"
#include "h5ppTypeCheck.h"
#include "h5ppDatasetProperties.h"
#include "h5ppAttributeProperties.h"
#include "h5ppHdf5.h"
#include "h5ppInit.h"



namespace h5pp{

    namespace fs = std::experimental::filesystem;
    namespace tc = h5pp::Type::Check;
/*!
 \brief Writes and reads data to a binary hdf5-file.
*/
    enum class AccessMode {OPEN,TRUNCATE,RENAME};

    class File {
    private:
        herr_t      retval;
        fs::path    outputFilename;
        fs::path    outputDir;
        fs::path    outputFileFullPath;
        bool        createDir;
        size_t      logLevel;

        //Mpi related constants
        hid_t plist_facc;
        hid_t plist_xfer;
        hid_t plist_lncr;
        hid_t plist_lapl;
        hid_t open_file(){return H5Fopen(outputFileFullPath.c_str(), H5F_ACC_RDWR, plist_facc);}
        herr_t close_file(hid_t file){return H5Fclose(file);}

    public:
//    hid_t       file;

        AccessMode accessMode;
//        explicit File(const std::string output_filename_, const std::string output_dirname_ ="", bool overwrite_ = false, bool resume_ = true,bool create_dir_ = true);

        File(const std::string outputFilename_, const std::string outputDir_="", AccessMode accessMode_ = AccessMode::TRUNCATE, bool createOutDir_=true, size_t logLevel_ = 3) {
            logLevel = logLevel_;
            h5pp::Logger::setLogger("h5pp",logLevel,false);
            accessMode          = accessMode_;
            outputFilename      = outputFilename_;
            outputDir           = outputDir_;
            createDir           = createOutDir_;


            /*
            * Check if zlib compression is available and can be used for both
            * compression and decompression.  Normally we do not perform error
            * checking in these examples for the sake of clarity, but in this
            * case we will make an exception because this filter is an
            * optional part of the hdf5 library.
            */
            herr_t          status;
            htri_t          avail;
            H5Z_filter_t    filter_type;
            unsigned int    flags, filter_info;
            avail = H5Zfilter_avail(H5Z_FILTER_DEFLATE);
            if (!avail) {
                spdlog::warn("zlib filter not available");
            }
            status = H5Zget_filter_info (H5Z_FILTER_DEFLATE, &filter_info);
            if ( !(filter_info & H5Z_FILTER_CONFIG_ENCODE_ENABLED) ||
                 !(filter_info & H5Z_FILTER_CONFIG_DECODE_ENABLED) ) {
                spdlog::warn("zlib filter not available for encoding and decoding");
            }

            initialize();

        }


        ~File(){
            H5Pclose(plist_facc);
            H5Pclose(plist_xfer);
            H5Pclose(plist_lncr);
            h5pp::Type::Complex::closeTypes();
        }

        void setLogLevel(size_t logLevelZeroToSix){
            logLevel = logLevelZeroToSix;
            h5pp::Logger::setLogLevel(logLevelZeroToSix);
        };



        std::string get_file_name(){return outputFileFullPath.filename().string();}
        std::string get_file_path(){return outputFileFullPath.string();}


        template <typename DataType>
        void read_dataset(DataType &data, const std::string &dataset_relative_name);

        template <typename DataType>
        void write_dataset(const DataType &data, const std::string &dataset_relative_name);

        template <typename DataType>
        void write_dataset(const DataType &data, const DatasetProperties &props);

        template <typename AttrType>
        void write_attribute_to_link(const AttrType &attribute, const AttributeProperties &aprops);

        template <typename AttrType>
        void write_attribute_to_link(const AttrType &attribute, const std::string &attribute_name,  const std::string &link_name);

        template <typename AttrType>
        void write_attribute_to_file(const AttrType &attribute, const std::string attribute_name);



        inline void create_group_link(const std::string &group_relative_name){
            hid_t file = open_file();
            h5pp::Hdf5::create_group_link(file,plist_lncr,group_relative_name);
            close_file(file);
        }

//        void create_group_link(const std::string &group_relative_name);
        inline void write_symbolic_link(const std::string &src_path, const std::string &tgt_path){
            hid_t file = open_file();
            h5pp::Hdf5::write_symbolic_link(file,src_path, tgt_path);
            close_file(file);
        }


        bool check_if_link_exists_recursively(std::string link){
            hid_t file = open_file();
            bool exists = h5pp::Hdf5::check_if_link_exists_recursively(file, link);
            H5Fclose(file);
            return exists;
        }



//
//        template <typename AttrType>
//        void write_attribute_to_group(const AttrType &attribute, const std::string &attribute_name, const std::string &link_name);
//


        std::vector<std::string> print_contents_of_group(std::string group_name);



    private:

        bool file_is_valid(){
            return file_is_valid(outputFileFullPath);
        }

        bool file_is_valid(fs::path some_hdf5_filename) {
            if (fs::exists(some_hdf5_filename)){
                if (H5Fis_hdf5(outputFileFullPath.c_str()) > 0) {
                    return true;
                } else {
                    return false;
                }
            }else{
                return false;
            }
        }



        fs::path get_new_filename(fs::path some_hdf5_filename){
            int i=1;
            fs::path some_new_hdf5_filename = some_hdf5_filename;
            while (fs::exists(some_new_hdf5_filename)){
                some_new_hdf5_filename.replace_filename(some_hdf5_filename.stem().string() + "-" + std::to_string(i++) + some_hdf5_filename.extension().string() );
            }
            return some_new_hdf5_filename;
        }


        void create_dataset_link(hid_t file, const DatasetProperties &props){
            h5pp::Hdf5::create_dataset_link(file,plist_lncr,props);
        }

        void select_hyperslab(const hid_t &filespace, const hid_t &memspace){
            h5pp::Hdf5::select_hyperslab(filespace,memspace);
        }



//
//        template <typename AttrType>
//        void write_attribute_to_group(const AttrType &attribute, const AttributeProperties &aprops);
//
//

        void initialize(){
            h5pp::Logger::setLogger("h5pp-init",logLevel,false);
            spdlog::debug("outputDir     : {}", outputDir.string() );
            plist_facc = H5Pcreate(H5P_FILE_ACCESS);
            plist_lncr = H5Pcreate(H5P_LINK_CREATE);   //Create missing intermediate group if they don't exist
            plist_xfer = H5Pcreate(H5P_DATASET_XFER);
            plist_lapl = H5Pcreate(H5P_LINK_ACCESS);
            H5Pset_create_intermediate_group(plist_lncr, 1);
            set_output_file_path();
            h5pp::Type::Complex::initTypes();
        }



        void set_output_file_path() {
            if (outputDir.empty()) {
                if (outputFilename.is_relative()) {
                    outputDir = fs::current_path();
                } else if (outputFilename.is_absolute()) {
                    outputDir = outputDir.stem();
                    outputFilename = outputFilename.filename();
                }
            }


            fs::path currentDir             = fs::current_path();
            fs::path outputDirAbs           = fs::system_complete(outputDir);
            spdlog::debug("currentDir        : {}", currentDir.string() );
            spdlog::debug("outputDir         : {}", outputDir.string() );
            spdlog::debug("outputDirAbs      : {}", outputDirAbs.string() );
            spdlog::debug("outputFilename    : {}", outputFilename.string() );


            if(createDir){
                if (fs::create_directories(outputDirAbs)){
                    outputDirAbs = fs::canonical(outputDirAbs);
                    spdlog::info("Created directory: {}",outputDirAbs.string());
                }else{
                    spdlog::info("Directory already exists: {}",outputDirAbs.string());
                }
            }else{
                spdlog::critical("Target folder does not exist and creation of directory is disabled in settings");
            }
            outputFileFullPath = fs::system_complete(outputDirAbs / outputFilename);


            switch (accessMode){
                case AccessMode::OPEN: {
                    spdlog::debug("File mode OPEN: {}", outputFileFullPath.string());
                    try{
                        if(file_is_valid(outputFileFullPath)){
                            hid_t file = open_file();
                            close_file(file);
                        }
                    }catch(std::exception &ex){
                        throw std::runtime_error("Failed to open hdf5 file :" + outputFileFullPath.string() );

                    }
                    break;
                }
                case AccessMode::TRUNCATE: {
                    spdlog::debug("File mode TRUNCATE: {}", outputFileFullPath.string());
                    try{
                        hid_t file = H5Fcreate(outputFileFullPath.c_str(), H5F_ACC_TRUNC,  H5P_DEFAULT, plist_facc);
                        H5Fclose(file);
                        file = open_file();
                        herr_t close_file(file);
                    }catch(std::exception &ex){
                        throw std::runtime_error("Failed to create hdf5 file :" + outputFileFullPath.string() );
                    }
                    break;
                }
                case AccessMode::RENAME: {
                    try{
                        spdlog::debug("File mode RENAME: {}", outputFileFullPath.string());
                        if(file_is_valid(outputFileFullPath)) {
                            outputFileFullPath =  get_new_filename(outputFileFullPath);
                            spdlog::info("Renamed output file: {} ---> {}", outputFilename.string(),outputFileFullPath.filename().string());
                            spdlog::info("File mode RENAME: {}", outputFileFullPath.string());
                        }
                        hid_t file = H5Fcreate(outputFileFullPath.c_str(), H5F_ACC_TRUNC,  H5P_DEFAULT, plist_facc);
                        H5Fclose(file);
                        file = open_file();
                        close_file(file);
                    }catch(std::exception &ex){
                        throw std::runtime_error("Failed to create renamed hdf5 file :" + outputFileFullPath.string() );
                    }
                    break;
                }
                default:{
                    spdlog::error("File Mode not set. Choose  AccessMode:: |OPEN|TRUNCATE|RENAME|");
                    throw std::runtime_error("File Mode not set. Choose  AccessMode::  |OPEN|TRUNCATE|RENAME|");
                }
            }

        }



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
void h5pp::File::write_attribute_to_link(const AttrType &attribute, const AttributeProperties &aprops){
    hid_t file = open_file();
    if (h5pp::Hdf5::check_if_link_exists_recursively(file, aprops.link_name) ) {
        if (not h5pp::Hdf5::check_if_attribute_exists(file,aprops.link_name,aprops.attr_name)) {
//            hid_t linkObject = H5Dopen(file, aprops.link_name.c_str(), H5P_DEFAULT);
            hid_t linkObject = H5Oopen(file, aprops.link_name.c_str(), H5P_DEFAULT);
            hid_t attribute_id = H5Acreate(linkObject, aprops.attr_name.c_str(), aprops.datatype, aprops.memspace,
                                           H5P_DEFAULT, H5P_DEFAULT);

            if constexpr (tc::has_member_c_str<AttrType>::value) {
                retval = H5Awrite(attribute_id, aprops.datatype, attribute.c_str());
            } else if constexpr (tc::has_member_data<AttrType>::value) {
                retval = H5Awrite(attribute_id, aprops.datatype, attribute.data());
            } else {
                retval = H5Awrite(attribute_id, aprops.datatype, &attribute);
            }

            H5Dclose(linkObject);
            H5Aclose(attribute_id);
            H5Fflush(file, H5F_SCOPE_GLOBAL);
            H5Fclose(file);
        }
    }
    else{
        H5Fclose(file);
        std::string error = "Link " + aprops.link_name + " does not exist, yet attribute is being written.";
        spdlog::critical(error);
        throw(std::logic_error(error));
    }
}


template <typename AttrType>
void h5pp::File::write_attribute_to_link(const AttrType &attribute, const std::string &attribute_name,  const std::string &link_name){
    AttributeProperties aprops;
    aprops.datatype  = h5pp::Type::get_DataType<AttrType>();
    aprops.memspace  = h5pp::Utils::get_MemSpace(attribute);
    aprops.size      = h5pp::Utils::get_Size(attribute);
    aprops.ndims     = h5pp::Utils::get_Rank<AttrType>();
    aprops.dims      = h5pp::Utils::get_Dimensions(attribute);
    aprops.attr_name = attribute_name;
    aprops.link_name = link_name;
    if constexpr (tc::has_member_c_str<AttrType>::value
                  or std::is_same<char * , typename std::decay<AttrType>::type>::value
    ){
        retval                  = H5Tset_size(aprops.datatype, aprops.size);
        retval                  = H5Tset_strpad(aprops.datatype,H5T_STR_NULLTERM);
    }
    write_attribute_to_link(attribute,aprops);
}











#endif //H5PP_H5PPFILE_H
