#include "h5ppAttributeProperties.h"
#include "h5ppConstants.h"
#include "h5ppHdf5.h"
#include "h5ppUtils.h"
namespace h5pp::Analyze {

    inline h5pp::DatasetProperties getDatasetProperties_read(const Hid::h5f &          file,
                                                             std::string_view          dsetName,
                                                             const std::optional<bool> dsetExists = std::nullopt,
                                                             const PropertyLists &     plists     = PropertyLists()) {
        // Use this function to get info from existing datasets on file.
        h5pp::Logger::log->trace("Reading properties of dataset: [{}] from file", dsetName);
        h5pp::DatasetProperties dsetProps;
        dsetProps.dsetName   = dsetName;
        dsetProps.dsetExists = h5pp::Hdf5::checkIfLinkExists(file, dsetName, dsetExists, plists);
        if(dsetProps.dsetExists.value()) {
            dsetProps.dataSet   = h5pp::Hdf5::openObject<Hid::h5d>(file, dsetProps.dsetName.value(), H5P_DEFAULT, dsetProps.dsetExists);
            H5I_type_t linkType = H5Iget_type(dsetProps.dataSet.value());
            if(linkType != H5I_DATASET) { throw std::runtime_error("Given path does not point to a dataset: [{" + dsetProps.dsetName.value() + "]"); }

            dsetProps.dataType  = H5Dget_type(dsetProps.dataSet);
            dsetProps.dataSpace = H5Dget_space(dsetProps.dataSet);
            dsetProps.memSpace  = H5Dget_space(dsetProps.dataSet);
            ;
            dsetProps.ndims = H5Sget_simple_extent_ndims(dsetProps.dataSpace);
            dsetProps.dims  = std::vector<hsize_t>(dsetProps.ndims.value());
            H5Sget_simple_extent_dims(dsetProps.dataSpace, dsetProps.dims.value().data(), nullptr);
            dsetProps.size  = std::accumulate(dsetProps.dims.value().begin(), dsetProps.dims.value().end(), 1, std::multiplies<>());
            dsetProps.bytes = H5Dget_storage_size(dsetProps.dataSet);

            // We read the layout from file. Note that it is not possible to change the layout on existing datasets! Read more here
            // https://support.hdfgroup.org/HDF5/Tutor/layout.html
            dsetProps.plist_dset_create = H5Dget_create_plist(dsetProps.dataSet);
            dsetProps.layout            = H5Pget_layout(dsetProps.plist_dset_create);
            dsetProps.chunkDims         = dsetProps.dims.value(); // Can get modified below
            if(dsetProps.layout.value() == H5D_CHUNKED)
                H5Pget_chunk(dsetProps.plist_dset_create, dsetProps.ndims.value(), dsetProps.chunkDims.value().data()); // Discard returned chunk rank, it's the same as ndims
        }
        return dsetProps;
    }

    template<typename DataType>
    h5pp::DatasetProperties getDatasetProperties_bootstrap(Hid::h5f &                                file,
                                                           std::string_view                          dsetName,
                                                           const DataType &                          data,
                                                           const std::optional<bool>                 dsetExists              = std::nullopt,
                                                           const std::optional<H5D_layout_t>         desiredLayout           = std::nullopt,
                                                           const std::optional<std::vector<hsize_t>> desiredChunkDims        = std::nullopt,
                                                           const std::optional<unsigned int>         desiredCompressionLevel = std::nullopt,
                                                           const PropertyLists &                     plists                  = PropertyLists()) {
        h5pp::Logger::log->trace("Inferring properties for future dataset: [{}] from type", dsetName);

        // Use this function to detect info from the given DataType, to later create a dataset from scratch.
        h5pp::DatasetProperties dataProps;
        dataProps.dsetName   = dsetName;
        dataProps.dsetExists = h5pp::Hdf5::checkIfLinkExists(file, dsetName, dsetExists, plists);
        // Infer properties from the given datatype
        dataProps.ndims     = h5pp::Utils::getRank<DataType>();
        dataProps.dims      = h5pp::Utils::getDimensions(data);
        dataProps.size      = h5pp::Utils::getSize(data);
        dataProps.bytes     = h5pp::Utils::getBytesTotal(data);
        dataProps.dataType  = h5pp::Utils::getH5DataType<DataType>();               // We use our own data-type matching to avoid any confusion
        dataProps.size      = h5pp::Utils::setStringSize(data, dataProps.dataType); // This only affects strings
        dataProps.layout    = h5pp::Utils::decideLayout(dataProps.size.value(), dataProps.bytes.value(), desiredLayout);
        dataProps.chunkDims = h5pp::Utils::getDefaultChunkDimensions(dataProps.size.value(), dataProps.dims.value(), desiredChunkDims);
        dataProps.memSpace  = h5pp::Utils::getMemSpace(dataProps.size.value(), dataProps.ndims.value(), dataProps.dims.value());
        dataProps.dataSpace = h5pp::Utils::getDataSpace(dataProps.size.value(), dataProps.ndims.value(), dataProps.dims.value(), dataProps.layout.value());

        if(desiredCompressionLevel) {
            dataProps.compressionLevel = std::min((unsigned int) 9, desiredCompressionLevel.value());
        } else {
            dataProps.compressionLevel = 0;
        }
        dataProps.plist_dset_create = H5Pcreate(H5P_DATASET_CREATE);
        h5pp::Hdf5::setDatasetCreationPropertyLayout(dataProps);
        h5pp::Hdf5::setDatasetCreationPropertyCompression(dataProps);
        h5pp::Hdf5::setDataSpaceExtent(dataProps);
        return dataProps;
    }

