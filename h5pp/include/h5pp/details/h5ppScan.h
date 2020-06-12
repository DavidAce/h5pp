#include "h5ppConstants.h"
#include "h5ppHdf5.h"
#include "h5ppInfo.h"
#include "h5ppTypeSfinae.h"
#include "h5ppUtils.h"

namespace h5pp::scan {

    /*! \fn fillDsetInfo
     * Fills missing information about a dataset in an info-struct
     *
     * @param info A struct with information about a dataset
     * @param file a h5f file identifier
     * @param dsetPath the full path to a dataset in an HDF5 file
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     */
    inline void
        fillDsetInfo(h5pp::DsetInfo &info, const hid::h5f &file, std::string_view dsetPath, const Options &options = Options(), const PropertyLists &plists = PropertyLists()) {
        h5pp::logger::log->debug("Reading metadata of dataset [{}]", dsetPath);
        if(not info.h5_file) info.h5_file = file;
        if(not info.dsetPath) info.dsetPath = dsetPath;
        if(not info.dsetExists) info.dsetExists = h5pp::hdf5::checkIfDatasetExists(info.h5_file.value(), info.dsetPath.value(), std::nullopt, plists.link_access);
        // If the dataset does not exist, there isn't much else to do so we return;
        if(info.dsetExists and not info.dsetExists.value()) return;
        // From here on the dataset exists
        if(not info.h5_dset) info.h5_dset = h5pp::hdf5::openLink<hid::h5d>(file, dsetPath, info.dsetExists, plists.link_access);
        if(not info.h5_type) info.h5_type = H5Dget_type(info.h5_dset.value());
        if(not info.h5_space) info.h5_space = H5Dget_space(info.h5_dset.value());

        // If no slab is given the default space will be the whole dataset
        if(options.fileSlabs) h5pp::hdf5::selectHyperSlabs(info.h5_space.value(), options.fileSlabs.value(), options.fileSlabSelectOps);

        // Get the properties of the selected space
        if(not info.dsetByte) info.dsetByte = h5pp::hdf5::getBytesTotal(info.h5_dset.value());
        if(not info.dsetSize) info.dsetSize = h5pp::hdf5::getSize(info.h5_dset.value());
        if(not info.dsetRank) info.dsetRank = h5pp::hdf5::getRank(info.h5_dset.value());
        if(not info.dsetDims) info.dsetDims = h5pp::hdf5::getDimensions(info.h5_dset.value());
        if(not info.dsetDimsMax) info.dsetDimsMax = h5pp::hdf5::getMaxDimensions(info.h5_dset.value());
        // We read the layout from file. Note that it is not possible to change the layout on existing datasets! Read more here
        // https://support.hdfgroup.org/HDF5/Tutor/layout.html
        if(not info.h5_plist_dset_create) info.h5_plist_dset_create = H5Dget_create_plist(info.h5_dset.value());
        if(not info.h5_plist_dset_access) info.h5_plist_dset_access = H5Dget_access_plist(info.h5_dset.value());
        if(not info.h5_layout) info.h5_layout = H5Pget_layout(info.h5_plist_dset_create.value());
        if(not info.chunkDims and info.h5_layout.value() == H5D_CHUNKED) {
            info.chunkDims = std::vector<hsize_t>((size_t) info.dsetRank.value(), 0);
            int success    = H5Pget_chunk(info.h5_plist_dset_create.value(), info.dsetRank.value(), info.chunkDims.value().data());
            if(success < 0) {
                H5Eprint(H5E_DEFAULT, stderr);
                throw std::runtime_error(h5pp::format("Failed to read chunk dimensions of dataset [{}]", info.dsetPath.value()));
            }
        }
    }

