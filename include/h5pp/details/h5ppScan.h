#include "h5ppConstants.h"
#include "h5ppHdf5.h"
#include "h5ppInfo.h"
#include "h5ppTypeSfinae.h"
#include "h5ppUtils.h"

namespace h5pp::scan {

    /*! \fn readDsetInfo
     * Populates a DsetInfo object by reading properties from a dataset on file
     *
     * @param info A struct with information about a dataset
     * @param file a h5f file identifier
     * @param dsetPath the full path to a dataset in an HDF5 file
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     */
    template<typename h5x>
    inline void readDsetInfo(h5pp::DsetInfo &info, const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5pp_loc_id<h5x>,
                      "Template function [h5pp::scan::readDsetInfo(..., const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");

        if(not options.linkPath and not info.dsetPath) throw h5pp::runtime_error("Could not read dataset info: No dataset path was given");
        // Start by copying fields in options which override later analysis
        if(not info.h5Type) info.h5Type = options.h5Type;
        if(not info.dsetSlab) info.dsetSlab = options.dsetSlab;
        if(not info.dsetPath) info.dsetPath = h5pp::util::safe_str(options.linkPath.value());
        h5pp::logger::log->debug("Scanning metadata of dataset [{}]", info.dsetPath.value());
        /* clang-format off */

        // Copy the location
        if(not info.h5File){
            if constexpr(std::is_same_v<h5x, hid::h5f>) info.h5File = loc;
            else info.h5File = H5Iget_file_id(loc);
        }

        if(not info.dsetExists) info.dsetExists = h5pp::hdf5::checkIfLinkExists(info.getLocId(), info.dsetPath.value(), plists.linkAccess);

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
        if(not info.h5DsetCreate) info.h5DsetCreate      = H5Dget_create_plist(info.h5Dset.value());
        if(not info.h5DsetAccess) info.h5DsetAccess      = H5Dget_access_plist(info.h5Dset.value());
        if(not info.h5Layout)     info.h5Layout          = H5Pget_layout(info.h5DsetCreate.value());
        if(not info.dsetChunk)    info.dsetChunk         = h5pp::hdf5::getChunkDimensions(info.h5DsetCreate.value());
        if(not info.dsetDimsMax)  info.dsetDimsMax       = h5pp::hdf5::getMaxDimensions(info.h5Space.value(), info.h5Layout.value());
        if(not info.h5Filters)    info.h5Filters         = h5pp::hdf5::getFilters(info.h5DsetCreate.value());
        if(not info.compression)  info.compression       = h5pp::hdf5::getDeflateLevel(info.h5DsetCreate.value());


        if(not info.resizePolicy) info.resizePolicy = options.resizePolicy;
        if(not info.resizePolicy) {
            if(info.h5Layout != H5D_CHUNKED) info.resizePolicy = h5pp::ResizePolicy::OFF;
            else if(info.dsetSlab) info.resizePolicy = h5pp::ResizePolicy::GROW; // A hyperslab selection on the dataset has been made. Let's not shrink!
            else info.resizePolicy = h5pp::ResizePolicy::FIT;
        }

        // Apply hyperslab selection if there is any
        if(info.dsetSlab) h5pp::hdf5::selectHyperslab(info.h5Space.value(), info.dsetSlab.value());
        /* clang-format on */

