#include "h5ppConstants.h"
#include "h5ppHdf5.h"
#include "h5ppMeta.h"
#include "h5ppTableProperties.h"
#include "h5ppTypeSfinae.h"
#include "h5ppUtils.h"

namespace h5pp::scan {

    /*! \fn fillDsetMeta
     * Fills missing information about a dataset in a meta-data struct
     *
     * @param meta A struct with i nformation about a dataset
     * @param file a h5f file identifier
     * @param dsetPath the full path to a dataset in an HDF5 file
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     */
    inline void
        fillMetaDset(h5pp::MetaDset &meta, const hid::h5f &file, std::string_view dsetPath, const Options &options = Options(), const PropertyLists &plists = PropertyLists()) {
        h5pp::logger::log->debug("Reading metadata of dataset [{}]", dsetPath);
        if(not meta.h5_file) meta.h5_file = file;
        if(not meta.dsetPath) meta.dsetPath = dsetPath;
        if(not meta.dsetExists) meta.dsetExists = h5pp::hdf5::checkIfDatasetExists(meta.h5_file.value(), meta.dsetPath.value(), std::nullopt, plists.link_access);
        // If the dataset does not exist, there isn't much else to do so we return;
        if(meta.dsetExists and not meta.dsetExists.value()) return;
        // From here on the dataset exists
        if(not meta.h5_dset) meta.h5_dset = h5pp::hdf5::openLink<hid::h5d>(file, dsetPath, meta.dsetExists, plists.link_access);
        if(not meta.h5_type) meta.h5_type = H5Dget_type(meta.h5_dset.value());
        if(not meta.h5_space) meta.h5_space = H5Dget_space(meta.h5_dset.value());

        // If no slab is given the default space will be the whole dataset
        if(options.fileSlabs) h5pp::hdf5::selectHyperSlabs(meta.h5_space.value(), options.fileSlabs.value(), options.fileSlabSelectOps);

        // Get the properties of the selected space
        if(not meta.dsetByte) meta.dsetByte = h5pp::hdf5::getBytesTotal(meta.h5_dset.value());
        if(not meta.dsetSize) meta.dsetSize = h5pp::hdf5::getSize(meta.h5_dset.value());
        if(not meta.dsetRank) meta.dsetRank = h5pp::hdf5::getRank(meta.h5_dset.value());
        if(not meta.dsetDims) meta.dsetDims = h5pp::hdf5::getDimensions(meta.h5_dset.value());
        if(not meta.dsetDimsMax) meta.dsetDimsMax = h5pp::hdf5::getMaxDimensions(meta.h5_dset.value());
        // We read the layout from file. Note that it is not possible to change the layout on existing datasets! Read more here
        // https://support.hdfgroup.org/HDF5/Tutor/layout.html
        if(not meta.h5_plist_dset_create) meta.h5_plist_dset_create = H5Dget_create_plist(meta.h5_dset.value());
        if(not meta.h5_plist_dset_access) meta.h5_plist_dset_access = H5Dget_access_plist(meta.h5_dset.value());
        if(not meta.h5_layout) meta.h5_layout = H5Pget_layout(meta.h5_plist_dset_create.value());
        if(not meta.chunkDims and meta.h5_layout.value() == H5D_CHUNKED) {
            meta.chunkDims = std::vector<hsize_t>(meta.dsetRank.value(), 0);
            int success    = H5Pget_chunk(meta.h5_plist_dset_create.value(), meta.dsetRank.value(), meta.chunkDims.value().data());
            if(success < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to read chunk dimensions of dataset [{}]", meta.dsetPath.value()));
            }
        }
    }