    template<typename DataType>
    inline h5pp::DsetInfo
        newDsetInfo(const hid::h5f &file, const DataType &data, std::string_view dsetPath, const Options &options = Options(), const PropertyLists &plists = PropertyLists()) {
        h5pp::DsetInfo info;
        info.dsetExists = h5pp::hdf5::checkIfDatasetExists(file, dsetPath, std::nullopt, plists.link_access);
        if(info.dsetExists.value()) {
            fillDsetInfo(info, file, dsetPath, options, plists);
            return info;
        }
        h5pp::logger::log->debug("Creating dataset info for type [{}]", h5pp::type::sfinae::type_name<DataType>());
        info.dsetPath             = h5pp::util::safe_str(dsetPath);
        info.h5_file              = file;
        info.h5_type              = h5pp::util::getH5Type<DataType>(options.h5_type);
        info.h5_layout            = h5pp::util::decideLayout(data, options.dataDims, options.dataDimsMax, options.h5_layout);
        info.h5_plist_dset_create = H5Pcreate(H5P_DATASET_CREATE);
        info.h5_plist_dset_access = H5Pcreate(H5P_DATASET_ACCESS);
        info.dsetDims             = h5pp::util::getDimensions(data, options.dataDims); // Infer the dimensions from given data, or interpret differently if dims given in options
        info.dsetDimsMax          = h5pp::util::getDimensionsMax(info.dsetDims.value(), info.h5_layout.value(), options.dataDimsMax);
        info.dsetSize             = h5pp::util::getSize(data, options.dataDims);
        info.dsetByte             = h5pp::util::getBytesTotal(data, options.dataDims);
        info.dsetRank             = h5pp::util::getRank<DataType>(options.dataDims);
        info.chunkDims            = h5pp::util::getDefaultChunkDimensions(info.dsetSize.value(), info.dsetDims.value(), options.chunkDims);
        info.compression          = h5pp::hdf5::getValidCompressionLevel(options.compression);
        info.h5_space             = h5pp::util::getDataSpace(info.dsetSize.value(), info.dsetDims.value(), info.h5_layout.value(), options.dataDimsMax);
        h5pp::hdf5::setStringSize(data, info.h5_type.value(), options.dataDims);
        h5pp::hdf5::setProperty_layout(info); // Must go before setting chunk dims
        h5pp::hdf5::setProperty_chunkDims(info);
        h5pp::hdf5::setProperty_compression(info);
        h5pp::hdf5::setSpaceExtent(info);
        return info;
    }

    /*!
     * \fn getDsetInfo
     * Returns information about a dataset in a info-struct
     * @param file a h5t file identifier
     * @param dsetPath the full path to a dataset in an HDF5 file
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     * @return
     */
    inline h5pp::DsetInfo getDsetInfo(const hid::h5f &file, std::string_view dsetPath, const Options &options = Options(), const PropertyLists &plists = PropertyLists()) {
        h5pp::DsetInfo info;
        fillDsetInfo(info, file, dsetPath, options, plists);
        return info;
    }

    template<typename DataType>
    inline void fillDataInfo(const DataType &data, DataInfo &info, const Options &options = Options()) {
        h5pp::logger::log->debug("Reading metadata of datatype [{}]", h5pp::type::sfinae::type_name<DataType>());
        // The point of passing options is to reinterpret the shape of the data and not to resize!
        // The data container should already be resized before entering this function.

        if(not info.dataDims) info.dataDims = h5pp::util::getDimensions(data, options.dataDims);
        if(not info.dataSize) info.dataSize = h5pp::util::getSize(data, options.dataDims);
        if(not info.dataRank) info.dataRank = h5pp::util::getRank<DataType>(options.dataDims);
        if(not info.dataByte) info.dataByte = h5pp::util::getBytesTotal(data, options.dataDims);
        if(not info.cpp_type) info.cpp_type = h5pp::type::sfinae::type_name<DataType>();
        if(not info.h5_space) info.h5_space = h5pp::util::getMemSpace(info.dataSize.value(), info.dataDims.value());
        if(options.mmrySlabs) h5pp::hdf5::selectHyperSlabs(info.h5_space.value(), options.mmrySlabs.value(), options.mmrySlabSelectOps);
    }