        // Get c++ properties
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
            std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Scanned metadata {}", info.string(h5pp::logger::logIf(LogLevel::trace)));
        auto error_msg = h5pp::debug::reportCompatibility(info.h5Layout, info.dsetDims, info.dsetChunk, info.dsetDimsMax);
        if(not error_msg.empty()) throw h5pp::runtime_error("Scanned dataset metadata is not well defined: \n{}", error_msg);
    }

    /*! \fn readDsetInfo
     * Infers information for a new dataset based and passed options only
     * @param loc A valid HDF5 location (group or file)
     * @param dsetPath The path to the dataset relative to loc
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     */
    template<typename h5x>
    [[nodiscard]] inline h5pp::DsetInfo
        readDsetInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        if(not options.linkPath) throw h5pp::runtime_error("Could not read dataset info: No dataset path was given in options");
        h5pp::DsetInfo info;
        readDsetInfo(info, loc, options, plists);
        return info;
    }

    /*! \brief Populates a DsetInfo object based entirely on passed options
     * @param loc A valid HDF5 location (group or file)
     * @param options A struct containing optional metadata
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     */
    template<typename h5x>
    inline h5pp::DsetInfo makeDsetInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5pp_loc_id<h5x>,
                      "Template function [h5pp::scan::makeDsetInfo(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");

        auto info = readDsetInfo(loc, options, plists);
        if(info.dsetExists.value()) return info;
        h5pp::logger::log->debug("Creating metadata for new dataset [{}]", options.linkPath.value());
        // First copy the parameters given in options
        info.dsetDims     = options.dataDims;
        info.dsetDimsMax  = options.dsetMaxDims;
        info.dsetChunk    = options.dsetChunkDims;
        info.dsetSlab     = options.dsetSlab;
        info.h5Type       = options.h5Type;
        info.h5Layout     = options.h5Layout;
        info.compression  = options.compression;
        info.resizePolicy = options.resizePolicy;

        // Some sanity checks
        if(not info.dsetDims)
            throw h5pp::runtime_error("Error creating metadata for new dataset [{}]: "
                                      "Dimensions for new dataset must be specified when no data is given",
                                      info.dsetPath.value());
        if(not info.h5Type)
            throw h5pp::runtime_error("Error creating metadata for new dataset [{}]: "
                                      "The HDF5 type for a new dataset must be specified when no data is given",
                                      info.dsetPath.value());

        if(info.dsetChunk) {
            // If dsetDimsChunk has been given then the layout is supposed to be chunked
            if(not info.h5Layout) info.h5Layout = H5D_CHUNKED;

            // Check that chunking options are sane
            if(info.dsetDims and info.dsetDims->size() != info.dsetChunk->size())
                throw h5pp::runtime_error("Error creating metadata for new dataset [{}]: "
                                          "Dataset and chunk dimensions must be the same size: "
                                          "dset dims {} | chunk dims {}",
                                          info.dsetPath.value(),
                                          info.dsetDims.value(),
                                          info.dsetChunk.value());

            if(info.h5Layout != H5D_CHUNKED)
                throw h5pp::runtime_error("Error creating metadata for new dataset [{}]: "
                                          "Dataset chunk dimensions {} requires H5D_CHUNKED layout",
                                          info.dsetPath.value(),
                                          info.dsetChunk.value());
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
        if(not info.resizePolicy) {
            if(info.h5Layout != H5D_CHUNKED)
                info.resizePolicy = h5pp::ResizePolicy::OFF;
            else
                info.resizePolicy = h5pp::ResizePolicy::FIT;
        }
        if(not info.h5Space) info.h5Space = h5pp::util::getDsetSpace(info.dsetSize.value(), info.dsetDims.value(), info.h5Layout.value(), info.dsetDimsMax);
        // Apply hyperslab selection if there is any
        if(info.dsetSlab) h5pp::hdf5::selectHyperslab(info.h5Space.value(), info.dsetSlab.value());
        if(not info.h5DsetCreate) info.h5DsetCreate = H5Pcreate(H5P_DATASET_CREATE);
        if(not info.h5DsetAccess) info.h5DsetAccess = H5Pcreate(H5P_DATASET_ACCESS);
        /* clang-format on */
        h5pp::hdf5::setProperty_layout(info);    // Must go before setting chunk dims
        h5pp::hdf5::setProperty_chunkDims(info); // Will nullify chunkdims if not H5D_CHUNKED
        h5pp::hdf5::setProperty_compression(info);
        h5pp::hdf5::setSpaceExtent(info);

        // Get c++ properties
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
            std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Created metadata {}", info.string(h5pp::logger::logIf(LogLevel::trace)));
        auto error_msg = h5pp::debug::reportCompatibility(info.h5Layout, info.dsetDims, info.dsetChunk, info.dsetDimsMax);
        if(not error_msg.empty()) throw h5pp::runtime_error("Created dataset metadata is not well defined: \n{}", error_msg);
        return info;
    }

    /*! \brief Populates a DsetInfo object.
     *  If the attribute exists properties are read from file.
     *  Otherwise properties are inferred from the given data
     * @param loc A valid HDF5 location (group or file)
     * @param data The data from which to infer properties
     * @param options A struct containing optional metadata
     * @param plists (optional) access property for the file. Used to determine link access property when searching for the dataset.
     */
    template<typename DataType, typename h5x>
    [[nodiscard]] inline h5pp::DsetInfo inferDsetInfo(const h5x           &loc,
                                                      const DataType      &data,
                                                      const Options       &options = Options(),
                                                      const PropertyLists &plists  = PropertyLists()) {
        static_assert(not type::sfinae::is_h5pp_id<DataType>);
        static_assert(h5pp::type::sfinae::is_h5pp_loc_id<h5x>,
                      "Template function [h5pp::scan::inferDsetInfo(const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");
        static_assert(not h5pp::type::sfinae::is_h5pp_id<DataType>,
                      "Template function [h5pp::scan::inferDsetInfo(...,const DataType & data, ...)] requires type DataType to be "
                      "none of the h5pp::hid::h5x identifiers");
        auto info = readDsetInfo(loc, options, plists);
        if(info.dsetExists.value()) return info;
        h5pp::logger::log->debug("Creating metadata for new dataset [{}]", options.linkPath.value());

        // First copy the parameters given in options
        /* clang-format off */
        if(not info.dsetDims    ) info.dsetDims     = options.dataDims;
        if(not info.dsetDimsMax ) info.dsetDimsMax  = options.dsetMaxDims;
        if(not info.dsetChunk   ) info.dsetChunk    = options.dsetChunkDims;
        if(not info.dsetSlab    ) info.dsetSlab     = options.dsetSlab;
        if(not info.h5Type      ) info.h5Type       = options.h5Type;
        if(not info.h5Layout    ) info.h5Layout     = options.h5Layout;
        if(not info.resizePolicy  ) info.resizePolicy   = options.resizePolicy;
        if(not info.compression ) info.compression  = options.compression;
        /* clang-format on */

        if constexpr(std::is_pointer_v<DataType>) {
            if(not info.dsetDims)
                throw h5pp::runtime_error("Error creating metadata for new dataset [{}]: "
                                          "Dimensions for new dataset must be specified for pointer data of type [{}]",
                                          info.dsetPath.value(),
                                          h5pp::type::sfinae::type_name<DataType>());
        }

        if(info.dsetChunk) {
            // If dsetDimsChunk has been given then the layout is supposed to be chunked
            if(not info.h5Layout) info.h5Layout = H5D_CHUNKED;

            // Check that chunking options are sane
            if(info.dsetDims and info.dsetDims->size() != info.dsetChunk->size())
                throw h5pp::runtime_error("Error creating metadata for new dataset [{}]: "
                                          "Dataset and chunk dimensions must be the same size: "
                                          "dset dims {} | chunk dims {}",
                                          info.dsetPath.value(),
                                          info.dsetDims.value(),
                                          info.dsetChunk.value());

            if(info.h5Layout != H5D_CHUNKED)
                throw h5pp::runtime_error("Error creating metadata for new dataset [{}]: "
                                          "Dataset chunk dimensions {} requires H5D_CHUNKED layout",
                                          info.dsetPath.value(),
                                          info.dsetChunk.value());
        }

        // If dsetDimsMax has been given and any of them is H5S_UNLIMITED then the layout is supposed to be chunked
        if(info.dsetDimsMax) {
            // If dsetDimsMax has been given then the layout is supposed to be chunked
            if(not info.h5Layout) info.h5Layout = H5D_CHUNKED;
            if(info.h5Layout != H5D_CHUNKED)
                throw h5pp::runtime_error("Error creating metadata for new dataset [{}]: "
                                          "Dataset max dimensions {} requires H5D_CHUNKED layout",
                                          info.dsetPath.value(),
                                          info.dsetDimsMax.value());
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
        if(not info.resizePolicy) {
            if(info.h5Layout != H5D_CHUNKED)
                info.resizePolicy = h5pp::ResizePolicy::OFF;
            else
                info.resizePolicy = h5pp::ResizePolicy::FIT;
        }

        h5pp::hdf5::setStringSize<DataType>(data, info.h5Type.value(), info.dsetSize.value(), info.dsetByte.value(), info.dsetDims.value());       // String size will be H5T_VARIABLE unless explicitly specified
        if(not info.h5Space)           info.h5Space           = h5pp::util::getDsetSpace(info.dsetSize.value(), info.dsetDims.value(), info.h5Layout.value(), info.dsetDimsMax);
        // Apply hyperslab selection if there is any
        if(info.dsetSlab) h5pp::hdf5::selectHyperslab(info.h5Space.value(), info.dsetSlab.value());

        if(not info.h5DsetCreate) info.h5DsetCreate = H5Pcreate(H5P_DATASET_CREATE);
        if(not info.h5DsetAccess) info.h5DsetAccess = H5Pcreate(H5P_DATASET_ACCESS);
        h5pp::hdf5::setProperty_layout(info);    // Must go before setting chunk dims
        h5pp::hdf5::setProperty_chunkDims(info); // Will nullify chunkdims if not H5D_CHUNKED
        h5pp::hdf5::setProperty_compression(info);
        h5pp::hdf5::setSpaceExtent(info);
        /* clang-format on */

        // Get c++ properties
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
            std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Created metadata {}", info.string(h5pp::logger::logIf(LogLevel::trace)));
        auto error_msg = h5pp::debug::reportCompatibility(info.h5Layout, info.dsetDims, info.dsetChunk, info.dsetDimsMax);
        if(not error_msg.empty()) throw h5pp::runtime_error("Created dataset metadata is not well defined: \n{}", error_msg);
        return info;
    }

    /*! \brief Populates a DataInfo object by scanning the given data type.*/
    template<typename DataType>
    inline void scanDataInfo(DataInfo &info, const DataType &data, const Options &options = Options()) {
        static_assert(not type::sfinae::is_h5pp_id<DataType>);
        h5pp::logger::log->debug("Scanning metadata of datatype [{}]", h5pp::type::sfinae::type_name<DataType>());
        // The point of passing options is to reinterpret the shape of the data and not to resize!
        // The data container should already be resized before entering this function.

        // First copy the relevant options
        if(not info.dataDims) info.dataDims = options.dataDims;
        if(not info.dataSlab) info.dataSlab = options.dataSlab;

        // Then set the missing information
        if constexpr(std::is_pointer_v<DataType>)
            if(not info.dataDims)
                throw h5pp::runtime_error(
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
        // Apply hyperslab selection if there is any
        if(info.dataSlab) h5pp::hdf5::selectHyperslab(info.h5Space.value(), info.dataSlab.value());
        h5pp::logger::log->trace("Scanned metadata {}", info.string(h5pp::logger::logIf(LogLevel::trace)));
    }

    /*! \brief Creates and returns a populated a DataInfo object by scanning the given data type.*/
    template<typename DataType>
    inline h5pp::DataInfo scanDataInfo(const DataType &data, const Options &options = Options()) {
        h5pp::DataInfo dataInfo;
        // As long as the two selections have the same number of elements, the data can be transferred
        scanDataInfo(dataInfo, data, options);
        return dataInfo;
    }

    /*! \brief Populates an AttrInfo object with properties read from file */
    template<typename h5x>
    inline void readAttrInfo(AttrInfo &info, const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5pp_loc_id<h5x>,
                      "Template function [h5pp::scan::readAttrInfo(..., const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");

        /* clang-format off */
        if(not options.linkPath and not info.linkPath) throw h5pp::runtime_error("Could not read attribute info: No link path was given");
        if(not options.attrName and not info.attrName) throw h5pp::runtime_error("Could not read attribute info: No attribute name was given");
        if(not info.linkPath)    info.linkPath      = h5pp::util::safe_str(options.linkPath.value());
        if(not info.h5Type)      info.h5Type        = options.h5Type;
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

        if(not info.linkExists)  info.linkExists = h5pp::hdf5::checkIfLinkExists(info.getLocId(), info.linkPath.value(), plists.linkAccess);

        // If the link does not exist, there isn't much else to do so we return;
        if(info.linkExists and not info.linkExists.value()) return;

        // From here on the link exists
        if(not info.h5Link)     info.h5Link       = h5pp::hdf5::openLink<hid::h5o>(loc, info.linkPath.value(), info.linkExists, plists.linkAccess);
        if(not info.attrExists)
            info.attrExists = h5pp::hdf5::checkIfAttrExists(info.h5Link.value(), info.attrName.value(), plists.linkAccess);
        if(info.attrExists and not info.attrExists.value()) return;

        // From here on the attribute exists
        if(not info.h5Attr)    info.h5Attr  = H5Aopen_name(info.h5Link.value(), h5pp::util::safe_str(info.attrName.value()).c_str());
        if(not info.h5Type)    info.h5Type  = H5Aget_type(info.h5Attr.value());
        if(not info.h5Space)   info.h5Space = H5Aget_space(info.h5Attr.value());

        // Get the properties of the selected space
        if(not info.attrByte)   info.attrByte       = h5pp::hdf5::getBytesTotal(info.h5Attr.value(), info.h5Space, info.h5Type);
        if(not info.attrSize)   info.attrSize       = h5pp::hdf5::getSize(info.h5Space.value());
        if(not info.attrDims)   info.attrDims       = h5pp::hdf5::getDimensions(info.h5Space.value());
        if(not info.attrRank)   info.attrRank       = h5pp::hdf5::getRank(info.h5Space.value());
        if(not info.h5PlistAttrCreate) info.h5PlistAttrCreate = H5Aget_create_plist(info.h5Attr.value());
        // Apply hyperslab selection if there is any
        if(info.attrSlab) h5pp::hdf5::selectHyperslab(info.h5Space.value(), info.attrSlab.value());
            /* clang-format on */
#if H5_VERSION_GE(1, 10, 0)
        if(not info.h5PlistAttrAccess) info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_ACCESS);
#else
        if(not info.h5PlistAttrAccess) info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_CREATE); // Missing access property in HDF5 1.8.x
