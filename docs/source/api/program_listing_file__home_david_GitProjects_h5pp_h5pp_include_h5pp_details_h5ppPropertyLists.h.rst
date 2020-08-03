
.. _program_listing_file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppPropertyLists.h:

Program Listing for File h5ppPropertyLists.h
============================================

|exhale_lsh| :ref:`Return to documentation for file <file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppPropertyLists.h>` (``/home/david/GitProjects/h5pp/h5pp/include/h5pp/details/h5ppPropertyLists.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #pragma once
   #include "h5ppHid.h"
   #include <hdf5.h>
   
   namespace h5pp {
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
           }
       };
   }