    template<typename DataType>
    inline h5pp::MetaDset
        makeMetaDset(const hid::h5f &file, const DataType &data, std::string_view dsetPath, const Options &options = Options(), const PropertyLists &plists = PropertyLists()) {
        h5pp::MetaDset meta;
        meta.dsetExists = h5pp::hdf5::checkIfDatasetExists(file, dsetPath, std::nullopt, plists.link_access);
        if(meta.dsetExists.value()) {
            fillMetaDset(meta, file, dsetPath, options, plists);
            return meta;
        }
        h5pp::logger::log->debug("Creating dataset metadata for type [{}]", h5pp::type::sfinae::type_name<DataType>());
        meta.dsetPath   = dsetPath;
        meta.h5_file = file;
        meta.h5_type              = h5pp::util::getH5Type<DataType>(options.h5_type);
        meta.h5_layout            = h5pp::util::decideLayout(data,options.dataDims,options.dataDimsMax, options.h5_layout);
        meta.h5_plist_dset_create = H5Pcreate(H5P_DATASET_CREATE);
        meta.h5_plist_dset_access = H5Pcreate(H5P_DATASET_ACCESS);
        meta.dsetDims    = h5pp::util::getDimensions(data, options.dataDims); // Infer the dimensions from given data, or interpret differently if dims given in options
        meta.dsetDimsMax = h5pp::util::getDimensionsMax(meta.dsetDims.value(), meta.h5_layout.value(), options.dataDimsMax);
        meta.dsetSize    = h5pp::util::getSize(data,options.dataDims);
        meta.dsetByte    = h5pp::util::getBytesTotal(data,options.dataDims);
        meta.dsetRank    = h5pp::util::getRank<DataType>(options.dataDims);
        meta.chunkDims   = h5pp::util::getDefaultChunkDimensions(meta.dsetSize.value(), meta.dsetDims.value(), options.chunkDims);
        meta.compression = h5pp::hdf5::getValidCompressionLevel(options.compression);
        meta.h5_space    = h5pp::util::getDataSpace(meta.dsetSize.value(), meta.dsetDims.value(), meta.h5_layout.value(), options.dataDimsMax);
        h5pp::hdf5::setStringSize(meta.h5_type.value(), H5T_VARIABLE);
        h5pp::hdf5::setProperty_layout(meta); // Must go before setting chunk dims
        h5pp::hdf5::setProperty_compression(meta);
        h5pp::hdf5::setProperty_chunkDims(meta);
        h5pp::hdf5::setSpaceExtent(meta);
        return meta;
    }

    /*!
     * \fn getDsetMeta
     * Returns information about a dataset in a meta-data struct
     * @param file a h5t file identifier
     * @param dsetPath the full path to a dataset in an HDF5 file
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     * @return
     */
    inline h5pp::MetaDset getMetaDset(const hid::h5f &file, std::string_view dsetPath, const Options &options = Options(), const PropertyLists &plists = PropertyLists()) {
        h5pp::MetaDset meta;
        fillMetaDset(meta, file, dsetPath, options, plists);
        return meta;
    }

    template<typename DataType>
    inline void fillMetaData(const DataType &data, MetaData &meta, const Options &options = Options()) {
        h5pp::logger::log->debug("Reading metadata of datatype [{}]", h5pp::type::sfinae::type_name<DataType>());
        // The point of passing options is to reinterpret the shape of the data and not to resize!
        // The data container should already be resized before entering this function.

        if(not meta.dataDims) meta.dataDims = h5pp::util::getDimensions(data, options.dataDims);
        if(not meta.dataSize) meta.dataSize = h5pp::util::getSize(data,options.dataDims);
        if(not meta.dataRank) meta.dataRank = h5pp::util::getRank<DataType>(options.dataDims);
        if(not meta.dataByte) meta.dataByte = h5pp::util::getBytesTotal(data, options.dataDims);
        if(not meta.cpp_type) meta.cpp_type = h5pp::type::sfinae::type_name<DataType>();
        if(not meta.h5_space) meta.h5_space = h5pp::util::getMemSpace(meta.dataSize.value(), meta.dataDims.value());
        if(options.mmrySlabs) h5pp::hdf5::selectHyperSlabs(meta.h5_space.value(), options.mmrySlabs.value(), options.mmrySlabSelectOps);
    }

    template<typename DataType>
    inline h5pp::MetaData getMetaData(const DataType &data, const Options &options = Options()) {
        h5pp::MetaData metaData;
        // As long as the two selections have the same number of elements, the data can be transferred
        fillMetaData(data, metaData, options);
        return metaData;
    }

    inline void fillMetaAttr(MetaAttr &meta, const hid::h5f &file, std::string_view attrName, std::string_view linkPath, const PropertyLists &plists = PropertyLists()) {
        h5pp::logger::log->debug("Reading metadata of attribute [{}] in link [{}]", attrName, linkPath);
        if(not meta.linkPath) meta.linkPath = linkPath;
        if(not meta.linkExists) meta.linkExists = h5pp::hdf5::checkIfLinkExists(file, meta.linkPath.value(), std::nullopt, plists.link_access);
        // If the dataset does not exist, there isn't much else to do so we return;
        if(meta.linkExists and not meta.linkExists.value()) return;
        // From here on the link exists
        if(not meta.h5_link) meta.h5_link = h5pp::hdf5::openLink<hid::h5o>(file, linkPath, meta.linkExists, plists.link_access);
        if(not meta.attrName) meta.attrName = attrName;
        if(not meta.attrExists)
            meta.attrExists = h5pp::hdf5::checkIfAttributeExists(file, meta.linkPath.value(), meta.attrName.value(), meta.linkExists.value(), std::nullopt, plists.link_access);
        if(meta.attrExists and not meta.attrExists.value()) return;
        // From here on the attribute exists
        if(not meta.h5_attr) meta.h5_attr = H5Aopen_name(meta.h5_link.value(), meta.attrName.value().c_str());
        if(not meta.h5_type) meta.h5_type = H5Aget_type(meta.h5_attr.value());
        if(not meta.h5_space) meta.h5_space = H5Aget_space(meta.h5_attr.value());
        if(not meta.attrByte) meta.attrByte = h5pp::hdf5::getBytesTotal(meta.h5_attr.value());
        if(not meta.attrSize) meta.attrSize = h5pp::hdf5::getSize(meta.h5_attr.value());
        if(not meta.attrDims) meta.attrDims = h5pp::hdf5::getDimensions(meta.h5_attr.value());
        if(not meta.attrRank) meta.attrRank = h5pp::hdf5::getRank(meta.h5_attr.value());
        if(not meta.h5_plist_attr_create) meta.h5_plist_attr_create  = H5Aget_create_plist(meta.h5_attr.value());
//        if(not meta.h5_plist_attr_access) meta.h5_plist_attr_access  = H5Aget_create_plist(meta.h5_attr.value());
    }

