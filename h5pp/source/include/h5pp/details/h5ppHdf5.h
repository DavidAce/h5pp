//
// Created by david on 2019-03-05.
//

#ifndef H5PP_HDF5_H
#define H5PP_HDF5_H
#include <hdf5.h>
#include <spdlog/spdlog.h>
#include "h5ppTypeCheck.h"



namespace h5pp{
    namespace Hdf5{

        inline bool check_if_link_exists_recursively(hid_t file, std::string path){
            std::stringstream path_stream(path);
            std::vector<std::string> split_path;
            std::string part;
            while(std::getline(path_stream, part, '/')){
                if (not part.empty()) {
                    split_path.push_back(part);
                }
            }
            bool exists = true;
            std::string sum_parts;
            for(unsigned long i = 0; i < split_path.size(); i++){
                sum_parts += "/" + split_path[i] ;
                exists = exists and H5Lexists(file, sum_parts.c_str(), H5P_DEFAULT) > 0;
                if (not exists){break;}
            }
            return exists;
        }



        inline bool check_if_attribute_exists(hid_t file,const std::string &link_name, const std::string &attribute_name){
            hid_t dataset      = H5Dopen(file, link_name.c_str(), H5P_DEFAULT);
            bool exists = false;
            unsigned int num_attrs = H5Aget_num_attrs(dataset);

            for (unsigned int i = 0; i < num_attrs; i ++){
                hid_t attr_id = H5Aopen_idx(dataset, i);
                hsize_t buf_size = 0;
                std::vector<char> buf;
                buf_size = H5Aget_name (attr_id, buf_size, nullptr);
                buf.resize(buf_size+1);
                buf_size = H5Aget_name (attr_id, buf_size+1, buf.data());
                std::string attr_name (buf.data());
                H5Aclose(attr_id);
                if (attribute_name == attr_name){exists  = true ; break;}
            }

            H5Dclose(dataset);
            return exists;
        }

        void set_extent_dataset(hid_t file, const DatasetProperties &props);
        inline void set_extent_dataset(hid_t file,const DatasetProperties &props){
            if (check_if_link_exists_recursively(file, props.dset_name)) {
                hid_t dataset = H5Dopen(file, props.dset_name.c_str(), H5P_DEFAULT);
                H5Dset_extent(dataset, props.dims.data());
                H5Dclose(dataset);
            }else{
                spdlog::critical("Link does not exist, yet the extent is being set.");
                exit(1);
            }
        }



        inline void extendDataset(hid_t file, const std::string & dataset_relative_name, const int dim, const int extent){
            if (H5Lexists(file, dataset_relative_name.c_str(), H5P_DEFAULT)) {
                hid_t dataset = H5Dopen(file, dataset_relative_name.c_str(), H5P_DEFAULT);
                // Retrieve the current size of the memspace (act as if you don't know it's size and want to append)
                hid_t filespace = H5Dget_space(dataset);
                const int ndims = H5Sget_simple_extent_ndims(filespace);
                std::vector<hsize_t> old_dims(ndims);
                std::vector<hsize_t> new_dims(ndims);
                H5Sget_simple_extent_dims(filespace, old_dims.data(), nullptr);
                new_dims = old_dims;
                new_dims[dim] += extent;
                H5Dset_extent(dataset, new_dims.data());
                H5Dclose(dataset);
                H5Sclose(filespace);
                H5Fflush(file,H5F_SCOPE_LOCAL);
            }

        }

        template<typename DataType>
        void extendDataset(hid_t file, const DataType &data, const std::string & dataset_relative_name){
            namespace tc = h5pp::Type::Check;
            if constexpr (tc::is_eigen_core<DataType>::value){
//                hid_t file = h5ppFile.open_file();
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
//                h5ppFile.close(file);
            }
            else{
                extend_dataset(dataset_relative_name, 0, h5pp::Utils::get_Size(data));
            }
        }



