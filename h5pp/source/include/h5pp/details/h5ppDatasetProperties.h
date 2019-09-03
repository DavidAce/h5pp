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
        bool                    extendable = false;
        bool                    linkExists = false;
        hid_t                   dataType;
        hid_t                   memSpace;
        hid_t                   dataSpace;
        hsize_t                 size;
        int                     ndims;
        std::vector<hsize_t>    chunkSize;
        std::vector<hsize_t>    dims;
        std::string             dsetName;
        unsigned int            compressionLevel = 6;

        ~DatasetProperties(){
            H5Tclose(dataType);
            H5Sclose(memSpace);
            H5Sclose(dataSpace);
        }
    };




}

#endif //H5PP_DATASETPROPERTIES_H