    template<typename DataType>
    inline h5pp::MetaAttr makeMetaAttr(const hid::h5f &     file,
                                       const DataType &     data,
                                       std::string_view     attrName,
                                       std::string_view     linkPath,
                                       const Options &      options = Options(),
                                       const PropertyLists &plists  = PropertyLists()) {
        h5pp::logger::log->debug("Creating new dataset metadata for type [{}]", h5pp::type::sfinae::type_name<DataType>());
        h5pp::MetaAttr meta;
        meta.attrName   = attrName;
        meta.linkPath   = linkPath;
        meta.linkExists = h5pp::hdf5::checkIfLinkExists(file, meta.linkPath.value(), std::nullopt, plists.link_access);
        if(meta.linkExists and meta.linkExists.value()) meta.h5_link = h5pp::hdf5::openLink<hid::h5o>(file, linkPath, meta.linkExists, plists.link_access);
        meta.attrExists = h5pp::hdf5::checkIfAttributeExists(file, meta.linkPath.value(), meta.attrName.value(), meta.linkExists.value(), std::nullopt, plists.link_access);
        meta.h5_type    = h5pp::util::getH5Type<DataType>(options.h5_type);
        meta.h5_plist_attr_create = H5Pcreate(H5P_ATTRIBUTE_CREATE);
        meta.h5_plist_attr_access = H5Pcreate(H5P_ATTRIBUTE_ACCESS);
        meta.attrSize = h5pp::util::getSize(data,options.dataDims);
        meta.attrByte = h5pp::util::getBytesTotal(data);
        meta.attrRank = h5pp::util::getRank<DataType>(options.dataDims);
        meta.attrDims = h5pp::util::getDimensions(data, options.dataDims);
        meta.h5_space = h5pp::util::getDataSpace(meta.attrSize.value(), meta.attrDims.value(), H5D_COMPACT);
//        h5pp::hdf5::setStringSize(meta.h5_type.value(), h5pp::util::getBytesTotal(data));
        h5pp::hdf5::setStringSize(meta.h5_type.value(), H5T_VARIABLE);
        return meta;
    }

    inline h5pp::MetaAttr getMetaAttr(const hid::h5f &file, std::string_view attrName, std::string_view linkPath, const PropertyLists &plists = PropertyLists()) {
        MetaAttr metaAttr;
        fillMetaAttr(metaAttr, file, attrName, linkPath, plists);
        return metaAttr;
    }

    //    inline void printSpace(const hid::h5s & space){
    //        auto size = h5pp::hdf5::getSize(space);
    //        auto rank = h5pp::hdf5::getRank(space);
    //        auto dims = h5pp::hdf5::getDimensions(space);
    //        auto dimsMax = h5pp::hdf5::getMaxDimensions(space);
    //        h5pp::logger::log->trace("Space size {} | rank {} | dims {} | max dims {}",size,rank,dims,dimsMax);
    //
    //    }

