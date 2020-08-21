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
    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline void fillDsetInfo(h5pp::DsetInfo &info, const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        if(not options.linkPath) throw std::runtime_error("Could not fill dataset info: No dataset path was given in options");
        h5pp::logger::log->debug("Scanning metadata of dataset [{}]", options.linkPath.value());
        // Copy the location
        if constexpr(std::is_same_v<h5x, hid::h5f>) info.h5File = loc;
        if constexpr(std::is_same_v<h5x, hid::h5g>) info.h5Group = loc;
        if constexpr(std::is_same_v<h5x, hid::h5o>) {
            H5I_type_t type = H5Iget_type(loc);
            if(type == H5I_type_t::H5I_GROUP or type == H5I_type_t::H5I_FILE)
                info.h5ObjLoc = loc;
            else
                throw std::runtime_error("Given object type for location is not a group or a file");
        }
        if(not info.dsetPath) info.dsetPath = h5pp::util::safe_str(options.linkPath.value());
        if(not info.dsetExists) info.dsetExists = h5pp::hdf5::checkIfDatasetExists(info.getLocId(), info.dsetPath.value(), std::nullopt, plists.linkAccess);
        if(not info.dsetSlab) info.dsetSlab = options.dsetSlab;
        // If the dataset does not exist, there isn't much else to do so we return;
        if(info.dsetExists and not info.dsetExists.value()) return;
        // From here on the dataset exists
        if(not info.h5Dset) info.h5Dset = h5pp::hdf5::openLink<hid::h5d>(loc, options.linkPath.value(), info.dsetExists, plists.linkAccess);
        if(not info.h5Type) info.h5Type = H5Dget_type(info.h5Dset.value());
        if(not info.h5Space) info.h5Space = H5Dget_space(info.h5Dset.value());

        // Get the properties of the selected space
        if(not info.dsetByte) info.dsetByte = h5pp::hdf5::getBytesTotal(info.h5Dset.value(), info.h5Space, info.h5Type);
        if(not info.dsetSize) info.dsetSize = h5pp::hdf5::getSize(info.h5Space.value());
        if(not info.dsetRank) info.dsetRank = h5pp::hdf5::getRank(info.h5Space.value());
        if(not info.dsetDims) info.dsetDims = h5pp::hdf5::getDimensions(info.h5Space.value());

        // We read the layout from file. Note that it is not possible to change the layout on existing datasets! Read more here
        // https://support.hdfgroup.org/HDF5/Tutor/layout.html
        if(not info.h5PlistDsetCreate) info.h5PlistDsetCreate = H5Dget_create_plist(info.h5Dset.value());
        if(not info.h5PlistDsetAccess) info.h5PlistDsetAccess = H5Dget_access_plist(info.h5Dset.value());
        if(not info.h5Layout) info.h5Layout = H5Pget_layout(info.h5PlistDsetCreate.value());
        if(not info.dsetChunk) info.dsetChunk = h5pp::hdf5::getChunkDimensions(info.h5PlistDsetCreate.value());
        if(not info.dsetDimsMax) info.dsetDimsMax = h5pp::hdf5::getMaxDimensions(info.h5Space.value(), info.h5Layout.value());

        if(not info.resizeMode) info.resizeMode = options.resizeMode;
        if(not info.resizeMode) {
            if(info.h5Layout != H5D_CHUNKED)
                info.resizeMode = h5pp::ResizeMode::DO_NOT_RESIZE;
            else
                info.resizeMode = h5pp::ResizeMode::RESIZE_TO_FIT;
        }

        // Get c++ properties
        std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Scanned metadata {}", info.string());
        auto error_msg = h5pp::debug::reportCompatibility(info.h5Layout, info.dsetDims, info.dsetChunk, info.dsetDimsMax);
        if(not error_msg.empty()) throw std::runtime_error(h5pp::format("Scanned dataset metadata is not well defined: \n{}", error_msg));
    }

    /*! \fn readDsetInfo
     * Infers information for a new dataset based and passed options only
     * @param loc A valid HDF5 location (group or file)
     * @param dsetPath The path to the dataset relative to loc
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     */
    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline h5pp::DsetInfo readDsetInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        if(not options.linkPath) throw std::runtime_error("Could not read dataset info: No dataset path was given in options");
        h5pp::DsetInfo info;
        fillDsetInfo(info, loc, options, plists);
        return info;
    }

    /*! \fn getDsetInfo
     * Infers information for a new dataset based and passed options only
     * @param loc A valid HDF5 location (group or file)
     * @param dsetPath The path to the dataset relative to loc
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     */
    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline h5pp::DsetInfo getDsetInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        auto info = readDsetInfo(loc, options, plists);
        if(info.dsetExists.value()) return info;
        h5pp::logger::log->debug("Creating metadata for new dataset [{}]", options.linkPath.value());
        // First copy the parameters given in options
        info.dsetDims    = options.dataDims;
        info.dsetDimsMax = options.dsetDimsMax;
        info.dsetChunk   = options.dsetDimsChunk;
        info.dsetSlab    = options.dsetSlab;
        info.h5Type      = options.h5Type;
        info.h5Layout    = options.h5Layout;
        info.compression = options.compression;
        info.resizeMode  = options.resizeMode;

        // Some sanity checks
        if(not info.dsetDims)
            throw std::runtime_error(h5pp::format("Error creating metadata for new dataset [{}]: "
                                                  "Dimensions for new dataset must be specified when no data is given",
                                                  info.dsetPath.value()));
        if(not info.h5Type)
            throw std::runtime_error(h5pp::format("Error creating metadata for new dataset [{}]: "
                                                  "The HDF5 type for a new dataset must be specified when no data is given",
                                                  info.dsetPath.value()));

        if(info.dsetChunk) {
            // If dsetDimsChunk has been given then the layout is supposed to be chunked
            if(not info.h5Layout) info.h5Layout = H5D_CHUNKED;

            // Check that chunking options are sane
            if(info.dsetDims and info.dsetDims->size() != info.dsetChunk->size())
                throw std::runtime_error(h5pp::format("Error creating metadata for new dataset [{}]: "
                                                      "Dataset and chunk dimensions must be the same size: "
                                                      "dset dims {} | chunk dims {}",
                                                      info.dsetPath.value(),
                                                      info.dsetDims.value(),
                                                      info.dsetChunk.value()));

            if(info.h5Layout != H5D_CHUNKED)
                throw std::runtime_error(h5pp::format("Error creating metadata for new dataset [{}]: "
                                                      "Dataset chunk dimensions {} requires H5D_CHUNKED layout",
                                                      info.dsetPath.value(),
                                                      info.dsetChunk.value()));
        }

        // If dsetDimsMax has been given and any of them is H5S_UNLIMITED then the layout is supposed to be chunked
        if(info.dsetDimsMax) {
            // If dsetDimsMax has been given then the layout is supposed to be chunked
            if(not info.h5Layout) info.h5Layout = H5D_CHUNKED;
        }

        // Next infer the missing properties
        /* clang-format off */
        if(not info.dsetSize)    info.dsetSize      = h5pp::util::getSizeFromDimensions(info.dsetDims.value());
        if(not info.dsetRank)    info.dsetRank      = h5pp::util::getRankFromDimensions(info.dsetDims.value());
        if(not info.dsetByte)    info.dsetByte      = info.dsetSize.value() * h5pp::hdf5::getBytesPerElem(info.h5Type.value()); // Trick needed for strings.
        if(not info.h5Layout)    info.h5Layout      = h5pp::util::decideLayout(info.dsetByte.value());
        if(not info.dsetDimsMax) info.dsetDimsMax   = h5pp::util::decideDimensionsMax(info.dsetDims.value(), info.h5Layout.value());
        if(not info.dsetChunk)   info.dsetChunk     = h5pp::util::getChunkDimensions(h5pp::hdf5::getBytesPerElem(info.h5Type.value()), info.dsetDims.value(),info.dsetDimsMax,info.h5Layout);
        if(not info.compression) info.compression   = h5pp::hdf5::getValidCompressionLevel(info.compression);
        if(not info.resizeMode) {
            if(info.h5Layout != H5D_CHUNKED)
                info.resizeMode = h5pp::ResizeMode::DO_NOT_RESIZE;
            else
                info.resizeMode = h5pp::ResizeMode::RESIZE_TO_FIT;
        }
        /* clang-format on */

        info.h5PlistDsetCreate = H5Pcreate(H5P_DATASET_CREATE);
        info.h5PlistDsetAccess = H5Pcreate(H5P_DATASET_ACCESS);
        info.h5Space           = h5pp::util::getDsetSpace(info.dsetSize.value(), info.dsetDims.value(), info.h5Layout.value(), info.dsetDimsMax);
        h5pp::hdf5::setProperty_layout(info);    // Must go before setting chunk dims
        h5pp::hdf5::setProperty_chunkDims(info); // Will nullify chunkdims if not H5D_CHUNKED
        h5pp::hdf5::setProperty_compression(info);
        h5pp::hdf5::setSpaceExtent(info);

        // Get c++ properties
        std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Created metadata {}", info.string());
        auto error_msg = h5pp::debug::reportCompatibility(info.h5Layout, info.dsetDims, info.dsetChunk, info.dsetDimsMax);
        if(not error_msg.empty()) throw std::runtime_error(h5pp::format("Created dataset metadata is not well defined: \n{}", error_msg));
        return info;
    }

    /*! \fn getDsetInfo
     * Infers information for a new dataset based on given data and passed options
     * @param loc A valid HDF5 location (group or file)
     * @param data The data from which to infer properties
     * @param dsetPath The path to the dataset relative to loc
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     */
    template<typename DataType, typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline h5pp::DsetInfo getDsetInfo(const h5x &loc, const DataType &data, const Options &options = Options(), const PropertyLists &plists = PropertyLists()) {
        auto info = readDsetInfo(loc, options, plists);
        if(info.dsetExists.value()) return info;
        h5pp::logger::log->debug("Creating metadata for new dataset [{}]", options.linkPath.value());

        // First copy the parameters given in options
        info.dsetDims    = options.dataDims;
        info.dsetDimsMax = options.dsetDimsMax;
        info.dsetChunk   = options.dsetDimsChunk;
        info.dsetSlab    = options.dsetSlab;
        info.h5Type      = options.h5Type;
        info.h5Layout    = options.h5Layout;
        info.resizeMode  = options.resizeMode;
        info.compression = options.compression;
        if constexpr(std::is_pointer_v<DataType>) {
            if(not info.dsetDims)
                throw std::runtime_error(h5pp::format("Error creating metadata for new dataset [{}]: "
                                                      "Dimensions for new dataset must be specified for pointer data of type [{}]",
                                                      info.dsetPath.value(),
                                                      h5pp::type::sfinae::type_name<DataType>()));
        }

        if(info.dsetChunk) {
            // If dsetDimsChunk has been given then the layout is supposed to be chunked
            if(not info.h5Layout) info.h5Layout = H5D_CHUNKED;

            // Check that chunking options are sane
            if(info.dsetDims and info.dsetDims->size() != info.dsetChunk->size())
                throw std::runtime_error(h5pp::format("Error creating metadata for new dataset [{}]: "
                                                      "Dataset and chunk dimensions must be the same size: "
                                                      "dset dims {} | chunk dims {}",
                                                      info.dsetPath.value(),
                                                      info.dsetDims.value(),
                                                      info.dsetChunk.value()));

            if(info.h5Layout != H5D_CHUNKED)
                throw std::runtime_error(h5pp::format("Error creating metadata for new dataset [{}]: "
                                                      "Dataset chunk dimensions {} requires H5D_CHUNKED layout",
                                                      info.dsetPath.value(),
                                                      info.dsetChunk.value()));
        }

        // If dsetDimsMax has been given and any of them is H5S_UNLIMITED then the layout is supposed to be chunked
        if(info.dsetDimsMax) {
            // If dsetDimsMax has been given then the layout is supposed to be chunked
            if(not info.h5Layout) info.h5Layout = H5D_CHUNKED;
            if(info.h5Layout != H5D_CHUNKED)
                throw std::runtime_error(h5pp::format("Error creating metadata for new dataset [{}]: "
                                                      "Dataset max dimensions {} requires H5D_CHUNKED layout",
                                                      info.dsetPath.value(),
                                                      info.dsetDimsMax.value()));
        }

        // Next infer the missing properties
        /* clang-format off */
        if(not info.dsetDims)    info.dsetDims      = h5pp::util::getDimensions(data);
        if(not info.h5Type)      info.h5Type        = h5pp::util::getH5Type<DataType>();
        if(not info.dsetSize)    info.dsetSize      = h5pp::util::getSizeFromDimensions(info.dsetDims.value());
        if(not info.dsetRank)    info.dsetRank      = h5pp::util::getRankFromDimensions(info.dsetDims.value());
        if(not info.dsetByte)    info.dsetByte      = h5pp::util::getBytesTotal(data,info.dsetSize);
        if(not info.h5Layout)    info.h5Layout      = h5pp::util::decideLayout(data,info.dsetDims, info.dsetDimsMax);
        if(not info.dsetDimsMax) info.dsetDimsMax   = h5pp::util::decideDimensionsMax(info.dsetDims.value(), info.h5Layout);
        if(not info.dsetChunk)   info.dsetChunk     = h5pp::util::getChunkDimensions(h5pp::util::getBytesPerElem<DataType>(), info.dsetDims.value(),info.dsetDimsMax, info.h5Layout);
        if(not info.compression) info.compression   = h5pp::hdf5::getValidCompressionLevel(info.compression);
        if(not info.resizeMode) {
            if(info.h5Layout != H5D_CHUNKED)
                info.resizeMode = h5pp::ResizeMode::DO_NOT_RESIZE;
            else
                info.resizeMode = h5pp::ResizeMode::RESIZE_TO_FIT;
        }

        h5pp::hdf5::setStringSize<DataType>(data, info.h5Type.value(), info.dsetSize.value(), info.dsetByte.value(), info.dsetDims.value());       // String size will be H5T_VARIABLE unless explicitly specified
        /* clang-format on */
        info.h5Space           = h5pp::util::getDsetSpace(info.dsetSize.value(), info.dsetDims.value(), info.h5Layout.value(), info.dsetDimsMax);
        info.h5PlistDsetCreate = H5Pcreate(H5P_DATASET_CREATE);
        info.h5PlistDsetAccess = H5Pcreate(H5P_DATASET_ACCESS);
        h5pp::hdf5::setProperty_layout(info);    // Must go before setting chunk dims
        h5pp::hdf5::setProperty_chunkDims(info); // Will nullify chunkdims if not H5D_CHUNKED
        h5pp::hdf5::setProperty_compression(info);
        h5pp::hdf5::setSpaceExtent(info);

        // Get c++ properties
        std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Created metadata {}", info.string());
        auto error_msg = h5pp::debug::reportCompatibility(info.h5Layout, info.dsetDims, info.dsetChunk, info.dsetDimsMax);
        if(not error_msg.empty()) throw std::runtime_error(h5pp::format("Created dataset metadata is not well defined: \n{}", error_msg));
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

    template<typename DataType>
    inline void fillDataInfo(const DataType &data, DataInfo &info, const Options &options = Options()) {
        h5pp::logger::log->debug("Scanning metadata of datatype [{}]", h5pp::type::sfinae::type_name<DataType>());
        // The point of passing options is to reinterpret the shape of the data and not to resize!
        // The data container should already be resized before entering this function.

        // First copy the relevant options
        if(not info.dataDims) info.dataDims = options.dataDims;
        if(not info.dataSlab) info.dataSlab = options.dataSlab;

        // Then set the missing information
        if constexpr(std::is_pointer_v<DataType>)
            if(not info.dataDims)
                throw std::runtime_error(
                    h5pp::format("Error deducing data info: Dimensions must be specified for pointer data of type [{}]", h5pp::type::sfinae::type_name<DataType>()));

        // Let the dataDims inform the rest of the inference process
        if(not info.dataDims) info.dataDims = h5pp::util::getDimensions(data); // Will fail if no dataDims passed on a pointer
        if(not info.dataSize) info.dataSize = h5pp::util::getSizeFromDimensions(info.dataDims.value());
        if(not info.dataRank) info.dataRank = h5pp::util::getRankFromDimensions(info.dataDims.value());
        if(not info.dataByte) info.dataByte = info.dataSize.value() * h5pp::util::getBytesPerElem<DataType>();
        std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType<DataType>();
        h5pp::util::setStringSize<DataType>(
            data, info.dataSize.value(), info.dataByte.value(), info.dataDims.value()); // String size will be H5T_VARIABLE unless explicitly specified
        if(not info.h5Space) info.h5Space = h5pp::util::getMemSpace(info.dataSize.value(), info.dataDims.value());
        h5pp::logger::log->trace("Scanned metadata {}", info.string());
    }

    template<typename DataType>
    inline h5pp::DataInfo getDataInfo(const DataType &data, const Options &options = Options()) {
        h5pp::DataInfo dataInfo;
        // As long as the two selections have the same number of elements, the data can be transferred
        fillDataInfo(data, dataInfo, options);
        return dataInfo;
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline void fillAttrInfo(AttrInfo &info, const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        /* clang-format off */
        if(not options.linkPath) throw std::runtime_error("Could not fill attribute info: No link path was given in options");
        if(not options.attrName) throw std::runtime_error("Could not fill attribute info: No attribute name was given in options");
        if(not info.linkPath)    info.linkPath      = h5pp::util::safe_str(options.linkPath.value());
        if(not info.attrName)    info.attrName      = h5pp::util::safe_str(options.attrName.value());
        if(not info.attrSlab)    info.attrSlab      = options.attrSlab;
        h5pp::logger::log->debug("Scanning metadata of attribute [{}] in link [{}]", info.attrName.value(), info.linkPath.value());
        if(not info.linkExists)  info.linkExists = h5pp::hdf5::checkIfLinkExists(loc, info.linkPath.value(), std::nullopt, plists.linkAccess);
        // If the dataset does not exist, there isn't much else to do so we return;
        if(info.linkExists and not info.linkExists.value()) return;
        // From here on the link exists
        if(not info.h5Link)     info.h5Link       = h5pp::hdf5::openLink<hid::h5o>(loc, info.linkPath.value(), info.linkExists, plists.linkAccess);
        if(not info.attrExists)
            info.attrExists = h5pp::hdf5::checkIfAttributeExists(info.h5Link.value(), info.linkPath.value(), info.attrName.value(), std::nullopt, plists.linkAccess);
        if(info.attrExists and not info.attrExists.value()) return;
        // From here on the attribute exists
        if(not info.h5Attr)    info.h5Attr        = H5Aopen_name(info.h5Link.value(), h5pp::util::safe_str(info.attrName.value()).c_str());
        if(not info.h5Type)    info.h5Type        = H5Aget_type(info.h5Attr.value());
        if(not info.h5Space)   info.h5Space = H5Aget_space(info.h5Attr.value());
        // Get the properties of the selected space
        if(not info.attrByte)   info.attrByte       = h5pp::hdf5::getBytesTotal(info.h5Attr.value(), info.h5Space, info.h5Type);
        if(not info.attrSize)   info.attrSize       = h5pp::hdf5::getSize(info.h5Space.value());
        if(not info.attrDims)   info.attrDims       = h5pp::hdf5::getDimensions(info.h5Space.value());
        if(not info.attrRank)   info.attrRank       = h5pp::hdf5::getRank(info.h5Space.value());
        if(not info.h5PlistAttrCreate) info.h5PlistAttrCreate = H5Aget_create_plist(info.h5Attr.value());
            /* clang-format on */
#if H5_VERSION_GE(1, 10, 0)
        if(not info.h5PlistAttrAccess) info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_ACCESS);
#else
        if(not info.h5PlistAttrAccess) info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_CREATE); // Missing access property in HDF5 1.8.x
#endif
        // Get c++ properties
        std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Scanned metadata {}", info.string());
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline h5pp::AttrInfo readAttrInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        h5pp::AttrInfo info;
        fillAttrInfo(info, loc, options, plists);
        return info;
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline h5pp::AttrInfo getAttrInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        auto info = readAttrInfo(loc, options, plists);
        if(info.attrExists.value()) return info;
        h5pp::logger::log->debug("Creating new attribute info for [{}] at link [{}]", options.attrName.value(), options.linkPath.value());

        // First copy the parameters given in options
        if(not info.attrDims) info.attrDims = options.dataDims;
        if(not info.attrSlab) info.attrSlab = options.attrSlab;
        if(not info.h5Type) info.h5Type = options.h5Type;

        // Some sanity checks
        if(not info.attrDims)
            throw std::runtime_error(h5pp::format("Error creating info for attribute [{}] in link [{}]: "
                                                  "Dimensions for new attribute must be specified when no data is given",
                                                  info.attrName.value(),
                                                  info.linkPath.value()));
        if(not info.h5Type)
            throw std::runtime_error(h5pp::format("Error creating info for attribute [{}] in link [{}]: "
                                                  "The HDF5 type for a new dataset must be specified when no data is given",
                                                  info.attrName.value(),
                                                  info.linkPath.value()));

        // Next we infer the missing properties
        if(not info.attrSize) info.attrSize = h5pp::util::getSizeFromDimensions(info.attrDims.value());
        if(not info.attrRank) info.attrRank = h5pp::util::getRankFromDimensions(info.attrDims.value());
        if(not info.attrByte) info.attrByte = info.attrSize.value() * h5pp::hdf5::getBytesPerElem(info.h5Type.value());
        if(not info.h5Space) info.h5Space = h5pp::util::getDsetSpace(info.attrSize.value(), info.attrDims.value(), H5D_COMPACT);

        info.h5PlistAttrCreate = H5Pcreate(H5P_ATTRIBUTE_CREATE);
#if H5_VERSION_GE(1, 10, 0)
        info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_ACCESS);