    template<typename DataType>
    h5pp::DatasetProperties getDatasetProperties_write(Hid::h5f &                                file,
                                                       std::string_view                          dsetName,
                                                       const DataType &                          data,
                                                       std::optional<bool>                       dsetExists              = std::nullopt,
                                                       const std::optional<H5D_layout_t>         desiredLayout           = std::nullopt,
                                                       const std::optional<std::vector<hsize_t>> desiredChunkDims        = std::nullopt,
                                                       const std::optional<unsigned int>         desiredCompressionLevel = std::nullopt,
                                                       const PropertyLists &                     plists                  = PropertyLists()) {
        h5pp::Logger::log->trace("Reading properties for writing into dataset: [{}]", dsetName);

        if(not dsetExists) dsetExists = h5pp::Hdf5::checkIfLinkExists(file, dsetName, dsetExists, plists);

        if(dsetExists.value()) {
            // We enter overwrite-mode
            // Here we get dataset properties which with which we can update datasets
            auto dsetProps = getDatasetProperties_read(file, dsetName, dsetExists, plists);

            if(dsetProps.ndims != h5pp::Utils::getRank<DataType>())
                throw std::runtime_error("Number of dimensions in existing dataset (" + std::to_string(dsetProps.ndims.value()) + ") differ from dimensions in given data (" +
                                         std::to_string(h5pp::Utils::getRank<DataType>()) + ")");

            if(not h5pp::Utils::checkEqualTypesRecursive(dsetProps.dataType, h5pp::Utils::getH5DataType<DataType>()))
                throw std::runtime_error("Given datatype does not match the type of an existing dataset: " + dsetProps.dsetName.value());

            h5pp::DatasetProperties dataProps;
            // Start by copying properties that are immutable on overwrites
            dataProps.dataSet    = dsetProps.dataSet;
            dataProps.dsetName   = dsetProps.dsetName;
            dataProps.dsetExists = dsetProps.dsetExists;
            dataProps.layout     = dsetProps.layout;
            dataProps.dataType   = h5pp::Utils::getH5DataType<DataType>();
            dataProps.ndims      = h5pp::Utils::getRank<DataType>();
            dataProps.chunkDims  = dsetProps.chunkDims;

            // The rest we can inferr directly from the data
            dataProps.dims             = h5pp::Utils::getDimensions(data);
            dataProps.size             = h5pp::Utils::getSize(data);
            dataProps.bytes            = h5pp::Utils::getBytesTotal(data);
            dataProps.size             = h5pp::Utils::setStringSize(data, dataProps.dataType); // This only affects strings
            dataProps.memSpace         = h5pp::Utils::getMemSpace(dataProps.size.value(), dataProps.ndims.value(), dataProps.dims.value());
            dataProps.dataSpace        = h5pp::Utils::getDataSpace(dataProps.size.value(), dataProps.ndims.value(), dataProps.dims.value(), dataProps.layout.value());
            dataProps.compressionLevel = 0; // Not used when overwriting
            h5pp::Hdf5::setDataSpaceExtent(dataProps);

            // Make some sanity checks on sizes
            auto dsetMaxDims = h5pp::Hdf5::getMaxDims(dsetProps);
            for(size_t idx = 0; idx < dsetProps.ndims.value(); idx++) {
                if(dsetMaxDims[idx] != H5S_UNLIMITED and dataProps.layout.value() == H5D_CHUNKED and dataProps.dims.value()[idx] > dsetMaxDims[idx])
                    throw std::runtime_error("Dimension too large. Existing dataset [" + dsetProps.dsetName.value() + "] has a maximum size [" + std::to_string(dsetMaxDims[idx]) +
                                             "] in dimension [" + std::to_string(idx) + "], but the given data has size [" + std::to_string(dataProps.dims.value()[idx]) +
                                             "] in the same dimension. The dataset has layout H5D_CHUNKED but the dimension is not tagged with H5S_UNLIMITED");
                if(dsetMaxDims[idx] != H5S_UNLIMITED and dataProps.layout.value() != H5D_CHUNKED and dataProps.dims.value()[idx] != dsetMaxDims[idx])
                    throw std::runtime_error("Dimensions not equal. Existing dataset [" + dsetProps.dsetName.value() + "] has a maximum size [" + std::to_string(dsetMaxDims[idx]) +
                                             "] in dimension [" + std::to_string(idx) + "], but the given data has size [" + std::to_string(dataProps.dims.value()[idx]) +
                                             "] in the same dimension. Consider using H5D_CHUNKED layout for resizeable datasets");
            }
            return dataProps;
        } else {
            // We enter write-from-scratch mode
            // Use this function to detect info from the given DataType, to later create a dataset from scratch.
            return getDatasetProperties_bootstrap(file, dsetName, data, dsetExists, desiredLayout, desiredChunkDims, desiredCompressionLevel, plists);
        }
    }

