#include "h5ppConstants.h"
#include "h5ppHdf5.h"
#include "h5ppInfo.h"
#include "h5ppTypeSfinae.h"
#include "h5ppUtils.h"

namespace h5pp::scan {

    /*! \fn readDsetInfo
     * Fills missing information about a dataset in an info-struct
     *
     * @param info A struct with information about a dataset
     * @param file a h5f file identifier
     * @param dsetPath the full path to a dataset in an HDF5 file
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     */
    template<typename h5x>
    inline void readDsetInfo(h5pp::DsetInfo &info, const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5_loc_v<h5x>,
                      "Template function [h5pp::scan::readDsetInfo(..., const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");

        if(not options.linkPath and not info.dsetPath) throw std::runtime_error("Could not fill dataset info: No dataset path was given");
        if(not info.dsetSlab) info.dsetSlab = options.dsetSlab;
        if(not info.dsetPath) info.dsetPath = h5pp::util::safe_str(options.linkPath.value());
        h5pp::logger::log->debug("Scanning metadata of dataset [{}]", info.dsetPath.value());
        /* clang-format off */

        // Copy the location
        if(not info.h5File){
            if constexpr(std::is_same_v<h5x, hid::h5f>) info.h5File = loc;
            else info.h5File = H5Iget_file_id(loc);
        }

        if(not info.dsetExists) info.dsetExists = h5pp::hdf5::checkIfLinkExists(info.getLocId(), info.dsetPath.value(), std::nullopt, plists.linkAccess);

        // If the dataset does not exist, there isn't much else to do so we return;
        if(not info.dsetExists.value()) return;
        // From here on the dataset exists
        if(not info.h5Dset)  info.h5Dset  = h5pp::hdf5::openLink<hid::h5d>(loc, info.dsetPath.value(), info.dsetExists, plists.linkAccess);
        if(not info.h5Type)  info.h5Type  = H5Dget_type(info.h5Dset.value());
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
        if(not info.h5Layout)          info.h5Layout          = H5Pget_layout(info.h5PlistDsetCreate.value());
        if(not info.dsetChunk)         info.dsetChunk         = h5pp::hdf5::getChunkDimensions(info.h5PlistDsetCreate.value());
        if(not info.dsetDimsMax)       info.dsetDimsMax       = h5pp::hdf5::getMaxDimensions(info.h5Space.value(), info.h5Layout.value());

        if(not info.resizeMode) info.resizeMode = options.resizeMode;
        if(not info.resizeMode) {
            if(info.h5Layout != H5D_CHUNKED) info.resizeMode = h5pp::ResizeMode::DO_NOT_RESIZE;
            else info.resizeMode = h5pp::ResizeMode::RESIZE_TO_FIT;
        }
        if(not info.compression) {
            hid::h5p plist   = H5Dget_create_plist(info.h5Dset.value());
            info.compression = h5pp::hdf5::getCompressionLevel(plist);
        }
        /* clang-format on */