#else
        info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_CREATE); // Missing access property in HDF5 1.8.x
#endif
        // Get c++ properties
        std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Created  metadata  {}", info.string());
        return info;
    }

    template<typename DataType, typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline h5pp::AttrInfo getAttrInfo(const h5x &loc, const DataType &data, const Options &options, const PropertyLists &plists = PropertyLists()) {
        auto info = readAttrInfo(loc, options, plists);
        if(not info.linkExists) throw std::runtime_error(h5pp::format("Could not get attribute info for link [{}]: Link does not exist.", options.linkPath.value()));
        if(not info.linkExists.value()) throw std::runtime_error(h5pp::format("Could not get attribute info for link [{}]: Link does not exist.", options.linkPath.value()));
        if(info.attrExists and info.attrExists.value()) return info;
        h5pp::logger::log->debug("Creating new attribute info for [{}] at link [{}]", options.attrName.value(), options.linkPath.value());

        // First copy the parameters given in options
        if(not info.attrDims) info.attrDims = options.dataDims;
        if(not info.attrSlab) info.attrSlab = options.attrSlab;
        if(not info.h5Type) info.h5Type = options.h5Type;
        // Some sanity checks
        if constexpr(std::is_pointer_v<DataType>) {
            if(not info.attrDims)
                throw std::runtime_error(h5pp::format("Error creating attribute [{}] on link [{}]: Dimensions for new attribute must be specified for pointer data of type [{}]",
                                                      options.attrName.value(),
                                                      options.linkPath.value(),
                                                      h5pp::type::sfinae::type_name<DataType>()));
        }

        // Next infer the missing properties
        /* clang-format off */
        if(not info.attrDims)    info.attrDims      = h5pp::util::getDimensions(data);
        if(not info.h5Type)      info.h5Type        = h5pp::util::getH5Type<DataType>();
        if(not info.attrSize)    info.attrSize      = h5pp::util::getSizeFromDimensions(info.attrDims.value());
        if(not info.attrRank)    info.attrRank      = h5pp::util::getRankFromDimensions(info.attrDims.value());
        if(not info.attrByte)    info.attrByte      = h5pp::util::getBytesTotal(data,info.attrSize);
        h5pp::hdf5::setStringSize<DataType>(data,info.h5Type.value(), info.attrSize.value(), info.attrByte.value(), info.attrDims.value());       // String size will be H5T_VARIABLE unless explicitly specified
        if(not info.h5Space) info.h5Space = h5pp::util::getDsetSpace(info.attrSize.value(), info.attrDims.value(), H5D_COMPACT);
        /* clang-format on */

        info.h5PlistAttrCreate = H5Pcreate(H5P_ATTRIBUTE_CREATE);
#if H5_VERSION_GE(1, 10, 0)
        info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_ACCESS);
