#pragma once

#include "h5ppInitListType.h"
#include "h5ppOptions.h"
#include "h5ppScan.h"
#include "h5ppTypeTraits.h"
#include "h5ppVersion.h"

namespace h5pp::inline v2 {
    class File;
}

namespace h5pp::inline v2::detail {
    [[nodiscard]] inline std::string normalizeLinkPath(std::string_view linkPath) {
        auto path = h5pp::util::safe_str(linkPath);
        if(path.empty()) return "/";
        if(path == "/") return path;
        if(path.front() == '/') path.erase(0, 1);
        while(path.size() > 1 and path.back() == '/') path.pop_back();
        return path;
    }

    [[nodiscard]] inline std::string joinLinkPath(std::string_view rootPath, std::string_view childPath) {
        auto root  = normalizeLinkPath(rootPath);
        auto child = normalizeLinkPath(childPath);
        if(root == "/") return child;
        if(child == "/") return root;
        if(child.empty()) return root;
        return h5pp::format("{}/{}", root, child);
    }

    [[nodiscard]] inline h5pp::OptDimsType toOptDimsType(const OptDims &dims) {
        if(dims.has_value()) return h5pp::OptDimsType(dims.value());
        return std::nullopt;
    }

    [[nodiscard]] inline Options makeOptions(int                             compression,
                                             std::string_view                dsetPath,
                                             const std::optional<Hyperslab> &dsetSlab,
                                             const DatasetWriteOptions      &writeOptions) {
        Options options;
        options.linkPath     = h5pp::util::safe_str(dsetPath);
        options.dataDims     = toOptDimsType(writeOptions.dims);
        options.dataSlab     = writeOptions.dataSlab;
        options.dsetSlab     = dsetSlab;
        options.h5Type       = writeOptions.h5Type;
        options.resizePolicy = writeOptions.resizePolicy;
        options.compression  = compression;
        return options;
    }

    [[nodiscard]] inline Options makeOptions(int compression, std::string_view dsetPath, const DatasetCreateOptions &createOptions) {
        Options options;
        options.linkPath      = h5pp::util::safe_str(dsetPath);
        options.dataDims      = toOptDimsType(createOptions.dims);
        options.dsetChunkDims = toOptDimsType(createOptions.dimsChunk);
        options.dsetMaxDims   = toOptDimsType(createOptions.dimsMax);
        options.h5Layout      = createOptions.h5Layout;
        options.h5Type        = createOptions.h5Type;
        options.compression =
            createOptions.compression.has_value() ? hdf5::getValidCompressionLevel(createOptions.compression) : compression;
        return options;
    }

    [[nodiscard]] inline Options makeOptions(std::string_view dsetPath, const DatasetAppendOptions &appendOptions) {
        Options options;
        options.linkPath = h5pp::util::safe_str(dsetPath);
        options.dataDims = toOptDimsType(appendOptions.dims);
        options.h5Type   = appendOptions.h5Type;
        return options;
    }

    [[nodiscard]] inline Options
        makeOptions(std::string_view dsetPath, const std::optional<Hyperslab> &dsetSlab, const DatasetReadOptions &readOptions) {
        Options options;
        options.linkPath = h5pp::util::safe_str(dsetPath);
        if(readOptions.dims.has_value()) {
            options.dataDims = toOptDimsType(readOptions.dims);
        } else if(not readOptions.dataSlab.has_value() and dsetSlab.has_value() and dsetSlab->extent.has_value()) {
            options.dataDims = h5pp::OptDimsType(dsetSlab->extent.value());
            if(dsetSlab->blocks.has_value())
                for(size_t axis = 0; axis < options.dataDims->size(); ++axis) options.dataDims->at(axis) *= dsetSlab->blocks->at(axis);
        }
        options.dataSlab = readOptions.dataSlab;
        options.dsetSlab = dsetSlab;
        options.h5Type   = readOptions.h5Type;
        return options;
    }

    [[nodiscard]] inline Options
        makeOptions(std::string_view linkPath, std::string_view attrName, const AttributeWriteOptions &writeOptions) {
        Options options;
        options.linkPath = h5pp::util::safe_str(linkPath);
        options.attrName = h5pp::util::safe_str(attrName);
        options.dataDims = toOptDimsType(writeOptions.dims);
        options.h5Type   = writeOptions.h5Type;
        return options;
    }

    [[nodiscard]] inline Options
        makeOptions(std::string_view linkPath, std::string_view attrName, const AttributeReadOptions &readOptions) {
        Options options;
        options.linkPath = h5pp::util::safe_str(linkPath);
        options.attrName = h5pp::util::safe_str(attrName);
        options.dataDims = toOptDimsType(readOptions.dims);
        options.h5Type   = readOptions.h5Type;
        return options;
    }
}