        inline void create_group_link(hid_t file, hid_t plist_lncr, const std::string &group_relative_name) {
            //Check if group exists already
            spdlog::trace("Creating group link: {}", group_relative_name);
            std::stringstream path_stream(group_relative_name);
            std::vector<std::string> split_path;
            std::string head;
            std::string stem = "/";
            while(std::getline(path_stream, head, '/')){
                split_path.push_back(stem + head);
                stem += head + "/";
            }

            for(auto &group_name : split_path){
                if (not h5pp::Hdf5::check_if_link_exists_recursively(file, group_name)) {
                    hid_t group = H5Gcreate(file, group_name.c_str(), plist_lncr, H5P_DEFAULT, H5P_DEFAULT);
                    H5Gclose(group);
                }
            }
        }

        inline void write_symbolic_link(hid_t file ,const std::string &src_path, const std::string &tgt_path){
            bool exists = h5pp::Hdf5::check_if_link_exists_recursively(file, src_path);
            if (not exists)throw std::runtime_error("Trying to write soft link to non-existing path: " + src_path);
            herr_t retval = H5Lcreate_soft(src_path.c_str(), file, tgt_path.c_str(), H5P_DEFAULT, H5P_DEFAULT);
        }


        inline void create_dataset_link(hid_t file,hid_t plist_lncr, const DatasetProperties &props){
            if (not h5pp::Hdf5::check_if_link_exists_recursively(file, props.dset_name)){
                hid_t dataspace = h5pp::Utils::get_DataSpace_unlimited(props.ndims);
                hid_t dset_cpl  = H5Pcreate(H5P_DATASET_CREATE);
                H5Pset_layout(dset_cpl, H5D_CHUNKED);
                H5Pset_chunk(dset_cpl, props.ndims, props.chunk_size.data());
                // H5Pset_deflate (dset_cpl ,props.compression_level);
                hid_t dataset = H5Dcreate(file,
                                          props.dset_name.c_str(),
                                          props.datatype,
                                          dataspace,
                                          plist_lncr,
                                          dset_cpl,
                                          H5P_DEFAULT);
                H5Dclose(dataset);
                H5Sclose(dataspace);
                H5Pclose(dset_cpl);
            }
        }


        inline void select_hyperslab(const hid_t &filespace, const hid_t &memspace){
            const int ndims = H5Sget_simple_extent_ndims(filespace);
            std::vector<hsize_t> mem_dims(ndims);
            std::vector<hsize_t> file_dims(ndims);
            std::vector<hsize_t> start(ndims);
            H5Sget_simple_extent_dims(memspace , mem_dims.data(), nullptr);
            H5Sget_simple_extent_dims(filespace, file_dims.data(), nullptr);
            for(int i = 0; i < ndims; i++){
                start[i] = file_dims[i] - mem_dims[i];
            }
            H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start.data(), nullptr, mem_dims.data(), nullptr);
        }



//        template<typename DataType>
//        void extendDataset(h5pp::File &h5ppFile, const DataType &data, const std::string & dataset_relative_name){
//            namespace tc = h5pp::Type::Check;
//            if constexpr (tc::is_eigen_core<DataType>::value){
//                hid_t file = h5ppFile.open_file();
//                extend_dataset(dataset_relative_name, 0, data.rows());
//                hid_t dataset   = H5Dopen(file, dataset_relative_name.c_str(), H5P_DEFAULT);
//                hid_t filespace = H5Dget_space(dataset);
//                int ndims = H5Sget_simple_extent_ndims(filespace);
//                std::vector<hsize_t> dims(ndims);
//                H5Sget_simple_extent_dims(filespace,dims.data(),NULL);
//                H5Dclose(dataset);
//                H5Sclose(filespace);
//                if (dims[1] < (hsize_t) data.cols()){
//                    extend_dataset(dataset_relative_name, 1, data.cols());
//                }
//                h5ppFile.close(file);
//            }
//            else{
//                extend_dataset(dataset_relative_name, 0, h5pp::Utils::get_Size(data));
//            }
//        }

    }


}



#endif //LIBH5PP_H5PPHDF5_H
