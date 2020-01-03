#pragma once
#include <hdf5.h>
#include <hdf5_hl.h>
#include <string>
#include <vector>

namespace h5pp {
    class DatasetProperties {
        public:
        std::string          dsetName;
        bool                 extendable = false;
        bool                 linkExists = false;
        hid_t                dataType;
        hid_t                memSpace;
        hid_t                dataSpace;
        hsize_t              size;
        int                  ndims;
        std::vector<hsize_t> dims;
        std::vector<hsize_t> chunkDims;
        size_t               compressionLevel = 0;

        ~DatasetProperties() {
            H5Tclose(dataType);
            H5Sclose(memSpace);
            H5Sclose(dataSpace);
        }
    };

}
