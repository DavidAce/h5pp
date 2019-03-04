//
// Created by david on 2019-03-01.
//

#ifndef H5PP_DATASETPROPERTIES_H
#define H5PP_DATASETPROPERTIES_H
#include <hdf5.h>
#include <hdf5_hl.h>
#include <vector>
#include <string>

namespace h5pp{

    class DatasetProperties {
    public:
        hid_t                   datatype;
        hid_t                   memspace;
        hsize_t                 size;
        int                     ndims;
        std::vector<hsize_t>    chunk_size;
        std::vector<hsize_t>    dims;
        std::string             dset_name;
        unsigned int            compression_level = 6;

        ~DatasetProperties(){
            H5Tclose(datatype);
            H5Sclose(memspace);
        }
    };




}

#endif //H5PP_DATASETPROPERTIES_H


