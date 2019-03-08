//
// Created by david on 2019-03-01.
//

#ifndef H5PP_ATTRIBUTEPROPERTIES_H
#define H5PP_ATTRIBUTEPROPERTIES_H
#include <hdf5.h>
#include <hdf5_hl.h>
#include <vector>
#include <string>

namespace h5pp{
    class AttributeProperties {
    public:
        hid_t                   dataType;
        hid_t                   memSpace;
        hsize_t                 size;
        int                     ndims;
        std::vector<hsize_t>    dims;
        std::string             attrName;
        std::string             linkName;
        ~AttributeProperties(){
            H5Tclose(dataType);
            H5Sclose(memSpace);
        }
    };
}
#endif //H5PP_ATTRIBUTEPROPERTIES_H



