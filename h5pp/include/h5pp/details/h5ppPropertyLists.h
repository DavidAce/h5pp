#pragma once
#include "h5ppHid.h"
#include <hdf5.h>

namespace h5pp {
    /*!
     * Property lists that describe policies for common tasks in HDF5.
     * Note that we do not include dataset property lists here because
     * those are local to each individual dataset.
     * Some of these can be used/modified to setup MPI usage.
     * */
    struct PropertyLists {
        Hid::h5p file_create  = H5P_DEFAULT; // H5Pcreate(H5P_FILE_CREATE);
        Hid::h5p file_access  = H5P_DEFAULT; // H5Pcreate(H5P_FILE_ACCESS);
        Hid::h5p link_create  = H5P_DEFAULT; // H5Pcreate(H5P_LINK_CREATE);
        Hid::h5p link_access  = H5P_DEFAULT; // H5Pcreate(H5P_LINK_ACCESS);
        Hid::h5p group_create = H5P_DEFAULT; // H5Pcreate(H5P_GROUP_CREATE);
        Hid::h5p group_access = H5P_DEFAULT; // H5Pcreate(H5P_GROUP_ACCESS);
        Hid::h5p dset_xfer    = H5P_DEFAULT; // H5Pcreate(H5P_DATASET_XFER);

        PropertyLists() {
            // Set default to create missing intermediate groups if they do not exist
            // ... should have been the default all along?
            link_create = H5Pcreate(H5P_LINK_CREATE);
            H5Pset_create_intermediate_group(link_create, 1);
        }
    };
}