        // Get c++ properties
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
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
    template<typename h5x>
    inline h5pp::DsetInfo readDsetInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        if(not options.linkPath) throw std::runtime_error("Could not read dataset info: No dataset path was given in options");
        h5pp::DsetInfo info;
        readDsetInfo(info, loc, options, plists);
        return info;
    }

    /*! \fn getDsetInfo
     * Infers information for a new dataset based and passed options only
     * @param loc A valid HDF5 location (group or file)
     * @param dsetPath The path to the dataset relative to loc
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     */
    template<typename h5x>
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

        info.h5PlistDsetCreate = H5Pcreate(H5P_DATASET_CREATE);
        info.h5PlistDsetAccess = H5Pcreate(H5P_DATASET_ACCESS);
        info.h5Space = h5pp::util::getDsetSpace(info.dsetSize.value(), info.dsetDims.value(), info.h5Layout.value(), info.dsetDimsMax);
        /* clang-format on */
        h5pp::hdf5::setProperty_layout(info);    // Must go before setting chunk dims
        h5pp::hdf5::setProperty_chunkDims(info); // Will nullify chunkdims if not H5D_CHUNKED
        h5pp::hdf5::setProperty_compression(info);
        h5pp::hdf5::setSpaceExtent(info);

        // Get c++ properties
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
            std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Created metadata {}", info.string());
        auto error_msg = h5pp::debug::reportCompatibility(info.h5Layout, info.dsetDims, info.dsetChunk, info.dsetDimsMax);
        if(not error_msg.empty()) throw std::runtime_error(h5pp::format("Created dataset metadata is not well defined: \n{}", error_msg));
        return info;
    }


    /*! \brief Populates an AttrInfo object.
     *  If the attribute exists properties are read from file.
     *  Otherwise properties are inferred from the given data
     * @param loc A valid HDF5 location (group or file)
     * @param data The data from which to infer properties
     * @param dsetPath The path to the dataset relative to loc
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     */
    template<typename DataType, typename h5x>
    inline h5pp::DsetInfo inferDsetInfo(const h5x &          loc,
                                      const DataType &     data,
                                      const Options &      options = Options(),
                                      const PropertyLists &plists  = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5_loc_v<h5x>,
                      "Template function [h5pp::scan::inferDsetInfo(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");
        static_assert(not h5pp::type::sfinae::is_h5_loc_v<DataType>,
                      "Template function [h5pp::scan::inferDsetInfo(...,const DataType & data, ...)] requires type DataType to be: "
                      "none of [h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");
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
        info.h5Space           = h5pp::util::getDsetSpace(info.dsetSize.value(), info.dsetDims.value(), info.h5Layout.value(), info.dsetDimsMax);
        info.h5PlistDsetCreate = H5Pcreate(H5P_DATASET_CREATE);
        info.h5PlistDsetAccess = H5Pcreate(H5P_DATASET_ACCESS);
        h5pp::hdf5::setProperty_layout(info);    // Must go before setting chunk dims
        h5pp::hdf5::setProperty_chunkDims(info); // Will nullify chunkdims if not H5D_CHUNKED
        h5pp::hdf5::setProperty_compression(info);
        h5pp::hdf5::setSpaceExtent(info);
        /* clang-format on */

        // Get c++ properties
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
            std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Created metadata {}", info.string());
        auto error_msg = h5pp::debug::reportCompatibility(info.h5Layout, info.dsetDims, info.dsetChunk, info.dsetDimsMax);
        if(not error_msg.empty()) throw std::runtime_error(h5pp::format("Created dataset metadata is not well defined: \n{}", error_msg));
        return info;
    }





    template<typename DataType>
    inline void fillDataInfo(DataInfo &info, const DataType &data, const Options &options = Options()) {
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
                    h5pp::format("Error deducing data info: Dimensions must be specified for pointer data of type [{}]",
                                 h5pp::type::sfinae::type_name<DataType>()));

        // Let the dataDims inform the rest of the inference process
        if(not info.dataDims) info.dataDims = h5pp::util::getDimensions(data); // Will fail if no dataDims passed on a pointer
        if(not info.dataSize) info.dataSize = h5pp::util::getSizeFromDimensions(info.dataDims.value());
        if(not info.dataRank) info.dataRank = h5pp::util::getRankFromDimensions(info.dataDims.value());
        if(not info.dataByte) info.dataByte = info.dataSize.value() * h5pp::util::getBytesPerElem<DataType>();
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
            std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType<DataType>();
        h5pp::util::setStringSize<DataType>(data,
                                            info.dataSize.value(),
                                            info.dataByte.value(),
                                            info.dataDims.value()); // String size will be H5T_VARIABLE unless explicitly specified
        if(not info.h5Space) info.h5Space = h5pp::util::getMemSpace(info.dataSize.value(), info.dataDims.value());
        h5pp::logger::log->trace("Scanned metadata {}", info.string());
    }

    template<typename DataType>
    inline h5pp::DataInfo getDataInfo(const DataType &data, const Options &options = Options()) {
        h5pp::DataInfo dataInfo;
        // As long as the two selections have the same number of elements, the data can be transferred
        fillDataInfo(dataInfo, data, options);
        return dataInfo;
    }

    /*! \brief Populates an AttrInfo object with properties read from file */
    template<typename h5x>
    inline void readAttrInfo(AttrInfo &info, const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5_loc_v<h5x>,
                      "Template function [h5pp::scan::readAttrInfo(..., const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");

        /* clang-format off */
        options.assertWellDefined();
        if(not options.linkPath and not info.linkPath) throw std::runtime_error("Could not fill attribute info: No link path was given");
        if(not options.attrName and not info.attrName) throw std::runtime_error("Could not fill attribute info: No attribute name was given");
        if(not info.linkPath)    info.linkPath      = h5pp::util::safe_str(options.linkPath.value());
        if(not info.attrName)    info.attrName      = h5pp::util::safe_str(options.attrName.value());
        if(not info.attrSlab)    info.attrSlab      = options.attrSlab;
        h5pp::logger::log->debug("Scanning metadata of attribute [{}] in link [{}]", info.attrName.value(), info.linkPath.value());

        // Copy the location
        if(not info.h5File){
            if constexpr(std::is_same_v<h5x, hid::h5f>) info.h5File = loc;
            else info.h5File = H5Iget_file_id(loc);
        }

        /* It's important to note the convention used here:
         *      * linkPath is relative to loc.
         *      * loc can be a file or group, but NOT a dataset.
         *      * h5Link is the object on which the attribute is attached.
         *      * h5Link is an h5o object which means that it can be a file, group or dataset.
         *      * loc != h5Link.
         *
         */

        if(not info.linkExists)  info.linkExists = h5pp::hdf5::checkIfLinkExists(info.getLocId(), info.linkPath.value(), std::nullopt, plists.linkAccess);

        // If the link does not exist, there isn't much else to do so we return;
        if(info.linkExists and not info.linkExists.value()) return;

        // From here on the link exists
        if(not info.h5Link)     info.h5Link       = h5pp::hdf5::openLink<hid::h5o>(loc, info.linkPath.value(), info.linkExists, plists.linkAccess);
        if(not info.attrExists)
            info.attrExists = h5pp::hdf5::checkIfAttributeExists(info.h5Link.value(), info.attrName.value(), std::nullopt, plists.linkAccess);
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
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
            std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Scanned metadata {}", info.string());
    }

    /*! \brief Creates and returns a populated AttrInfo object with properties read from file */
    template<typename h5x>
    inline h5pp::AttrInfo readAttrInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        h5pp::AttrInfo info;
        readAttrInfo(info, loc, options, plists);
        return info;
    }

    /*! \brief Populates an AttrInfo object.
     *  If the attribute exists properties are read from file.
     *  Otherwise properties are inferred from the given data */
    template<typename DataType, typename h5x>
    inline void inferAttrInfo(AttrInfo &           info,
                              const h5x &          loc,
                              const DataType &     data,
                              const Options &      options,
                              const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5_loc_v<h5x>,
                      "Template function [h5pp::scan::readAttrInfo(..., const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");
        static_assert(not h5pp::type::sfinae::is_h5_loc_v<DataType>,
                      "Template function [h5pp::scan::readAttrInfo(...,..., const DataType & data, ...)] requires type DataType to be: "
                      "none of [h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");

        options.assertWellDefined();
        readAttrInfo(info, loc, options, plists);

        if(not info.linkExists or not info.linkExists.value()) {
            h5pp::logger::log->debug("Attribute metadata is being created for a non existing link: [{}]", options.linkPath.value());
            //            throw std::runtime_error(
            //                h5pp::format("Could not get attribute info for link [{}]: Link does not exist.", options.linkPath.value()));
        }

        if(info.attrExists and info.attrExists.value()) return; // attrInfo got populated already

        h5pp::logger::log->debug("Creating new attribute info for [{}] at link [{}]", options.attrName.value(), options.linkPath.value());

        /* clang-format off */
        // First copy the parameters given in options
        if(not info.h5Type) info.h5Type     = options.h5Type;
        if(not info.attrDims) info.attrDims = options.dataDims;
        if(not info.attrSlab) info.attrSlab = options.attrSlab;

        // Some sanity checks
        if constexpr(std::is_pointer_v<DataType>) {
            if(not info.attrDims)
                throw std::runtime_error(h5pp::format("Error creating attribute [{}] on link [{}]: Dimensions for new attribute must be "
                                                      "specified for pointer data of type [{}]",
                                                      options.attrName.value(),
                                                      options.linkPath.value(),
                                                      h5pp::type::sfinae::type_name<DataType>()));
        }

        // Next infer the missing properties
        if(not info.h5Type)   info.h5Type   = h5pp::util::getH5Type<DataType>();
        if(not info.attrDims) info.attrDims = h5pp::util::getDimensions(data);
        if(not info.attrSize) info.attrSize = h5pp::util::getSizeFromDimensions(info.attrDims.value());
        if(not info.attrRank) info.attrRank = h5pp::util::getRankFromDimensions(info.attrDims.value());
        if(not info.attrByte) info.attrByte = h5pp::util::getBytesTotal(data, info.attrSize);
        h5pp::hdf5::setStringSize<DataType>(data,
                                            info.h5Type.value(),
                                            info.attrSize.value(),
                                            info.attrByte.value(),
                                            info.attrDims.value()); // String size will be H5T_VARIABLE unless explicitly specified
        if(not info.h5Space) info.h5Space = h5pp::util::getDsetSpace(info.attrSize.value(), info.attrDims.value(), H5D_COMPACT);
        /* clang-format on */

        info.h5PlistAttrCreate = H5Pcreate(H5P_ATTRIBUTE_CREATE);