    //    inline void
    //        fillMetaSpace(MetaSpace &metaSpace, const MetaData &metaData, const MetaDset &metaDset, const HyperSlab &memSlab = HyperSlab(), const HyperSlab &fileSlab =
    //        HyperSlab()) {
    //        // For this operation to make sense the dataset needs to exist on file already
    //        if(metaDset.dsetExists and not metaDset.dsetExists.value()) throw std::logic_error("Could not setup space information: The dataset does not exist");
    //        if(not metaDset.dsetExists) throw std::logic_error(h5pp::format("Could not setup space information: The given dsetMeta has not been fully initialized"));
    //        if(not metaDset.h5_dset) throw std::logic_error("Could not setup space information: The given dsetMeta struct does not have a valid dataset");
    //
    //        // Setup the fileSpace, i.e. select the region in the dataset that will be read
    //        metaSpace.h5_fileSpace = H5Dget_space(metaDset.h5_dset.value());
    //        h5pp::hdf5::selectHyperslab(metaSpace.h5_fileSpace.value(), fileSlab);
    //
    //        // Setup the memspace, i.e. select the corresponding region in memory that will receive data
    //        metaSpace.h5_memSpace = h5pp::utils::getMemSpace(metaData.dataDims.value());
    //
    //        h5pp::hdf5::selectHyperslab(metaSpace.h5_memSpace.value(), memSlab);
    //        metaSpace.assertSpaceReady();
    //    }
    //
    //    inline h5pp::MetaSpace getMetaSpace(const MetaData &metaData, const MetaDset &metaDset, const HyperSlab &memSlab = HyperSlab(), const HyperSlab &fileSlab = HyperSlab()) {
    //        h5pp::MetaSpace metaSpace;
    //        fillMetaSpace(metaSpace, metaData, metaDset, memSlab, fileSlab);
    //        return metaSpace;
    //    }