    template<typename DataType>
    inline h5pp::DataInfo getDataInfo(const DataType &data, const Options &options = Options()) {
        h5pp::DataInfo dataInfo;
        // As long as the two selections have the same number of elements, the data can be transferred
        fillDataInfo(data, dataInfo, options);
        return dataInfo;
    }

    inline void fillAttrInfo(AttrInfo &info, const hid::h5f &file, std::string_view attrName, std::string_view linkPath, const PropertyLists &plists = PropertyLists()) {
        h5pp::logger::log->debug("Reading metadata of attribute [{}] in link [{}]", attrName, linkPath);
        if(not info.linkPath) info.linkPath = linkPath;
        if(not info.linkExists) info.linkExists = h5pp::hdf5::checkIfLinkExists(file, info.linkPath.value(), std::nullopt, plists.link_access);
        // If the dataset does not exist, there isn't much else to do so we return;
        if(info.linkExists and not info.linkExists.value()) return;
        // From here on the link exists
        if(not info.h5_link) info.h5_link = h5pp::hdf5::openLink<hid::h5o>(file, linkPath, info.linkExists, plists.link_access);
        if(not info.attrName) info.attrName = attrName;
        if(not info.attrExists)
            info.attrExists = h5pp::hdf5::checkIfAttributeExists(file, info.linkPath.value(), info.attrName.value(), info.linkExists.value(), std::nullopt, plists.link_access);
        if(info.attrExists and not info.attrExists.value()) return;
        // From here on the attribute exists
        if(not info.h5_attr) info.h5_attr = H5Aopen_name(info.h5_link.value(), h5pp::util::safe_str(info.attrName.value()).c_str());
        if(not info.h5_type) info.h5_type = H5Aget_type(info.h5_attr.value());
        if(not info.h5_space) info.h5_space = H5Aget_space(info.h5_attr.value());
        if(not info.attrByte) info.attrByte = h5pp::hdf5::getBytesTotal(info.h5_attr.value());
        if(not info.attrSize) info.attrSize = h5pp::hdf5::getSize(info.h5_attr.value());
        if(not info.attrDims) info.attrDims = h5pp::hdf5::getDimensions(info.h5_attr.value());
        if(not info.attrRank) info.attrRank = h5pp::hdf5::getRank(info.h5_attr.value());
        if(not info.h5_plist_attr_create) info.h5_plist_attr_create = H5Aget_create_plist(info.h5_attr.value());
        //        if(not info.h5_plist_attr_access) info.h5_plist_attr_access  = H5Aget_create_plist(info.h5_attr.value());
    }

    template<typename DataType>
    inline h5pp::AttrInfo newAttrInfo(const hid::h5f &     file,
                                      const DataType &     data,
                                      std::string_view     attrName,
                                      std::string_view     linkPath,
                                      const Options &      options = Options(),
                                      const PropertyLists &plists  = PropertyLists()) {
        h5pp::logger::log->debug("Creating dataset metadata for type [{}]", h5pp::type::sfinae::type_name<DataType>());
        h5pp::AttrInfo info;
        info.attrName   = attrName;
        info.linkPath   = linkPath;
        info.linkExists = h5pp::hdf5::checkIfLinkExists(file, info.linkPath.value(), std::nullopt, plists.link_access);
        if(info.linkExists and info.linkExists.value()) info.h5_link = h5pp::hdf5::openLink<hid::h5o>(file, linkPath, info.linkExists, plists.link_access);
        info.attrExists = h5pp::hdf5::checkIfAttributeExists(file, info.linkPath.value(), info.attrName.value(), info.linkExists.value(), std::nullopt, plists.link_access);
        info.h5_type    = h5pp::util::getH5Type<DataType>(options.h5_type);
        info.h5_plist_attr_create = H5Pcreate(H5P_ATTRIBUTE_CREATE);
#if H5_VERSION_GE(1, 10, 0)
        info.h5_plist_attr_access = H5Pcreate(H5P_ATTRIBUTE_ACCESS);
#else
        info.h5_plist_attr_access = H5Pcreate(H5P_ATTRIBUTE_CREATE); // Missing access property in HDF5 1.8.x
#endif
        info.attrDims = h5pp::util::getDimensions(data, options.dataDims);
        info.attrSize = h5pp::util::getSize(data, options.dataDims);
        info.attrByte = h5pp::util::getBytesTotal(data);
        info.attrRank = h5pp::util::getRank<DataType>(options.dataDims);
        info.h5_space = h5pp::util::getDataSpace(info.attrSize.value(), info.attrDims.value(), H5D_COMPACT);
        h5pp::hdf5::setStringSize(data, info.h5_type.value(), options.dataDims);
        //        h5pp::hdf5::setStringSize(info.h5_type.value(), h5pp::util::getBytesTotal(data));
        //        h5pp::hdf5::setStringSize(info.h5_type.value(), H5T_VARIABLE);
        return info;
    }

