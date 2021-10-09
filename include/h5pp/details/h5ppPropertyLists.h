#pragma once
#include "h5ppHid.h"
#include <hdf5/hdf5.h>

namespace h5pp {
    /*!
     * Property lists that describe policies for common tasks in HDF5.
     * Note that we do not include dataset property lists here because
     * those are local to each individual dataset.
     * Some of these can be used/modified to setup MPI usage.
     * */
    struct PropertyLists {
        hid::h5p fileCreate  = H5P_DEFAULT; // H5Pcreate(H5P_FILE_CREATE);
        hid::h5p fileAccess  = H5P_DEFAULT; // H5Pcreate(H5P_FILE_ACCESS);
        hid::h5p linkCreate  = H5P_DEFAULT; // H5Pcreate(H5P_LINK_CREATE);
        hid::h5p linkAccess  = H5P_DEFAULT; // H5Pcreate(H5P_LINK_ACCESS);
        hid::h5p groupCreate = H5P_DEFAULT; // H5Pcreate(H5P_GROUP_CREATE);
        hid::h5p groupAccess = H5P_DEFAULT; // H5Pcreate(H5P_GROUP_ACCESS);
        hid::h5p dsetXfer    = H5P_DEFAULT; // H5Pcreate(H5P_DATASET_XFER);

        PropertyLists() {
            // Set default to create missing intermediate groups if they do not exist
            // ... should have been the default all along?
            linkCreate = H5Pcreate(H5P_LINK_CREATE);
            H5Pset_create_intermediate_group(linkCreate, 1);

            // h5pp uses H5F_CLOSE_STRONG by default (id's associated to a file are closed when the file is closed)
            fileAccess = H5Pcreate(H5P_FILE_ACCESS);
            H5Pset_fclose_degree(fileAccess, H5F_CLOSE_STRONG);
        }
    };

    inline PropertyLists defaultPlists = PropertyLists();

}