    //    inline h5pp::DsetProperties getDatasetProperties_read(const hid::h5f &          file,
    //                                                          std::string_view          dsetName,
    //                                                          const std::optional<bool> dsetExists = std::nullopt,
    //                                                          const PropertyLists &     plists     = PropertyLists()) {
    //        // Use this function to get info from existing datasets on file.
    //        h5pp::logger::log->debug("Scanning properties of existing dataset [{}]", dsetName);
    //
    //        h5pp::DsetProperties dsetProps;
    //        dsetProps.dsetPath   = dsetName;
    //        dsetProps.dsetExists = h5pp::hdf5::checkIfDatasetExists(file, dsetName, dsetExists, plists.link_access);
    //        if(dsetProps.dsetExists.value()) {
    //            dsetProps.dataSet   = h5pp::hdf5::openLink<hid::h5d>(file, dsetProps.dsetPath.value(), dsetProps.dsetExists, dsetProps.plist_dset_access);
    //            dsetProps.dataType  = H5Dget_type(dsetProps.dataSet);
    //            dsetProps.dataSpace = H5Dget_space(dsetProps.dataSet);
    //            dsetProps.memSpace  = H5Dget_space(dsetProps.dataSet);
    //            dsetProps.fileSpace = H5Dget_space(dsetProps.dataSet);
    //
    //            dsetProps.ndims = std::max(1, H5Sget_simple_extent_ndims(dsetProps.dataSpace));
    //            dsetProps.dims  = std::vector<hsize_t>(dsetProps.ndims.value(), 0);
    //            H5Sget_simple_extent_dims(dsetProps.dataSpace, dsetProps.dims.value().data(), nullptr);
    //            dsetProps.size  = h5pp::hdf5::getSize(dsetProps.dataSet);
    //            dsetProps.bytes = H5Dget_storage_size(dsetProps.dataSet);
    //
    //            // We read the layout from file. Note that it is not possible to change the layout on existing datasets! Read more here
    //            // https://support.hdfgroup.org/HDF5/Tutor/layout.html
    //            dsetProps.plist_dset_create = H5Dget_create_plist(dsetProps.dataSet);
    //            dsetProps.layout            = H5Pget_layout(dsetProps.plist_dset_create);
    //            dsetProps.chunkDims         = dsetProps.dims.value(); // Can get modified below
    //            if(dsetProps.layout.value() == H5D_CHUNKED)
    //                H5Pget_chunk(dsetProps.plist_dset_create, dsetProps.ndims.value(), dsetProps.chunkDims.value().data()); // Discard returned chunk rank, it's the same as ndims
    //        } else {
    //            h5pp::logger::log->info("Given dataset name does not point to a dataset: [{}]", dsetName);
    //        }
    //        return dsetProps;
    //    }
    //
    //    template<typename DataType>
    //    h5pp::DsetProperties getDatasetProperties_bootstrap(hid::h5f &                                file,
    //                                                        std::string_view                          dsetName,
    //                                                        const DataType &                          data,
    //                                                        const std::optional<bool>                 dsetExists              = std::nullopt,
    //                                                        const std::optional<hid::h5t>             desiredH5Type           = std::nullopt,
    //                                                        const std::optional<H5D_layout_t>         desiredLayout           = std::nullopt,
    //                                                        const std::optional<std::vector<hsize_t>> desiredChunkDims        = std::nullopt,
    //                                                        const std::optional<unsigned int>         desiredCompressionLevel = std::nullopt,
    //                                                        const PropertyLists &                     plists                  = PropertyLists()) {
    //        h5pp::logger::log->debug("Scanning properties of type [{}] for writing new dataset [{}]", h5pp::type::sfinae::type_name<DataType>(), dsetName);
    //
    //        // Use this function to detect info from the given DataType, to later create a dataset from scratch.
    //        h5pp::DsetProperties dataProps;
    //        dataProps.dsetPath   = dsetName;
    //        dataProps.dsetExists = h5pp::hdf5::checkIfLinkExists(file, dsetName, dsetExists, plists.link_access);
    //        // Infer properties from the given datatype
    //        dataProps.ndims     = h5pp::utils::getRank<DataType>();
    //        dataProps.dims      = h5pp::utils::getDimensions(data);
    //        dataProps.size      = h5pp::utils::getSize(data);
    //        dataProps.bytes     = h5pp::utils::getBytesTotal(data);
    //        dataProps.dataType  = h5pp::utils::getH5Type<DataType>(desiredH5Type);     // We use our own data-type matching unless the user provides a custom one
    //        dataProps.size      = h5pp::hdf5::setStringSize(data, dataProps.dataType); // This only affects strings
    //        dataProps.layout    = h5pp::utils::decideLayout(dataProps.bytes.value(), desiredLayout);
    //        dataProps.chunkDims = h5pp::utils::getDefaultChunkDimensions(dataProps.size.value(), dataProps.dims.value(), desiredChunkDims);
    //        dataProps.memSpace  = h5pp::utils::getMemSpace(dataProps.dims.value());
    //        dataProps.dataSpace = h5pp::utils::getDataSpace(dataProps.dims.value(), dataProps.layout.value());
    //
    //        if(desiredCompressionLevel) {
    //            dataProps.compressionLevel = std::min((unsigned int) 9, desiredCompressionLevel.value());
    //        } else {
    //            dataProps.compressionLevel = 0;
    //        }
    //        dataProps.plist_dset_create = H5Pcreate(H5P_DATASET_CREATE);
    //        h5pp::hdf5::setDatasetCreationPropertyLayout(dataProps);
    //        h5pp::hdf5::setDatasetCreationPropertyCompression(dataProps);
    //        //        h5pp::hdf5::setDataSpaceExtent(dataProps);
    //        return dataProps;
    //    }
    //
    //    template<typename DataType>
    //    h5pp::DsetProperties getDatasetProperties_write(hid::h5f &                                file,
    //                                                    std::string_view                          dsetName,
    //                                                    const DataType &                          data,
    //                                                    std::optional<bool>                       dsetExists              = std::nullopt,
    //                                                    const std::optional<hid::h5t>             desiredH5Type           = std::nullopt,
    //                                                    const std::optional<H5D_layout_t>         desiredLayout           = std::nullopt,
    //                                                    const std::optional<std::vector<hsize_t>> desiredChunkDims        = std::nullopt,
    //                                                    const std::optional<unsigned int>         desiredCompressionLevel = std::nullopt,
    //                                                    const PropertyLists &                     plists                  = PropertyLists()) {
    //        h5pp::logger::log->debug("Scanning properties of type [{}] for writing into dataset [{}]", h5pp::type::sfinae::type_name<DataType>(), dsetName);
    //
    //        if(not dsetExists) dsetExists = h5pp::hdf5::checkIfLinkExists(file, dsetName, dsetExists, plists.link_access);
    //
    //        if(dsetExists.value()) {
    //            // We enter overwrite-mode
    //            // Here we get dataset properties which with which we can update datasets
    //            auto dsetProps = getDatasetProperties_read(file, dsetName, dsetExists, plists);
    //
    //            if(dsetProps.ndims != h5pp::utils::getRank<DataType>())
    //                throw std::runtime_error("The number of dimensions in the existing dataset [" + std::string(dsetName) + "] is (" + std::to_string(dsetProps.ndims.value()) +
    //                                         "), which differs from the dimensions in given data (" + std::to_string(h5pp::utils::getRank<DataType>()) + ")");
    //
    //            if(not h5pp::hdf5::H5Tequal_recurse(dsetProps.dataType, h5pp::utils::getH5Type<DataType>()))
    //                throw std::runtime_error("Given datatype does not match the type of an existing dataset: " + dsetProps.dsetPath.value());
    //
    //            h5pp::DsetProperties dataProps;
    //            // Start by copying properties that are immutable on overwrites
    //            dataProps.dataSet           = dsetProps.dataSet;
    //            dataProps.dsetPath          = dsetProps.dsetPath;
    //            dataProps.dsetExists        = dsetProps.dsetExists;
    //            dataProps.layout            = dsetProps.layout;
    //            dataProps.dataType          = h5pp::utils::getH5Type<DataType>(desiredH5Type);
    //            dataProps.ndims             = h5pp::utils::getRank<DataType>();
    //            dataProps.chunkDims         = dsetProps.chunkDims;
    //            dataProps.plist_dset_access = dsetProps.plist_dset_create;
    //
    //            // The rest we can inferr directly from the data
    //            dataProps.dims  = h5pp::utils::getDimensions(data);
    //            dataProps.size  = h5pp::utils::getSize(data);
    //            dataProps.size  = h5pp::hdf5::setStringSize(data, dataProps.dataType); // This only affects strings
    //            dataProps.bytes = h5pp::utils::getBytesTotal(data);
    //
    //            dataProps.memSpace         = h5pp::utils::getMemSpace(dataProps.dims.value());
    //            dataProps.dataSpace        = h5pp::utils::getDataSpace(dataProps.dims.value(), dataProps.layout.value());
    //            dataProps.compressionLevel = 0; // Not used when overwriting
    //                                            //            h5pp::hdf5::setDataSpaceExtent(dataProps);
    //
    //            // Make some sanity checks on sizes
    //            auto dsetMaxDims = h5pp::hdf5::getMaxDimensions(dsetProps.dataSet);
    //            for(int idx = 0; idx < dsetProps.ndims.value(); idx++) {
    //                if(dsetMaxDims[idx] != H5S_UNLIMITED and dataProps.layout.value() == H5D_CHUNKED and dataProps.dims.value()[idx] > dsetMaxDims[idx])
    //                    throw std::runtime_error("Dimension too large. Existing dataset [" + dsetProps.dsetPath.value() + "] has a maximum size [" +
    //                    std::to_string(dsetMaxDims[idx]) +
    //                                             "] in dimension [" + std::to_string(idx) + "], but the given data has size [" + std::to_string(dataProps.dims.value()[idx]) +
    //                                             "] in the same dimension. The dataset has layout H5D_CHUNKED but the dimension is not tagged with H5S_UNLIMITED");
    //                if(dsetMaxDims[idx] != H5S_UNLIMITED and dataProps.layout.value() != H5D_CHUNKED and dataProps.dims.value()[idx] != dsetMaxDims[idx])
    //                    throw std::runtime_error(h5pp::format("Dimensions not equal. Existing dataset [{}] has a maximum size [{}] in dimension [{}], but the given data has size
    //                    [{}] "
    //                                                          "in that same dimension. Consider using layout H5D_CHUNKED for resizeable datasets",
    //                                                          dsetProps.dsetPath.value(),
    //                                                          dsetMaxDims[idx],
    //                                                          idx,
    //                                                          dataProps.dims.value()[idx]));
    //            }
    //            return dataProps;
    //        } else {
    //            // We enter write-from-scratch mode
    //            // Use this function to detect info from the given DataType, to later create a dataset from scratch.
    //            return getDatasetProperties_bootstrap(file, dsetName, data, dsetExists, desiredH5Type, desiredLayout, desiredChunkDims, desiredCompressionLevel, plists);
    //        }
    //    }