#endif
        // Get c++ properties
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
            std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Scanned metadata {}", info.string(h5pp::logger::logIf(LogLevel::trace)));
    }

    /*! \brief Creates and returns a populated AttrInfo object with properties read from file */
    template<typename h5x>
    [[nodiscard]] inline h5pp::AttrInfo
        readAttrInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        h5pp::AttrInfo info;
        readAttrInfo(info, loc, options, plists);
        return info;
    }

    /*! \brief Populates an AttrInfo object.
     *  If the attribute exists properties are read from file.
     *  Otherwise properties are inferred from the given data */
    template<typename DataType, typename h5x>
    inline void inferAttrInfo(AttrInfo            &info,
                              const h5x           &loc,
                              const DataType      &data,
                              const Options       &options,
                              const PropertyLists &plists = PropertyLists()) {
        static_assert(not type::sfinae::is_h5pp_id<DataType>);
        static_assert(h5pp::type::sfinae::is_h5pp_loc_id<h5x>,
                      "Template function [h5pp::scan::readAttrInfo(..., const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");
        static_assert(not h5pp::type::sfinae::is_h5pp_loc_id<DataType>,
                      "Template function [h5pp::scan::readAttrInfo(...,..., const DataType & data, ...)] requires type DataType to be: "
                      "none of [h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");

        readAttrInfo(info, loc, options, plists);
        if(not info.linkExists or not info.linkExists.value()) {
            h5pp::logger::log->debug("Attribute metadata is being created for a non existing link: [{}]", options.linkPath.value());
            //            throw h5pp::runtime_error(
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
                throw h5pp::runtime_error("Error creating attribute [{}] on link [{}]: Dimensions for new attribute must be "
                                                      "specified for pointer data of type [{}]",
                                                      options.attrName.value(),
                                                      options.linkPath.value(),
                                                      h5pp::type::sfinae::type_name<DataType>());
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
        // Apply hyperslab selection if there is any
        if(info.attrSlab) h5pp::hdf5::selectHyperslab(info.h5Space.value(), info.attrSlab.value());
        /* clang-format on */

        if(not info.h5PlistAttrCreate) info.h5PlistAttrCreate = H5Pcreate(H5P_ATTRIBUTE_CREATE);
#if H5_VERSION_GE(1, 10, 0)
        if(not info.h5PlistAttrAccess) info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_ACCESS);
#else
        if(not info.h5PlistAttrAccess) info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_CREATE); // Missing access property in HDF5 1.8.x
