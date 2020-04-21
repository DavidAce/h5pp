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
            meta.chunkDims = std::vector<hsize_t>((size_t) meta.dsetRank.value(), 0);
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
        meta.dsetPath   = h5pp::util::safe_str(dsetPath);
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
        h5pp::hdf5::setStringSize(data,meta.h5_type.value(), options.dataDims);
        h5pp::hdf5::setProperty_layout(meta); // Must go before setting chunk dims
        h5pp::hdf5::setProperty_chunkDims(meta);
        h5pp::hdf5::setProperty_compression(meta);
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
        if(not meta.h5_attr) meta.h5_attr = H5Aopen_name(meta.h5_link.value(), h5pp::util::safe_str(meta.attrName.value()).c_str());
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
#if H5_VERSION_GE(1,10,0)
        meta.h5_plist_attr_access = H5Pcreate(H5P_ATTRIBUTE_ACCESS);
#else
        meta.h5_plist_attr_access = H5Pcreate(H5P_ATTRIBUTE_CREATE); //Missing access property in HDF5 1.8.x
#endif
        meta.attrDims = h5pp::util::getDimensions(data, options.dataDims);
        meta.attrSize = h5pp::util::getSize(data,options.dataDims);
        meta.attrByte = h5pp::util::getBytesTotal(data);
        meta.attrRank = h5pp::util::getRank<DataType>(options.dataDims);
        meta.h5_space = h5pp::util::getDataSpace(meta.attrSize.value(), meta.attrDims.value(), H5D_COMPACT);
        h5pp::hdf5::setStringSize(data,meta.h5_type.value(), options.dataDims);
//        h5pp::hdf5::setStringSize(meta.h5_type.value(), h5pp::util::getBytesTotal(data));
//        h5pp::hdf5::setStringSize(meta.h5_type.value(), H5T_VARIABLE);
        return meta;
    }

    inline h5pp::MetaAttr getMetaAttr(const hid::h5f &file, std::string_view attrName, std::string_view linkPath, const PropertyLists &plists = PropertyLists()) {
        MetaAttr metaAttr;
        fillMetaAttr(metaAttr, file, attrName, linkPath, plists);
        return metaAttr;
    }

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
        if(pos != std::string::npos) tableProps.groupName.value().assign(tableProps.tableName.value().begin(), tableProps.tableName.value().begin() + (long) pos);

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
        herr_t   err = H5TBget_table_info(file, h5pp::util::safe_str(tableName).c_str(), &NFIELDS, &NRECORDS);
        if(err < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to get table information");
        }
        tableProps.NFIELDS  = NFIELDS;
        tableProps.NRECORDS = NRECORDS;
        return tableProps;
    }

}