    //    inline h5pp::AttributeProperties getAttributeProperties_read(const hid::h5f &          file,
    //                                                                 std::string_view          attrName,
    //                                                                 std::string_view          linkName,
    //                                                                 const std::optional<bool> attrExists = std::nullopt,
    //                                                                 const std::optional<bool> linkExists = std::nullopt,
    //                                                                 const PropertyLists &     plists     = PropertyLists()) {
    //        // Use this function to get info from existing attributes on file.
    //        h5pp::logger::log->debug("Scanning properties of existing attribute [{}] in link [{}]", attrName, linkName);
    //        h5pp::AttributeProperties attrProps;
    //        attrProps.linkExists = h5pp::hdf5::checkIfLinkExists(file, linkName, linkExists, plists.link_access);
    //        attrProps.attrExists = h5pp::hdf5::checkIfAttributeExists(file, linkName, attrName, attrProps.linkExists, attrExists, plists.link_access);
    //        attrProps.linkName   = linkName;
    //        attrProps.attrName   = attrName;
    //
    //        if(attrProps.attrExists.value()) {
    //            attrProps.linkObject  = h5pp::hdf5::openLink<hid::h5o>(file, attrProps.linkName.value(), attrProps.linkExists, attrProps.plist_attr_access);
    //            attrProps.attributeId = H5Aopen_name(attrProps.linkObject, std::string(attrProps.attrName.value()).c_str());
    //            H5I_type_t linkType   = H5Iget_type(attrProps.attributeId.value());
    //            if(linkType != H5I_ATTR) { throw std::runtime_error("Given attribute name does not point to an attribute: [{" + attrProps.attrName.value() + "]"); }
    //
    //            attrProps.dataType = H5Aget_type(attrProps.attributeId);
    //            attrProps.memSpace = H5Aget_space(attrProps.attributeId);
    //
    //            attrProps.ndims = std::max(1, H5Sget_simple_extent_ndims(attrProps.memSpace));
    //            attrProps.dims  = std::vector<hsize_t>(attrProps.ndims.value(), 0);
    //            H5Sget_simple_extent_dims(attrProps.memSpace, attrProps.dims.value().data(), nullptr);
    //            attrProps.size  = H5Sget_simple_extent_npoints(attrProps.memSpace);
    //            attrProps.bytes = H5Aget_storage_size(attrProps.attributeId);
    //        }
    //        return attrProps;
    //    }