#endif
        // Get c++ properties
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
            std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Created  metadata  {}", info.string(h5pp::logger::logIf(LogLevel::trace)));
    }

    /*! \brief Creates and returns a populated AttrInfo object.
     * If the attribute exists properties are read from file.
     * Otherwise properties are inferred from given data. */
    template<typename DataType, typename h5x>
    [[nodiscard]] inline h5pp::AttrInfo
        inferAttrInfo(const h5x &loc, const DataType &data, const Options &options, const PropertyLists &plists = PropertyLists()) {
        h5pp::AttrInfo info;
        inferAttrInfo(info, loc, data, options, plists);
        return info;
    }

    /*! \brief Populates an AttrInfo object based entirely on given options */
    template<typename h5x>
    inline void makeAttrInfo(AttrInfo &info, const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5pp_loc_id<h5x>,
                      "Template function [h5pp::scan::makeAttrInfo(..., const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");
        info = readAttrInfo(loc, options, plists);
        if(info.attrExists.value()) return;
        h5pp::logger::log->debug("Creating new attribute info for [{}] at link [{}]", options.attrName.value(), options.linkPath.value());

        // First copy the parameters given in options
        if(not info.h5Type) info.h5Type = options.h5Type;
        if(not info.attrDims) info.attrDims = options.dataDims;
        if(not info.attrSlab) info.attrSlab = options.attrSlab;
        if(not info.linkPath) info.linkPath = options.linkPath;

        // Some sanity checks
        if(not info.attrDims)
            throw h5pp::runtime_error("Error creating info for attribute [{}] in link [{}]: "
                                      "Dimensions for new attribute must be specified when no data is given",
                                      info.attrName.value(),
                                      info.linkPath.value());
        if(not info.h5Type)
            throw h5pp::runtime_error("Error creating info for attribute [{}] in link [{}]: "
                                      "The HDF5 type for a new dataset must be specified when no data is given",
                                      info.attrName.value(),
                                      info.linkPath.value());

        // Next we infer the missing properties
        if(not info.attrSize) info.attrSize = h5pp::util::getSizeFromDimensions(info.attrDims.value());
        if(not info.attrRank) info.attrRank = h5pp::util::getRankFromDimensions(info.attrDims.value());
        if(not info.attrByte) info.attrByte = info.attrSize.value() * h5pp::hdf5::getBytesPerElem(info.h5Type.value());
        if(not info.h5Space) info.h5Space = h5pp::util::getDsetSpace(info.attrSize.value(), info.attrDims.value(), H5D_COMPACT);
        // Apply hyperslab selection if there is any
        if(info.attrSlab) h5pp::hdf5::selectHyperslab(info.h5Space.value(), info.attrSlab.value());
        if(not info.h5PlistAttrCreate) info.h5PlistAttrCreate = H5Pcreate(H5P_ATTRIBUTE_CREATE);
