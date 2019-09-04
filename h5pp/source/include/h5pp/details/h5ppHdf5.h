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




        inline bool checkIfLinkExistsRecursively(hid_t file, std::string path){
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


        inline hid_t openLink(hid_t file, const std::string &linkName){
            try{
                if (checkIfLinkExistsRecursively(file, linkName)){
                    hid_t dataset = H5Oopen(file, linkName.c_str(), H5P_DEFAULT);
                    if (dataset < 0){
                        H5Eprint(H5E_DEFAULT, stderr);
                        throw std::runtime_error("Failed to open existing dataset: " + linkName);
                    }else{
                        return dataset;
                    }
                }else{
                    throw std::runtime_error("Dataset does not exist: " + linkName);
                }
            }
            catch (std::exception &ex){
                throw std::runtime_error("openLink: " + std::string(ex.what()));
            }

        }

        inline herr_t closeLink(hid_t link){
            herr_t closeHandle = H5Oclose(link);
            if (closeHandle < 0){
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error("Failed to close link.");
            }else{
                return closeHandle;
            }
        }

        inline bool checkIfAttributeExists(hid_t file, const std::string &linkName, const std::string &attributename){
            hid_t dataset      = openLink(file, linkName);
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
                if (attributename == attr_name){exists  = true ; break;}
            }

            closeLink(dataset);
            return exists;
        }

        void setExtentDataset(hid_t file, const DatasetProperties &props);
        inline void setExtentDataset(hid_t file, const DatasetProperties &props){
            try{
                if (not props.extendable) {
                    h5pp::Logger::log->critical("Called setExtentDataset for non-extendable dataset");
                }
                hid_t dataset = openLink(file, props.dsetName);
                herr_t err = H5Dset_extent(dataset, props.dims.data());
                if (err < 0) throw std::runtime_error("Error while setting extent");
                closeLink(dataset);
            }catch(std::exception &ex){
                H5Eprint(H5E_DEFAULT, stderr);
                h5pp::Logger::log->error("Could not set extent: {}", ex.what());
                throw std::runtime_error("Could not set extent: " + std::string(ex.what()));
            }
        }



        inline void extendDataset(hid_t file, const std::string & datasetRelativeName, const int dim, const int extent){
            try{
                hid_t dataset = openLink(file, datasetRelativeName);
                // Retrieve the current size of the memSpace (act as if you don't know it's size and want to append)
                hid_t filespace = H5Dget_space(dataset);
                const int ndims = H5Sget_simple_extent_ndims(filespace);
                std::vector<hsize_t> oldDims(ndims);
                std::vector<hsize_t> newDims(ndims);
                H5Sget_simple_extent_dims(filespace, oldDims.data(), nullptr);
                newDims = oldDims;
                newDims[dim] += extent;
                H5Dset_extent(dataset, newDims.data());
                H5Sclose(filespace);
                closeLink(dataset);
            }catch(std::exception &ex){
                h5pp::Logger::log->error("Could not extend dataset [ {} ] : {}", datasetRelativeName,ex.what());
                throw std::runtime_error("Could not extend dataset [ "+ datasetRelativeName + " ] :" + std::string(ex.what()));
            }
        }

        template<typename DataType>
        void extendDataset(hid_t file, const DataType &data, const std::string & datasetRelativeName){
            namespace tc = h5pp::Type::Check;
            try{
                if constexpr (tc::is_eigen_core<DataType>::value){
                    extendDataset(file,datasetRelativeName, 0, data.rows());
                    hid_t dataSet   = openLink(file, datasetRelativeName);
                    hid_t fileSpace = H5Dget_space(dataSet);
                    int ndims = H5Sget_simple_extent_ndims(fileSpace);
                    std::vector<hsize_t> dims(ndims);
                    H5Sget_simple_extent_dims(fileSpace,dims.data(),NULL);
                    H5Sclose(fileSpace);
                    closeLink(dataSet);
                    if (dims[1] < (hsize_t) data.cols()){
                        extendDataset(file,datasetRelativeName, 1, data.cols());
                    }
                }
                else{
                    extendDataset(file,datasetRelativeName, 0, h5pp::Utils::getSize(data));
                }
            }catch(std::exception &ex){
                h5pp::Logger::log->error("Could not extend dataset [ {} ] : {}", datasetRelativeName,ex.what());
                throw std::runtime_error("Could not extend dataset [ "+ datasetRelativeName + " ] :" + std::string(ex.what()));
            }

        }



        inline void create_group_link(hid_t file, hid_t plist_lncr, const std::string &group_relative_name) {
            //Check if group exists already
            h5pp::Logger::log->trace("Creating group link: {}", group_relative_name);
            std::stringstream path_stream(group_relative_name);
            std::vector<std::string> split_path;
            std::string head;
            std::string stem = "/";
            while(std::getline(path_stream, head, '/')){
                split_path.push_back(stem + head);
                stem += head + "/";
            }

            for(auto &group_name : split_path){
                if (not h5pp::Hdf5::checkIfLinkExistsRecursively(file, group_name)) {
                    hid_t group = H5Gcreate(file, group_name.c_str(), plist_lncr, H5P_DEFAULT, H5P_DEFAULT);
                    H5Gclose(group);
                }
            }
        }

        inline void write_symbolic_link(hid_t file ,const std::string &src_path, const std::string &tgt_path){
            if(checkIfLinkExistsRecursively(file, src_path)){
                herr_t retval = H5Lcreate_soft(src_path.c_str(), file, tgt_path.c_str(), H5P_DEFAULT, H5P_DEFAULT);
                if(retval < 0){
                    H5Eprint(H5E_DEFAULT, stderr);
                    h5pp::Logger::log->error("Failed to write symbolic link: {}", src_path);
                    throw std::runtime_error("Failed to write symbolic link:  " + src_path);
                }
            }else{
                throw std::runtime_error("Trying to write soft link to non-existing path: " + src_path);
            }
        }




        inline void setSizeDependentLayout(hid_t dset_cpl, const DatasetProperties &props){
            /*! Depending on the size of this dataset we may benefint from using either
                a contiguous layout (for big non-extendable non-compressible datasets),
                a chunked layout (for extendable and compressible datasets)
                or a compact layout (for tiny datasets).

                Contiguous
                For big non-extendable non-compressible datasets

                Chunked
                Chunking is required for enabling compression and other filters, as well as for
                creating extendible or unlimited dimension datasets. Note that a chunk always has
                the same rank as the dataset and the chunk's dimensions do not need to be factors
                of the dataset dimensions.

                Compact
                A compact dataset is one in which the raw data is stored in the object header of the dataset.
                This layout is for very small datasets that can easily fit in the object header.
                The compact layout can improve storage and access performance for files that have many very
                tiny datasets. With one I/O access both the header and data values can be read.
                The compact layout reduces the size of a file, as the data is stored with the header which
                will always be allocated for a dataset. However, the object header is 64 KB in size,
                so this layout can only be used for very small datasets.
             */

            // First, we check if the user explicitly asked for an extendable dataset.
            if(props.extendable) {
                if (props.chunkSize.empty() or props.chunkSize.data() == nullptr)
                    throw std::runtime_error("ChunkSize has not been set. Can't call H5Pset_chunk(...)");
                H5Pset_layout(dset_cpl, H5D_CHUNKED);
                H5Pset_chunk(dset_cpl, props.ndims, props.chunkSize.data());
            }else {
                hsize_t dsetsize = props.size * H5Tget_size(props.dataType); // Get size of dataset in bytes
                if (dsetsize <= h5pp::Constants::max_size_compact) {
                    H5Pset_layout(dset_cpl, H5D_COMPACT);
                }
                else{
                    H5Pset_layout(dset_cpl, H5D_CONTIGUOUS);
                }
            }
        }

        inline void createDatasetLink(hid_t file, hid_t plist_lncr, const DatasetProperties &props){
            if (not h5pp::Hdf5::checkIfLinkExistsRecursively(file, props.dsetName)){
                hid_t dset_cpl  = H5Pcreate(H5P_DATASET_CREATE);
                setSizeDependentLayout(dset_cpl,props);
                // H5Pset_deflate (dset_cpl ,props.compressionLevel);

                if(props.dsetName.empty())  throw std::runtime_error("props.dsetName is empty");
                if(props.dataSpace < 0)     throw std::runtime_error("props.dataSpace is not set, dataSpace < 0");


                hid_t dataset = H5Dcreate(file,
                                          props.dsetName.c_str(),
                                          props.dataType,
                                          props.dataSpace,
                                          plist_lncr,
                                          dset_cpl,
                                          H5P_DEFAULT);
                H5Pclose(dset_cpl);
                closeLink(dataset);
            }
        }


        inline void select_hyperslab(const hid_t &fileSpace, const hid_t &memSpace){
            const int ndims = H5Sget_simple_extent_ndims(fileSpace);
            std::vector<hsize_t> memDims(ndims);
            std::vector<hsize_t> fileDims(ndims);
            std::vector<hsize_t> start(ndims);
            H5Sget_simple_extent_dims(memSpace , memDims.data(), nullptr);
            H5Sget_simple_extent_dims(fileSpace, fileDims.data(), nullptr);
            for(int i = 0; i < ndims; i++){
                start[i] = fileDims[i] - memDims[i];
            }
            H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start.data(), nullptr, memDims.data(), nullptr);
        }



        inline herr_t fileInfo([[maybe_unused]]  hid_t loc_id, const char *name, [[maybe_unused]]  const H5L_info_t *linfo, void *opdata){
            try{

//                hid_t group;
                auto linkNames=reinterpret_cast< std::vector<std::string>* >(opdata);
//                group = H5Gopen2(loc_id, name, H5P_DEFAULT);
                //do stuff with group object, if needed
                linkNames->push_back(name);
//                std::cout << "Name : " << name << std::endl;
//                H5Gclose(group);
                return 0;




        //        hid_t group = H5Gopen2(loc_id, name, H5P_DEFAULT);
                std::cout << " " << name << std::endl; // Display the group name.
        //        H5Gclose(group);

            }catch(...){
                throw(std::logic_error("Not a group: " + std::string(name)));
            }
            return 0;
        }

        inline std::vector<std::string> getContentsOfGroup(hid_t file,std::string groupName){
            h5pp::Logger::log->trace("Getting contents of group: {}",groupName);
            std::vector<std::string> linkNames;
            try{
                [[maybe_unused]]  herr_t idx = H5Literate_by_name (file, groupName.c_str(), H5_INDEX_NAME,
                                                 H5_ITER_NATIVE, NULL, fileInfo, &linkNames,
                                                 H5P_DEFAULT);
            }
            catch(std::exception &ex){
                h5pp::Logger::log->debug("Failed to get contents --  {}",ex.what());
            }
            return linkNames;
        }

    }


}



#endif //LIBH5PP_H5PPHDF5_H