    inline h5pp::AttrInfo getAttrInfo(const hid::h5f &file, std::string_view attrName, std::string_view linkPath, const PropertyLists &plists = PropertyLists()) {
        AttrInfo attrInfo;
        fillAttrInfo(attrInfo, file, attrName, linkPath, plists);
        return attrInfo;
    }

    template<typename h5x, typename = h5pp::type::sfinae::is_h5_loc<h5x>>
    inline TableInfo getTableInfo(const h5x &loc, std::string_view tableName, std::optional<bool> tableExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        TableInfo info;
        // Copy the name and group name
        info.tableName = util::safe_str(tableName);
        info.tableGroupName = "";
        size_t pos          = info.tableName.value().find_last_of('/');
        if(pos != std::string::npos) info.tableGroupName.value().assign(info.tableName.value().begin(), info.tableName.value().begin() + static_cast<long>(pos));

        // Get the location
        H5I_type_t type = H5Iget_type(loc);
        if(type == H5I_type_t::H5I_GROUP or type == H5I_type_t::H5I_FILE)
            info.tableLocId = loc;
        else
            throw std::runtime_error("Given object type for location is not a group or a file");
        if constexpr(std::is_same_v<h5x, hid::h5f>) info.tableFile = loc;
        if constexpr(std::is_same_v<h5x, hid::h5g>) info.tableGroup = loc;

        info.tableExists = h5pp::hdf5::checkIfLinkExists(loc, tableName, tableExists, plists.link_access);
        if(not info.tableExists.value()) return info;

        info.tableDset = hdf5::openLink<hid::h5d>(loc, tableName, tableExists, plists.link_access);
        info.tableType = H5Dget_type(info.tableDset.value());
        // Alloate temporaries
        hsize_t n_fields, n_records;
        H5TBget_table_info(loc, util::safe_str(tableName).c_str(), &n_fields, &n_records);
        std::vector<size_t> field_sizes(n_fields);
        std::vector<size_t> field_offsets(n_fields);
        size_t              record_bytes;
        char **             field_names = new char *[n_fields];
        for(size_t i = 0; i < n_fields; i++) field_names[i] = new char[255];
        H5TBget_field_info(loc, util::safe_str(tableName).c_str(), field_names, field_sizes.data(), field_offsets.data(), &record_bytes);

        // Copy results
        std::vector<std::string> field_names_vec(n_fields);
        std::vector<hid::h5t>    field_types(n_fields);
        for(size_t i = 0; i < n_fields; i++) field_names_vec[i] = field_names[i];
        for(size_t i = 0; i < n_fields; i++) field_types[i] = H5Tget_member_type(info.tableType.value(), static_cast<unsigned>(i));
        info.numFields    = n_fields;
        info.numRecords   = n_records;
        info.recordBytes  = record_bytes;
        info.fieldSizes   = field_sizes;
        info.fieldOffsets = field_offsets;
        info.fieldTypes   = field_types;
        info.fieldNames   = field_names_vec;

        /* release array of char arrays */
        for(size_t i = 0; i < n_fields; i++) delete field_names[i];
        delete[] field_names;

        return info;
    }

