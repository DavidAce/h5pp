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
        hid::h5p fileCreate        = H5Pcreate(H5P_FILE_CREATE);
        hid::h5p fileAccess        = H5Pcreate(H5P_FILE_ACCESS);
        hid::h5p linkCreate        = H5Pcreate(H5P_LINK_CREATE);
        hid::h5p linkAccess        = H5Pcreate(H5P_LINK_ACCESS);
        hid::h5p dsetCreate        = H5Pcreate(H5P_DATASET_CREATE);
        hid::h5p dsetAccess        = H5Pcreate(H5P_DATASET_ACCESS);
        hid::h5p groupCreate       = H5Pcreate(H5P_GROUP_CREATE);
        hid::h5p groupAccess       = H5Pcreate(H5P_GROUP_ACCESS);
        hid::h5p dsetXfer          = H5Pcreate(H5P_DATASET_XFER);
        bool     vlenTrackReclaims = true;

        PropertyLists() {
            // Set default to create missing intermediate groups if they do not exist
            // ... should have been the default all along?
            //            linkCreate = H5Pcreate(H5P_LINK_CREATE);
            //            H5Pset_create_intermediate_group(linkCreate, 1);

            // h5pp uses H5F_CLOSE_STRONG by default (id's associated to a file are closed when the file is closed)
            fileAccess = H5Pcreate(H5P_FILE_ACCESS);
            if(H5Pset_fclose_degree(fileAccess, H5F_CLOSE_STRONG) < 0) throw h5pp::runtime_error("H5Pset_fclose_degree() failed");
            // The following settings are needed to reduce group size overhead
            if(H5Pset_libver_bounds(fileAccess, H5F_libver_t::H5F_LIBVER_EARLIEST, H5F_LIBVER_LATEST) < 0)
                throw h5pp::runtime_error("H5Pset_libver_bounds() failed");
            if(H5Pset_link_phase_change(groupCreate, 100, 6) < 0) throw h5pp::runtime_error("H5Pset_link_phase_change() failed");
            if(H5Pset_link_creation_order(groupCreate, H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED) < 0)
                throw h5pp::runtime_error("H5Pset_link_creation_order() failed");
        }
    };
}