#else
        info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_CREATE); // Missing access property in HDF5 1.8.x
#endif
        // Get c++ properties
        std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Created  metadata  {}", info.string());
        return info;
    }

    template<typename h5x, typename = h5pp::type::sfinae::enable_if_is_h5_loc<h5x>>
    inline TableInfo getTableInfo(const h5x &loc, std::string_view tableName, std::optional<bool> tableExists = std::nullopt, const PropertyLists &plists = PropertyLists()) {
        h5pp::logger::log->debug("Scanning metadata of table [{}]", util::safe_str(tableName));

        TableInfo info;
        // Copy the name and group name
        info.tablePath      = util::safe_str(tableName);
        info.tableGroupName = "";
        size_t pos          = info.tablePath.value().find_last_of('/');
        if(pos != std::string::npos) info.tableGroupName.value().assign(info.tablePath.value().begin(), info.tablePath.value().begin() + static_cast<long>(pos));

        // Copy the location
        if constexpr(std::is_same_v<h5x, hid::h5f>) info.tableFile = loc;
        if constexpr(std::is_same_v<h5x, hid::h5g>) info.tableGroup = loc;
        if constexpr(std::is_same_v<h5x, hid::h5o>) {
            H5I_type_t type = H5Iget_type(loc);
            if(type == H5I_type_t::H5I_GROUP or type == H5I_type_t::H5I_FILE)
                info.tableObjLoc = loc;
            else
                throw std::runtime_error("Given location type is not a group or a file");
        }

        info.tableExists = h5pp::hdf5::checkIfLinkExists(loc, tableName, tableExists, plists.linkAccess);
        if(not info.tableExists.value()) return info;

        info.tableDset = hdf5::openLink<hid::h5d>(loc, tableName, info.tableExists, plists.linkAccess);
        info.tableType = H5Dget_type(info.tableDset.value());
        // Alloate temporaries
        hsize_t n_fields, n_records;
        H5TBget_table_info(loc, util::safe_str(tableName).c_str(), &n_fields, &n_records);
        std::vector<size_t> field_sizes(n_fields);
        std::vector<size_t> field_offsets(n_fields);
        size_t              record_bytes;
        char                table_title[255];
        char **             field_names = new char *[n_fields];
        for(size_t i = 0; i < n_fields; i++) field_names[i] = new char[255];
        H5TBget_field_info(loc, util::safe_str(tableName).c_str(), field_names, field_sizes.data(), field_offsets.data(), &record_bytes);
        H5TBAget_title(info.tableDset.value(), table_title);
        // Copy results
        std::vector<std::string> field_names_vec(n_fields);
        std::vector<hid::h5t>    field_types(n_fields);
        for(size_t i = 0; i < n_fields; i++) field_names_vec[i] = field_names[i];
        for(size_t i = 0; i < n_fields; i++) field_types[i] = H5Tget_member_type(info.tableType.value(), static_cast<unsigned>(i));

        info.tableTitle   = table_title;
        info.numFields    = n_fields;
        info.numRecords   = n_records;
        info.recordBytes  = record_bytes;
        info.fieldSizes   = field_sizes;
        info.fieldOffsets = field_offsets;
        info.fieldTypes   = field_types;
        info.fieldNames   = field_names_vec;
        hid::h5p plist    = H5Dget_create_plist(info.tableDset->value());
        auto     chunkVec = h5pp::hdf5::getChunkDimensions(plist);
        if(chunkVec and not chunkVec->empty()) info.chunkSize = chunkVec.value()[0];

        /* release array of char arrays */
        for(size_t i = 0; i < n_fields; i++) delete[] field_names[i];
        delete[] field_names;

        // Get c++ properties
        info.cppTypeIndex = std::vector<std::type_index>();
        info.cppTypeName  = std::vector<std::string>();
        info.cppTypeSize  = std::vector<size_t>();
        for(size_t i = 0; i < n_fields; i++) {
            auto cppInfo = h5pp::hdf5::getCppType(info.fieldTypes.value()[i]);
            info.cppTypeIndex->emplace_back(std::get<0>(cppInfo));
            info.cppTypeName->emplace_back(std::get<1>(cppInfo));
            info.cppTypeSize->emplace_back(std::get<2>(cppInfo));
        }

        return info;
    }

    inline h5pp::TableInfo newTableInfo(const hid::h5t &                  tableType,
                                        std::string_view                  tablePath,
                                        std::string_view                  tableTitle,
                                        const std::optional<hsize_t>      desiredChunkSize        = std::nullopt,
                                        const std::optional<unsigned int> desiredCompressionLevel = std::nullopt) {
        TableInfo info;
        info.tableType      = tableType;
        info.tableTitle     = tableTitle;
        info.tablePath      = tablePath;
        info.tableGroupName = "";
        size_t pos          = info.tablePath.value().find_last_of('/');
        if(pos != std::string::npos)
            info.tableGroupName.value().assign(info.tablePath.value().begin(), info.tablePath.value().begin() + static_cast<std::string::difference_type>(pos));

        info.numFields        = H5Tget_nmembers(tableType);
        info.numRecords       = 0;
        info.recordBytes      = H5Tget_size(info.tableType.value());
        info.chunkSize        = desiredChunkSize.has_value() ? desiredChunkSize.value()
                                                             : h5pp::util::getChunkDimensions(info.recordBytes.value(), {1}, std::nullopt, H5D_layout_t::H5D_CHUNKED).value()[0];
        info.compressionLevel = h5pp::hdf5::getValidCompressionLevel(desiredCompressionLevel);

        info.fieldTypes   = std::vector<h5pp::hid::h5t>();
        info.fieldOffsets = std::vector<size_t>();
        info.fieldSizes   = std::vector<size_t>();
        info.fieldNames   = std::vector<std::string>();

        for(unsigned int idx = 0; idx < static_cast<unsigned int>(info.numFields.value()); idx++) {
            info.fieldTypes.value().emplace_back(H5Tget_member_type(info.tableType.value(), idx));
            info.fieldOffsets.value().emplace_back(H5Tget_member_offset(info.tableType.value(), idx));
            info.fieldSizes.value().emplace_back(H5Tget_size(info.fieldTypes.value().back()));
            const char *name = H5Tget_member_name(info.tableType.value(), idx);
            info.fieldNames.value().emplace_back(name);
            H5free_memory((void *) name);
        }

        // Get c++ properties
        info.cppTypeIndex = std::vector<std::type_index>();
        info.cppTypeName  = std::vector<std::string>();
        info.cppTypeSize  = std::vector<size_t>();
        for(size_t idx = 0; idx < info.numFields.value(); idx++) {
            auto cppInfo = h5pp::hdf5::getCppType(info.fieldTypes.value()[idx]);
            info.cppTypeIndex->emplace_back(std::get<0>(cppInfo));
            info.cppTypeName->emplace_back(std::get<1>(cppInfo));
            info.cppTypeSize->emplace_back(std::get<2>(cppInfo));
        }

        return info;
    }
}
