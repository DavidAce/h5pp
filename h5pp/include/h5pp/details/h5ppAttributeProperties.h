
#pragma once
#include <hdf5.h>
#include <hdf5_hl.h>
#include <string>
#include <vector>

namespace h5pp {
    class AttributeProperties {
        public:
        hid_t                dataType;
        hid_t                memSpace;
        hsize_t              size;
        int                  ndims;
        std::vector<hsize_t> dims;
        std::string          attrName;
        std::string          linkPath;
        ~AttributeProperties() {
            H5Tclose(dataType);
            H5Sclose(memSpace);
        }
    };
} // namespace h5pp