    inline h5pp::TableInfo newTableInfo(const hid::h5t &                  tableType,
                                        std::string_view                  tableName,
                                        std::string_view                  tableTitle,
                                        const std::optional<hsize_t>      desiredChunkSize        = std::nullopt,
                                        const std::optional<unsigned int> desiredCompressionLevel = std::nullopt) {
        TableInfo info;
        info.tableType      = tableType;
        info.tableTitle     = tableTitle;
        info.tableName      = tableName;
        info.tableGroupName = "";
        size_t pos                = info.tableName.value().find_last_of('/');
        if(pos != std::string::npos) info.tableGroupName.value().assign(info.tableName.value().begin(), info.tableName.value().begin() + (long) pos);

        info.numFields        = H5Tget_nmembers(tableType);
        info.numRecords       = 0;
        info.chunkSize        = desiredChunkSize.has_value() ? desiredChunkSize : 10;
        info.recordBytes      = H5Tget_size(info.tableType.value());
        info.compressionLevel = h5pp::hdf5::getValidCompressionLevel(desiredCompressionLevel);

        info.fieldTypes   = std::vector<h5pp::hid::h5t>();
        info.fieldOffsets = std::vector<size_t>();
        info.fieldSizes   = std::vector<size_t>();
        info.fieldNames   = std::vector<std::string>();

        for(unsigned int idx = 0; idx < (unsigned int) info.numFields.value(); idx++) {
            info.fieldTypes.value().emplace_back(H5Tget_member_type(info.tableType.value(), idx));
            info.fieldOffsets.value().emplace_back(H5Tget_member_offset(info.tableType.value(), idx));
            info.fieldSizes.value().emplace_back(H5Tget_size(info.fieldTypes.value().back()));
            const char *name = H5Tget_member_name(info.tableType.value(), idx);
            info.fieldNames.value().emplace_back(name);
            H5free_memory((void *) name);
        }
        return info;
    }
    //
    //    template<typename DataType>
    //    inline h5pp::TableInfo getMetaTable_write(const hid::h5f &file, const DataType &data, std::string_view tableName, const PropertyLists &plists = PropertyLists()) {
    //        bool exists = h5pp::hdf5::checkIfLinkExists(file,tableName,plists.link_access);
    //        if(exists)
    //            return getTableInfo(file,tableName,exists,plists);
    //        else {
    //            hid::h5d dataset      = h5pp::hdf5::openLink<hid::h5d>(file, tableName, std::nullopt, plists.link_access);
    //            hid::h5t entryType    = H5Dget_type(dataset);
    //            auto     tableProps   = newTableInfo(entryType, tableName, "", std::nullopt, std::nullopt);
    //            tableProps.numRecords = h5pp::util::getSize(data);
    //            return newTableInfo(entryType, tableName, "", std::nullopt, std::nullopt);
    //        }
    //    }

    //    inline h5pp::TableInfo getTableProperties_read(const hid::h5f &file, std::string_view tableName, const PropertyLists &plists = PropertyLists()) {
    //        hid::h5d dataset    = h5pp::hdf5::openLink<hid::h5d>(file, tableName, std::nullopt, plists.link_access);
    //        hid::h5t entryType  = H5Dget_type(dataset);
    //        auto     tableProps = newTableInfo(entryType, tableName, "", std::nullopt, std::nullopt);
    //        hsize_t  NFIELDS, NRECORDS;
    //        herr_t   err = H5TBget_table_info(file, h5pp::util::safe_str(tableName).c_str(), &NFIELDS, &NRECORDS);
    //        if(err < 0) {
    //            H5Eprint(H5E_DEFAULT, stderr);
    //            throw std::runtime_error("Failed to get table information");
    //        }
    //        tableProps.numFields  = NFIELDS;
    //        tableProps.numRecords = NRECORDS;
    //        return tableProps;
    //    }

}