#if H5_VERSION_GE(1, 10, 0)
        info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_ACCESS);
#else
        info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_CREATE); // Missing access property in HDF5 1.8.x
#endif
        // Get c++ properties
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
            std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Created  metadata  {}", info.string());
    }

    /*! \brief Creates and returns a populated AttrInfo object.
     * If the attribute exists properties are read from file.
     * Otherwise properties are inferred from given data. */
    template<typename DataType, typename h5x>
    inline h5pp::AttrInfo
        inferAttrInfo(const h5x &loc, const DataType &data, const Options &options, const PropertyLists &plists = PropertyLists()) {
        h5pp::AttrInfo info;
        inferAttrInfo(info, loc, data, options, plists);
        return info;
    }

    /*! \brief Creates and returns a populated AttrInfo object.
     * If the attribute exists properties are read from file.
     * Otherwise properties are inferred from given data. */
    //    template<typename h5x>
    //    inline h5pp::AttrInfo getAttrInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
    //        auto info = readAttrInfo(loc, options, plists);
    //        if(info.attrExists.value()) return info;
    //        h5pp::logger::log->debug("Creating new attribute info for [{}] at link [{}]", options.attrName.value(),
    //        options.linkPath.value());
    //
    //        // First copy the parameters given in options
    //        if(not info.attrDims) info.attrDims = options.dataDims;
    //        if(not info.attrSlab) info.attrSlab = options.attrSlab;
    //        if(not info.h5Type) info.h5Type = options.h5Type;
    //
    //        // Some sanity checks
    //        if(not info.attrDims)
    //            throw std::runtime_error(h5pp::format("Error creating info for attribute [{}] in link [{}]: "
    //                                                  "Dimensions for new attribute must be specified when no data is given",
    //                                                  info.attrName.value(),
    //                                                  info.linkPath.value()));
    //        if(not info.h5Type)
    //            throw std::runtime_error(h5pp::format("Error creating info for attribute [{}] in link [{}]: "
    //                                                  "The HDF5 type for a new dataset must be specified when no data is given",
    //                                                  info.attrName.value(),
    //                                                  info.linkPath.value()));
    //
    //        // Next we infer the missing properties
    //        if(not info.attrSize) info.attrSize = h5pp::util::getSizeFromDimensions(info.attrDims.value());
    //        if(not info.attrRank) info.attrRank = h5pp::util::getRankFromDimensions(info.attrDims.value());
    //        if(not info.attrByte) info.attrByte = info.attrSize.value() * h5pp::hdf5::getBytesPerElem(info.h5Type.value());
    //        if(not info.h5Space) info.h5Space = h5pp::util::getDsetSpace(info.attrSize.value(), info.attrDims.value(), H5D_COMPACT);
    //
    //        info.h5PlistAttrCreate = H5Pcreate(H5P_ATTRIBUTE_CREATE);
    //#if H5_VERSION_GE(1, 10, 0)
    //        info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_ACCESS);
    //#else
    //        info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_CREATE); // Missing access property in HDF5 1.8.x
    //#endif
    //        // Get c++ properties
    //        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
    //            std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());
    //
    //        h5pp::logger::log->trace("Created  metadata  {}", info.string());
    //        return info;
    //    }

    template<typename h5x>
    inline void fillTableInfo(TableInfo &info, const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5_loc_v<h5x>,
                      "Template function [h5pp::scan::fillTableInfo(..., const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");
        if(not options.linkPath and not info.tablePath)
            throw std::runtime_error("Could not fill table info: No table path was given in options");
        if(not info.tablePath) info.tablePath = h5pp::util::safe_str(options.linkPath.value());
        h5pp::logger::log->debug("Scanning metadata of table [{}]", info.tablePath.value());

        // Copy the location
        if(not info.h5File) {
            if constexpr(std::is_same_v<h5x, hid::h5f>)
                info.h5File = loc;
            else
                info.h5File = H5Iget_file_id(loc);
        }

        if(not info.tableExists)
            info.tableExists = h5pp::hdf5::checkIfLinkExists(info.getLocId(), info.tablePath.value(), std::nullopt, plists.linkAccess);

        // Infer the group name
        if(not info.tableGroupName) {
            info.tableGroupName = "";
            size_t pos          = info.tablePath.value().find_last_of('/');
            if(pos != std::string::npos)
                info.tableGroupName.value().assign(info.tablePath.value().begin(), info.tablePath.value().begin() + static_cast<long>(pos));
        }
        // This is as far as we get if the table does not exist
        if(not info.tableExists.value()) return;
        if(not info.h5Dset)
            info.h5Dset = hdf5::openLink<hid::h5d>(info.getLocId(), info.tablePath.value(), info.tableExists, plists.linkAccess);
        if(not info.h5Type) info.h5Type = H5Dget_type(info.h5Dset.value());
        if(not info.numRecords) {
            // We could use H5TBget_table_info here but internally that would create a temporary
            // dataset id and type id, but we already have them so we can use these directly instead
            auto dims = h5pp::hdf5::getDimensions(info.h5Dset.value());
            if(dims.size() != 1) throw std::logic_error("Tables can only have rank 1");
            info.numRecords = dims[0];
        }
        if(not info.numFields) info.numFields = static_cast<size_t>(H5Tget_nmembers(info.h5Type.value()));
        if(not info.tableTitle) {
            char table_title[255];
            H5TBAget_title(info.h5Dset.value(), table_title);
            info.tableTitle = table_title;
        }

        if(not info.fieldTypes) {
            hsize_t               n_fields = info.numFields.value();
            std::vector<hid::h5t> field_types(n_fields);
            for(size_t i = 0; i < n_fields; i++) field_types[i] = H5Tget_member_type(info.h5Type.value(), static_cast<unsigned>(i));
            info.fieldTypes = field_types;
        }

        if(not info.fieldSizes or not info.fieldOffsets or not info.recordBytes or not info.fieldNames) {
            hsize_t                  n_fields = info.numFields.value();
            std::vector<size_t>      field_sizes(n_fields);
            std::vector<size_t>      field_offsets(n_fields);
            std::vector<std::string> field_names_vec(n_fields);
            size_t                   record_bytes;
            char **                  field_names = new char *[n_fields];
            for(size_t i = 0; i < n_fields; i++) field_names[i] = new char[255];

            // Read the data
            H5TBget_field_info(loc, info.tablePath->c_str(), field_names, field_sizes.data(), field_offsets.data(), &record_bytes);
            for(size_t i = 0; i < n_fields; i++) field_names_vec[i] = field_names[i];

            // release array of char arrays
            for(size_t i = 0; i < n_fields; i++) delete[] field_names[i];
            delete[] field_names;

            // Copy the data
            info.recordBytes  = record_bytes;
            info.fieldSizes   = field_sizes;
            info.fieldOffsets = field_offsets;
            info.fieldNames   = field_names_vec;
        }

        if(not info.chunkSize) {
            hid::h5p plist    = H5Dget_create_plist(info.h5Dset.value());
            auto     chunkVec = h5pp::hdf5::getChunkDimensions(plist);
            if(chunkVec and not chunkVec->empty()) info.chunkSize = chunkVec.value()[0];
        }

        if(not info.compressionLevel) {
            hid::h5p plist        = H5Dget_create_plist(info.h5Dset.value());
            info.compressionLevel = h5pp::hdf5::getCompressionLevel(plist);
        }

        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize) {
            // Get c++ type information
            info.cppTypeIndex = std::vector<std::type_index>();
            info.cppTypeName  = std::vector<std::string>();
            info.cppTypeSize  = std::vector<size_t>();
            for(size_t i = 0; i < info.numFields.value(); i++) {
                auto cppInfo = h5pp::hdf5::getCppType(info.fieldTypes.value()[i]);
                info.cppTypeIndex->emplace_back(std::get<0>(cppInfo));
                info.cppTypeName->emplace_back(std::get<1>(cppInfo));
                info.cppTypeSize->emplace_back(std::get<2>(cppInfo));
            }
        }
    }

    template<typename h5x>
    inline TableInfo readTableInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        TableInfo info;
        fillTableInfo(info, loc, options, plists);
        return info;
    }

    template<typename h5x>
    inline h5pp::TableInfo
        getTableInfo(const h5x &loc, const Options &options, std::string_view tableTitle, const PropertyLists &plists = PropertyLists()) {
        auto info = readTableInfo(loc, options, plists);
        if(info.tableExists.value()) return info;
        h5pp::logger::log->debug("Creating metadata for new table [{}]", options.linkPath.value());
        info.tableTitle       = tableTitle;
        info.h5Type           = options.h5Type;
        info.numFields        = H5Tget_nmembers(info.h5Type.value());
        info.numRecords       = 0;
        info.recordBytes      = H5Tget_size(info.h5Type.value());
        info.compressionLevel = options.compression;
        if(options.dsetDimsChunk and not options.dsetDimsChunk->empty()) info.chunkSize = options.dsetDimsChunk.value()[0];

        if(not info.chunkSize)
            info.chunkSize =
                h5pp::util::getChunkDimensions(info.recordBytes.value(), {1}, std::nullopt, H5D_layout_t::H5D_CHUNKED).value()[0];
        if(not info.compressionLevel) info.compressionLevel = h5pp::hdf5::getValidCompressionLevel();

        info.fieldTypes   = std::vector<h5pp::hid::h5t>();
        info.fieldOffsets = std::vector<size_t>();
        info.fieldSizes   = std::vector<size_t>();
        info.fieldNames   = std::vector<std::string>();

        for(unsigned int idx = 0; idx < static_cast<unsigned int>(info.numFields.value()); idx++) {
            info.fieldTypes.value().emplace_back(H5Tget_member_type(info.h5Type.value(), idx));
            info.fieldOffsets.value().emplace_back(H5Tget_member_offset(info.h5Type.value(), idx));
            info.fieldSizes.value().emplace_back(H5Tget_size(info.fieldTypes.value().back()));
            const char *name = H5Tget_member_name(info.h5Type.value(), idx);
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
