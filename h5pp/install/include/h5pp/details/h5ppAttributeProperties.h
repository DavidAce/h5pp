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
        hid_t                   datatype;
        hid_t                   memspace;
        hsize_t                 size;
        int                     ndims;
        std::vector<hsize_t>    dims;
        std::string             attr_name;
        std::string             link_name;
        ~AttributeProperties(){
            H5Tclose(datatype);
            H5Sclose(memspace);
        }
    };
}
#endif //H5PP_ATTRIBUTEPROPERTIES_H