    //    template<typename DataType>
    //    inline h5pp::AttributeProperties getAttributeProperties_bootstrap(const hid::h5f &              file,
    //                                                                      const DataType &              data,
    //                                                                      std::string_view              attrName,
    //                                                                      std::string_view              linkName,
    //                                                                      std::optional<bool>           attrExists    = std::nullopt,
    //                                                                      std::optional<bool>           linkExists    = std::nullopt,
    //                                                                      const std::optional<hid::h5t> desiredH5Type = std::nullopt,
    //                                                                      const PropertyLists &         plists        = PropertyLists()) {
    //        // Use this function to get info from existing datasets on file.
    //        h5pp::logger::log->debug("Scanning properties of type [{}] for writing new attribute [{}] in link [{}]", h5pp::type::sfinae::type_name<DataType>(), attrName,
    //        linkName); h5pp::AttributeProperties dataProps; dataProps.linkExists = h5pp::hdf5::checkIfLinkExists(file, linkName, linkExists, plists.link_access);
    //        dataProps.attrExists = h5pp::hdf5::checkIfAttributeExists(file, linkName, attrName, dataProps.linkExists, attrExists, plists.link_access);
    //        dataProps.linkName   = linkName;
    //        dataProps.attrName   = attrName;
    //        if(dataProps.linkExists and dataProps.linkExists.value())
    //            dataProps.linkObject = h5pp::hdf5::openObject(file, dataProps.linkName.value(), dataProps.linkExists, plists.link_access);
    //        if(dataProps.attrExists and dataProps.attrExists.value()) dataProps.attributeId = H5Aopen_name(dataProps.linkObject, std::string(dataProps.attrName.value()).c_str());
    //
    //        dataProps.dataType = h5pp::util::getH5Type<DataType>(desiredH5Type);
    //        dataProps.size     = h5pp::util::getSize(data);
    //        dataProps.size     = h5pp::hdf5::setStringSize(dataProps.dataType.value(), dataProps.size.value()); // This only affects strings
    //        dataProps.bytes    = h5pp::util::getBytesTotal<DataType>(data);
    //        dataProps.ndims    = h5pp::util::getRank<DataType>();
    //        dataProps.dims     = h5pp::util::getDimensions(data);
    //        dataProps.memSpace = h5pp::util::getMemSpace(dataProps.dims.value());
    //        return dataProps;
    //    }
    //
    //    template<typename DataType>
    //    inline h5pp::AttributeProperties getAttributeProperties_write(const hid::h5f &              file,
    //                                                                  const DataType &              data,
    //                                                                  std::string_view              attrName,
    //                                                                  std::string_view              linkName,
    //                                                                  std::optional<bool>           attrExists    = std::nullopt,
    //                                                                  std::optional<bool>           linkExists    = std::nullopt,
    //                                                                  const std::optional<hid::h5t> desiredH5Type = std::nullopt,
    //                                                                  const PropertyLists &         plists        = PropertyLists()) {
    //        // Use this function to get info from existing datasets on file.
    //        h5pp::logger::log->debug("Scanning properties of type [{}] for writing into attribute [{}] in link [{}]", h5pp::type::sfinae::type_name<DataType>(), attrName,
    //        linkName);
    //
    //        linkExists = h5pp::hdf5::checkIfLinkExists(file, linkName, linkExists, plists.link_access);
    //        attrExists = h5pp::hdf5::checkIfAttributeExists(file, linkName, attrName, linkExists, std::nullopt, plists.link_access);
    //
    //        if(linkExists.value() and attrExists.value()) {
    //            // We enter overwrite mode
    //            h5pp::logger::log->trace("Attribute [{}] exists in link [{}]", attrName, linkName);
    //            auto attrProps = getAttributeProperties_read(file, attrName, linkName, attrExists, linkExists, plists);
    //            // Sanity check
    //            if(attrProps.ndims != h5pp::util::getRank<DataType>())
    //                throw std::runtime_error("Number of dimensions in existing attribute [" + std::string(attrName) + "] (" + std::to_string(attrProps.ndims.value()) +
    //                                         ") differ from dimensions in given data (" + std::to_string(h5pp::util::getRank<DataType>()) + ")");
    //
    //            if(not h5pp::hdf5::H5Tequal_recurse(attrProps.dataType, h5pp::util::getH5Type<DataType>()))
    //                throw std::runtime_error("Given datatype does not match the type of an existing dataset: " + attrProps.linkName.value());
    //
    //            h5pp::AttributeProperties dataProps = attrProps;
    //            // Start by copying properties that are immutable on overwrites
    //            dataProps.plist_attr_access = attrProps.plist_attr_create;
    //            dataProps.ndims             = h5pp::util::getRank<DataType>();
    //            // The rest we can inferr directly from the data
    //            dataProps.dataType = h5pp::util::getH5Type<DataType>(desiredH5Type);
    //            dataProps.size     = h5pp::util::getSize(data);
    //            dataProps.size     = h5pp::hdf5::setStringSize(dataProps.dataType.value(), dataProps.size.value()); // This only affects strings
    //            dataProps.bytes    = h5pp::util::getBytesTotal<DataType>(data);
    //            dataProps.ndims    = h5pp::util::getRank<DataType>();
    //            dataProps.dims     = h5pp::util::getDimensions(data);
    //            dataProps.memSpace = h5pp::util::getMemSpace(dataProps.dims.value());
    //            return dataProps;
    //
    //        } else {
    //            h5pp::logger::log->trace("Attribute [{}] does not exists in link [{}]", attrName, linkName);
    //            return getAttributeProperties_bootstrap(file, data, attrName, linkName, attrExists, linkExists, desiredH5Type, plists);
    //        }
    //    }

