
.. _program_listing_file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppHyperslab.h:

Program Listing for File h5ppHyperslab.h
========================================

|exhale_lsh| :ref:`Return to documentation for file <file__home_david_GitProjects_h5pp_h5pp_include_h5pp_details_h5ppHyperslab.h>` (``/home/david/GitProjects/h5pp/h5pp/include/h5pp/details/h5ppHyperslab.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #pragma once
   #include "h5ppDimensionType.h"
   #include "h5ppFormat.h"
   #include "h5ppOptional.h"
   #include "h5ppTypeSfinae.h"
   #include <hdf5.h>
   #include <type_traits>
   #include <utility>
   #include <vector>
   namespace h5pp {
       class Hyperslab {
           public:
           // Hyperslab properties. Read here https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-SelectHyperslab
   
           OptDimsType                 offset      = std::nullopt;   
           OptDimsType                 extent      = std::nullopt;   
           OptDimsType                 stride      = std::nullopt;   
           OptDimsType                 blocks      = std::nullopt;   
           std::optional<H5S_sel_type> select_type = std::nullopt;   
           H5S_seloper_t               select_oper = H5S_SELECT_SET; 
           Hyperslab() = default;
           Hyperslab(const DimsType &offset, const DimsType &extent, OptDimsType stride = std::nullopt, OptDimsType blocks = std::nullopt)
               : offset(offset), extent(extent), stride(std::move(stride)), blocks(std::move(blocks)) {}
   
           // Delete some constructors to avoid ambiguity. Both offset and extent are required if any of them is to be given
           Hyperslab(DimsType) = delete;
           Hyperslab(OptDimsType) = delete;
           Hyperslab(std::initializer_list<hsize_t>) = delete;
           Hyperslab(std::vector<hsize_t>) = delete;
   
           explicit Hyperslab(const hid::h5s &space) {
               int rank = H5Sget_simple_extent_ndims(space);
               if(rank < 0) throw std::runtime_error("Could not read ndims on given space");
               if(rank == 0) return;
               select_type = H5Sget_select_type(space);
               if(select_type.value() == H5S_SEL_HYPERSLABS) {
   #if H5_VERSION_GE(1, 10, 0)
                   htri_t is_regular = H5Sis_regular_hyperslab(space);
                   if(is_regular < 0) {
                       H5Eprint(H5E_DEFAULT, stderr);
                       throw std::runtime_error("Failed to query hyperslab type in space");
                   }
                   if(not is_regular)
                       throw std::runtime_error("The space has irregular (non-rectangular) hyperslab selection.\n"
                                                "This is not yet supported by h5pp");
   #endif
                   offset = std::vector<hsize_t>(static_cast<size_t>(rank), 0);
                   extent = std::vector<hsize_t>(static_cast<size_t>(rank), 0);
                   stride = std::vector<hsize_t>(static_cast<size_t>(rank), 0);
                   blocks = std::vector<hsize_t>(static_cast<size_t>(rank), 0);
   #if H5_VERSION_GE(1, 10, 0)
                   H5Sget_regular_hyperslab(space, offset->data(), stride->data(), extent->data(), blocks->data());
   #else
                   H5Sget_simple_extent_dims(space, extent->data(), nullptr);
   #endif
               } else if(select_type.value() == H5S_SEL_ALL) {
                   offset = std::vector<hsize_t>(static_cast<size_t>(rank), 0);
                   extent = std::vector<hsize_t>(static_cast<size_t>(rank), 0);
                   H5Sget_simple_extent_dims(space, extent->data(), nullptr);
               } else if(select_type.value() == H5S_SEL_NONE)
                   return;
               else if(select_type.value() == H5S_SEL_ERROR)
                   throw std::runtime_error("Invalid hyperslab selection");
               else
                   throw std::runtime_error("Unsupported selection type. Choose space selection type NONE, ALL or HYPERSLABS");
           }
           [[nodiscard]] bool empty() const { return not offset and not extent and not stride and not blocks; }
   
           [[nodiscard]] std::string string(bool enable = true) const {
               std::string msg;
               if(not enable) return msg;
               if(offset) msg.append(h5pp::format(" | offset {}", offset.value()));
               if(extent) msg.append(h5pp::format(" | extent {}", extent.value()));
               if(stride) msg.append(h5pp::format(" | stride {}", stride.value()));
               if(blocks) msg.append(h5pp::format(" | blocks {}", blocks.value()));
               return msg;
           }
       };
   
   }