#if H5_VERSION_GE(1, 10, 0)
        if(not info.h5PlistAttrAccess) info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_ACCESS);
#else
        if(not info.h5PlistAttrAccess) info.h5PlistAttrAccess = H5Pcreate(H5P_ATTRIBUTE_CREATE); // Missing access property in HDF5 1.8.x
#endif
        // Get c++ properties
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize)
            std::tie(info.cppTypeIndex, info.cppTypeName, info.cppTypeSize) = h5pp::hdf5::getCppType(info.h5Type.value());

        h5pp::logger::log->trace("Created  metadata  {}", info.string(h5pp::logger::logIf(LogLevel::trace)));
    }

    /*! \brief Creates and returns a populated AttrInfo object based entirely on given options */
    template<typename h5x>
    [[nodiscard]] inline h5pp::AttrInfo
        makeAttrInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        h5pp::AttrInfo info;
        inferAttrInfo(info, loc, options, plists);
        return info;
    }

    template<typename h5x>
    inline void inferAttrInfo(AttrInfo &info, const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5pp_loc_id<h5x>,
                      "Template function [h5pp::scan::inferAttrInfo(..., const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");
        if(not options.linkPath and not info.linkPath) throw h5pp::runtime_error("Could not infer attribute info: No link path was given");
        if(not options.attrName and not info.attrName)
            throw h5pp::runtime_error("Could not infer attribute info: No attribute name was given");
        if(not info.linkPath) info.linkPath = h5pp::util::safe_str(options.linkPath.value());
        if(not info.attrName) info.attrName = h5pp::util::safe_str(options.attrName.value());

        if(not info.linkExists) info.linkExists = h5pp::hdf5::checkIfLinkExists(loc, info.linkPath.value(), plists.linkAccess);
        if(info.linkExists.value())
            if(not info.h5Link)
                info.h5Link = h5pp::hdf5::openLink<hid::h5o>(loc, info.linkPath.value(), info.linkExists, plists.linkAccess);
        if(not info.attrExists) {
            if(info.h5Link)
                info.attrExists = h5pp::hdf5::checkIfAttrExists(info.h5Link.value(), info.attrName.value(), plists.linkAccess);
            else
                info.attrExists =
                    h5pp::hdf5::checkIfAttrExists(loc, info.linkPath.value(), info.attrName.value(), info.linkExists, plists.linkAccess);
        }

        if(info.attrExists.value())
            readAttrInfo(info, loc, options, plists); // Table exists so we can read properties from file
        else
            makeAttrInfo(info, loc, options, plists);
    }

    /*! \brief Populates a TableInfo object with properties read from file */
    template<typename h5x>
    inline void readTableInfo(TableInfo &info, const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_hdf5_loc_id<h5x>,
                      "Template function [h5pp::scan::readTableInfo(..., const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g], [h5pp::hid::h5o] or [hid_t]");
        if(not options.linkPath and not info.tablePath) throw h5pp::runtime_error("Could not read table info: No table path was given");
        // Copy fields from options to override later analysis
        if(not info.tablePath) info.tablePath = h5pp::util::safe_str(options.linkPath.value());
        if(not info.h5Type) info.h5Type = options.h5Type;

        h5pp::logger::log->debug("Scanning metadata of table [{}]", info.tablePath.value());

        // Copy the location
        if(not info.h5File) {
            if constexpr(std::is_same_v<h5x, hid::h5f>)
                info.h5File = loc;
            else
                info.h5File = H5Iget_file_id(loc);
        }

        if(not info.tableExists)
            info.tableExists = h5pp::hdf5::checkIfLinkExists(info.getLocId(), info.tablePath.value(), plists.linkAccess);

        // Infer the group name
        if(not info.tableGroupName) {
            info.tableGroupName = "";
            size_t pos          = info.tablePath.value().find_last_of('/');
            if(pos != std::string::npos)
                info.tableGroupName.value().assign(info.tablePath->begin(), info.tablePath->begin() + static_cast<long>(pos));
        }
        // This is as far as we get if the table does not exist
        if(not info.tableExists.value()) return;
        if(not info.h5Dset)
            info.h5Dset = hdf5::openLink<hid::h5d>(info.getLocId(), info.tablePath.value(), info.tableExists, plists.linkAccess);
        if(not info.h5Type) info.h5Type = H5Dget_type(info.h5Dset.value());
        if(not info.numRecords) {
            // We could use H5TBget_table_info here but internally that would create a temporary
            // dataset id and type id, but we already have them, so we can use these directly instead
            auto dims = h5pp::hdf5::getDimensions(info.h5Dset.value());
            if(dims.size() != 1) throw h5pp::logic_error("Tables can only have rank 1");
            info.numRecords = dims[0];
        }
        if(not info.numFields) {
            auto nmembers = H5Tget_nmembers(info.h5Type.value());
            if(nmembers < 0) throw h5pp::runtime_error("Failed to read nmembers for h5Type on table [{}]", info.tablePath.value());
            info.numFields = static_cast<hsize_t>(nmembers);
        }
        if(not info.tableTitle) {
            char   table_title[255];
            herr_t err = H5TBAget_title(info.h5Dset.value(), table_title);
            if(err < 0) throw h5pp::runtime_error("Failed to read title for table [{}]", info.tablePath.value());
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
            char                   **field_names = new char *[n_fields];
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
        if(not info.h5DsetCreate) info.h5DsetCreate = H5Dget_create_plist(info.h5Dset.value());
        if(not info.h5DsetAccess) info.h5DsetAccess = H5Dget_access_plist(info.h5Dset.value());
        if(not info.chunkDims) info.chunkDims = h5pp::hdf5::getChunkDimensions(info.h5DsetCreate.value());
        if(not info.h5Filters) info.h5Filters = h5pp::hdf5::getFilters(info.h5DsetCreate.value());
        if(not info.compression) info.compression = h5pp::hdf5::getDeflateLevel(info.h5DsetCreate.value());

        // Get c++ properties
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize) {
            info.cppTypeIndex = std::vector<std::type_index>();
            info.cppTypeName  = std::vector<std::string>();
            info.cppTypeSize  = std::vector<size_t>();
            for(size_t idx = 0; idx < info.numFields.value(); idx++) {
                auto cppInfo = h5pp::hdf5::getCppType(info.fieldTypes.value()[idx]);
                info.cppTypeIndex->emplace_back(std::get<0>(cppInfo));
                info.cppTypeName->emplace_back(std::get<1>(cppInfo));
                info.cppTypeSize->emplace_back(std::get<2>(cppInfo));
            }
        }
    }

    template<typename h5x>
    [[nodiscard]] inline TableInfo readTableInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        TableInfo info;
        readTableInfo(info, loc, options, plists);
        return info;
    }

    template<typename h5x>
    inline void makeTableInfo(h5pp::TableInfo     &info,
                              const h5x           &loc,
                              const Options       &options,
                              std::string_view     tableTitle,
                              const PropertyLists &plists = PropertyLists()) {
        readTableInfo(info, loc, options, plists);
        if(info.tableExists.value()) return;
        if(not options.linkPath and not info.tablePath) throw h5pp::runtime_error("Could not make table info: No table path was given");
        if(not info.tablePath) info.tablePath = h5pp::util::safe_str(options.linkPath.value());

        h5pp::logger::log->debug("Creating metadata for new table [{}]", info.tablePath.value());
        if(not options.h5Type and not info.h5Type) throw h5pp::runtime_error("Could not make table info: No hdf5 compound type was given");

        /* clang-format off */
        if(not info.tableTitle       ) info.tableTitle       = tableTitle;
        if(not info.h5Type           ) info.h5Type           = options.h5Type;
        if(not info.numFields        ) info.numFields        = H5Tget_nmembers(info.h5Type.value());
        if(not info.numRecords       ) info.numRecords       = 0;
        if(not info.recordBytes      ) info.recordBytes      = H5Tget_size(info.h5Type.value());
        if(not info.compression      ) info.compression      = options.compression;
        if(not info.chunkDims        ) info.chunkDims        = options.dsetChunkDims;

        if(not info.chunkDims)   info.chunkDims = h5pp::util::getChunkDimensions(info.recordBytes.value(), {1}, std::nullopt, H5D_layout_t::H5D_CHUNKED);
        if(not info.compression) info.compression = h5pp::hdf5::getValidCompressionLevel();

        if(not info.fieldTypes){
            info.fieldTypes   = std::vector<h5pp::hid::h5t>(info.numFields.value());
            for(unsigned int idx = 0; idx < info.fieldTypes->size(); idx++)
                info.fieldTypes.value()[idx] = H5Tget_member_type(info.h5Type.value(), idx);
        }
        if(not info.fieldOffsets){
            info.fieldOffsets = std::vector<size_t>(info.numFields.value());
            for(unsigned int idx = 0; idx < info.fieldOffsets->size(); idx++)
                info.fieldOffsets.value()[idx] = H5Tget_member_offset(info.h5Type.value(), idx);
        }
        if(not info.fieldSizes){
            info.fieldSizes   = std::vector<size_t>(info.numFields.value());
            for(unsigned int idx = 0; idx < info.fieldSizes->size(); idx++)
                info.fieldSizes.value()[idx] = H5Tget_size(info.fieldTypes.value()[idx]);
        }
        if(not info.fieldNames){
            info.fieldNames   = std::vector<std::string>(info.numFields.value());
            for(unsigned int idx = 0; idx < info.fieldNames->size(); idx++){
                const char *name = H5Tget_member_name(info.h5Type.value(), idx);
                info.fieldNames.value()[idx] = name;
                H5free_memory((void *) name);
            }
        }

        // Get c++ properties
        if(not info.cppTypeIndex or not info.cppTypeName or not info.cppTypeSize){
            info.cppTypeIndex = std::vector<std::type_index>();
            info.cppTypeName  = std::vector<std::string>();
            info.cppTypeSize  = std::vector<size_t>();
            for(size_t idx = 0; idx < info.numFields.value(); idx++) {
                auto cppInfo = h5pp::hdf5::getCppType(info.fieldTypes.value()[idx]);
                info.cppTypeIndex->emplace_back(std::get<0>(cppInfo));
                info.cppTypeName->emplace_back(std::get<1>(cppInfo));
                info.cppTypeSize->emplace_back(std::get<2>(cppInfo));
            }
        }
        /* clang-format on */
    }

    template<typename h5x>
    [[nodiscard]] inline h5pp::TableInfo
        makeTableInfo(const h5x &loc, const Options &options, std::string_view tableTitle, const PropertyLists &plists = PropertyLists()) {
        TableInfo info;
        makeTableInfo(info, loc, options, tableTitle, plists);
        return info;
    }

    template<typename h5x>
    inline void inferTableInfo(TableInfo &info, const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5pp_loc_id<h5x>,
                      "Template function [h5pp::scan::inferTableInfo(..., const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");
        if(not options.linkPath and not info.tablePath) throw h5pp::runtime_error("Could not infer table info: No table path was given");
        if(not info.tablePath) info.tablePath = h5pp::util::safe_str(options.linkPath.value());

        if(not info.tableExists)
            info.tableExists = h5pp::hdf5::checkIfLinkExists(info.getLocId(), info.tablePath.value(), plists.linkAccess);

        if(info.tableExists.value())
            readTableInfo(info, loc, options, plists); // Table exists so we can read properties from file
        else if(not info.tableExists.value() and info.tableTitle)
            makeTableInfo(info, loc, options, info.tableTitle.value(), plists);
        else if(not info.tableTitle)
            throw h5pp::runtime_error("Could not infer table info for new table [{}]: No table title given", info.tablePath.value());
    }

    /*! \brief Populates an AttrInfo object with properties read from file */
    template<typename h5x>
    inline void readLinkInfo(LinkInfo &info, const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        static_assert(h5pp::type::sfinae::is_h5pp_loc_id<h5x>,
                      "Template function [h5pp::scan::readLinkInfo(..., const h5x & loc, ...)] requires type h5x to be: "
                      "[h5pp::hid::h5f], [h5pp::hid::h5g] or [h5pp::hid::h5o]");

        if(not options.linkPath and not info.linkPath) throw h5pp::runtime_error("Could not read attribute info: No link path was given");
        if(not info.linkPath) info.linkPath = h5pp::util::safe_str(options.linkPath.value());

        h5pp::logger::log->debug("Scanning header of object [{}]", info.linkPath.value());

        // Copy the location
        if(not info.h5File) {
            if constexpr(std::is_same_v<h5x, hid::h5f>)
                info.h5File = loc;
            else
                info.h5File = H5Iget_file_id(loc);
        }

        /* It's important to note the convention used here:
         *      * linkPath is relative to loc.
         *      * loc can be a file or group, but NOT a dataset.
         *      * h5Link is the object on which the attribute is attached.
         *      * h5Link is an h5o object which means that it can be a file, group or dataset.
         *      * loc != h5Link.
         *
         */

        if(not info.linkExists) info.linkExists = h5pp::hdf5::checkIfLinkExists(info.getLocId(), info.linkPath.value(), plists.linkAccess);

        // If the link does not exist, there isn't much else to do so we return;
        if(info.linkExists and not info.linkExists.value()) return;

        // From here on the link exists
        if(not info.h5Link) info.h5Link = h5pp::hdf5::openLink<hid::h5o>(loc, info.linkPath.value(), info.linkExists, plists.linkAccess);

        H5O_hdr_info_t hInfo;
        H5O_info_t     oInfo;
#if defined(H5Oget_info_vers) && H5Oget_info_vers >= 3
        H5O_native_info_t nInfo;
        herr_t            nerr = H5Oget_native_info(info.h5Link.value(), &nInfo, H5O_NATIVE_INFO_HDR);
        herr_t            oerr = H5Oget_info(info.h5Link.value(), &oInfo, H5O_INFO_BASIC | H5O_INFO_TIME | H5O_INFO_NUM_ATTRS);
        hInfo                  = nInfo.hdr;
        if(nerr != 0)
            throw h5pp::runtime_error("H5Oget_native_info returned error code {} when reading link {}", nerr, info.linkPath.value());
#elif defined(H5Oget_info_vers) && H5Oget_info_vers >= 2
        herr_t oerr = H5Oget_info(info.h5Link.value(), &oInfo, H5O_INFO_HDR);
        hInfo       = oInfo.hdr;
#else
        herr_t oerr = H5Oget_info(info.h5Link.value(), &oInfo);
        hInfo       = oInfo.hdr;
#endif
        if(oerr != 0) throw h5pp::runtime_error("H5Oget_info returned error code {} when reading link {}", oerr, info.linkPath.value());

        info.h5HdrInfo = hInfo;
        info.h5HdrByte = hInfo.space.total;
        info.h5ObjType = oInfo.type;
        info.refCount  = oInfo.rc;
        info.atime     = oInfo.atime;
        info.mtime     = oInfo.mtime;
        info.ctime     = oInfo.ctime;
        info.btime     = oInfo.btime;
        info.num_attrs = oInfo.num_attrs;

        h5pp::logger::log->trace("Scanned header metadata {}", info.string(h5pp::logger::logIf(LogLevel::trace)));
    }

    /*! \brief Creates and returns a populated AttrInfo object with properties read from file */
    template<typename h5x>
    [[nodiscard]] inline h5pp::LinkInfo
        readLinkInfo(const h5x &loc, const Options &options, const PropertyLists &plists = PropertyLists()) {
        h5pp::LinkInfo info;
        readLinkInfo(info, loc, options, plists);
        return info;
    }

}