    inline h5pp::TableProperties getTableProperties_bootstrap(const hid::h5t &                  entryType,
                                                              std::string_view                  tableName,
                                                              std::string_view                  tableTitle,
                                                              const std::optional<hsize_t>      desiredChunkSize        = std::nullopt,
                                                              const std::optional<unsigned int> desiredCompressionLevel = std::nullopt) {
        TableProperties tableProps;
        tableProps.entryType  = entryType;
        tableProps.tableTitle = tableTitle;
        tableProps.tableName  = tableName;
        tableProps.groupName  = "";
        size_t pos            = tableProps.tableName.value().find_last_of('/');
        if(pos != std::string::npos) tableProps.groupName.value().assign(tableProps.tableName.value().begin(), tableProps.tableName.value().begin() + pos);

        tableProps.NFIELDS          = H5Tget_nmembers(entryType);
        tableProps.NRECORDS         = 0;
        tableProps.chunkSize        = desiredChunkSize.has_value() ? desiredChunkSize : 10;
        tableProps.entrySize        = H5Tget_size(tableProps.entryType);
        tableProps.compressionLevel = h5pp::hdf5::getValidCompressionLevel(desiredCompressionLevel);

        tableProps.fieldTypes   = std::vector<h5pp::hid::h5t>();
        tableProps.fieldOffsets = std::vector<size_t>();
        tableProps.fieldSizes   = std::vector<size_t>();
        tableProps.fieldNames   = std::vector<std::string>();

        for(unsigned int idx = 0; idx < (unsigned int) tableProps.NFIELDS.value(); idx++) {
            tableProps.fieldTypes.value().emplace_back(H5Tget_member_type(tableProps.entryType, idx));
            tableProps.fieldOffsets.value().emplace_back(H5Tget_member_offset(tableProps.entryType, idx));
            tableProps.fieldSizes.value().emplace_back(H5Tget_size(tableProps.fieldTypes.value().back()));
            const char *name = H5Tget_member_name(tableProps.entryType, idx);
            tableProps.fieldNames.value().emplace_back(name);
            H5free_memory((void *) name);
        }
        return tableProps;
    }

    template<typename DataType>
    inline h5pp::TableProperties getTableProperties_write(const hid::h5f &file, const DataType &data, std::string_view tableName, const PropertyLists &plists = PropertyLists()) {
        hid::h5d dataset    = h5pp::hdf5::openLink<hid::h5d>(file, tableName, std::nullopt, plists.link_access);
        hid::h5t entryType  = H5Dget_type(dataset);
        auto     tableProps = getTableProperties_bootstrap(entryType, tableName, "", std::nullopt, std::nullopt);
        tableProps.NRECORDS = h5pp::util::getSize(data);
        return tableProps;
    }

    inline h5pp::TableProperties getTableProperties_read(const hid::h5f &file, std::string_view tableName, const PropertyLists &plists = PropertyLists()) {
        hid::h5d dataset    = h5pp::hdf5::openLink<hid::h5d>(file, tableName, std::nullopt, plists.link_access);
        hid::h5t entryType  = H5Dget_type(dataset);
        auto     tableProps = getTableProperties_bootstrap(entryType, tableName, "", std::nullopt, std::nullopt);
        hsize_t  NFIELDS, NRECORDS;
        herr_t   err = H5TBget_table_info(file, std::string(tableName).c_str(), &NFIELDS, &NRECORDS);
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to get table information");
        }
        tableProps.NFIELDS  = NFIELDS;
        tableProps.NRECORDS = NRECORDS;
        return tableProps;
    }

}