    inline h5pp::DatasetProperties getAttributeProperties_read(const Hid::h5f &          file,
                                                               std::string_view          attrName,
                                                               std::string_view          linkName,
                                                               const std::optional<bool> attrExists = std::nullopt,
                                                               const PropertyLists &     plists     = PropertyLists()) {
        //        aprops.dataType = h5pp::Utils::getH5DataType<DataType>();
        //        aprops.size     = h5pp::Utils::getSize(attribute);
        //        aprops.byteSize = h5pp::Utils::getBytesTotal<DataType>(attribute);
        //        aprops.ndims    = h5pp::Utils::getRank<DataType>();
        //        aprops.dims     = h5pp::Utils::getDimensions(attribute);
        //        aprops.attrName = attributeName;
        //        aprops.linkPath = linkPath;
        //        aprops.memSpace = h5pp::Utils::getMemSpace(aprops.size, aprops.ndims, aprops.dims);

        // Use this function to get info from existing datasets on file.
        h5pp::Logger::log->trace("Reading properties of attribute: [{}] in link [{}] from file", attrName, linkName);
        h5pp::AttributeProperties dsetProps;
        dsetProps.dsetName   = dsetName;
        dsetProps.dsetExists = h5pp::Hdf5::checkIfLinkExists(file, dsetName, dsetExists, plists);
        if(dsetProps.dsetExists.value()) {
            dsetProps.dataSet   = h5pp::Hdf5::openObject<Hid::h5d>(file, dsetProps.dsetName.value(), H5P_DEFAULT, dsetProps.dsetExists);
            H5I_type_t linkType = H5Iget_type(dsetProps.dataSet.value());
            if(linkType != H5I_DATASET) { throw std::runtime_error("Given path does not point to a dataset: [{" + dsetProps.dsetName.value() + "]"); }

            dsetProps.dataType  = H5Dget_type(dsetProps.dataSet);
            dsetProps.dataSpace = H5Dget_space(dsetProps.dataSet);
            dsetProps.memSpace  = H5Dget_space(dsetProps.dataSet);
            ;
            dsetProps.ndims = H5Sget_simple_extent_ndims(dsetProps.dataSpace);
            dsetProps.dims  = std::vector<hsize_t>(dsetProps.ndims.value());
            H5Sget_simple_extent_dims(dsetProps.dataSpace, dsetProps.dims.value().data(), nullptr);
            dsetProps.size  = std::accumulate(dsetProps.dims.value().begin(), dsetProps.dims.value().end(), 1, std::multiplies<>());
            dsetProps.bytes = H5Dget_storage_size(dsetProps.dataSet);

            // We read the layout from file. Note that it is not possible to change the layout on existing datasets! Read more here
            // https://support.hdfgroup.org/HDF5/Tutor/layout.html
            dsetProps.plist_dset_create = H5Dget_create_plist(dsetProps.dataSet);
            dsetProps.layout            = H5Pget_layout(dsetProps.plist_dset_create);
            dsetProps.chunkDims         = dsetProps.dims.value(); // Can get modified below
            if(dsetProps.layout.value() == H5D_CHUNKED)
                H5Pget_chunk(dsetProps.plist_dset_create, dsetProps.ndims.value(), dsetProps.chunkDims.value().data()); // Discard returned chunk rank, it's the same as ndims
        }
        return dsetProps;
    }

}
