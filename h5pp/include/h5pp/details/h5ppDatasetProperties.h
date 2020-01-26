#pragma once
#include "h5ppHid.h"
#include "h5ppLogger.h"
#include "h5ppOptional.h"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <string>
#include <vector>

/*! \class DatasetProperties
 *  Properties of HDF5 datasets
 */
namespace h5pp {
    class DatasetProperties {
        public:
        hid::h5d                            dataSet;
        hid::h5t                            dataType;
        hid::h5s                            fileSpace;
        hid::h5s                            dataSpace;
        hid::h5s                            memSpace;
        hid::h5p                            plist_dset_create = H5P_DEFAULT;
        hid::h5p                            plist_dset_access = H5P_DEFAULT;
        std::optional<std::string>          dsetName;
        std::optional<bool>                 dsetExists;
        std::optional<H5D_layout_t>         layout; //
        std::optional<hsize_t>              size;
        std::optional<size_t>               bytes;
        std::optional<int>                  ndims;
        std::optional<std::vector<hsize_t>> dims;
        std::optional<std::vector<hsize_t>> chunkDims;
        std::optional<unsigned int>         compressionLevel;
    };

}
