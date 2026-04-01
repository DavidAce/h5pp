#include <algorithm>
#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/v1/h5pp.h>
#include <string_view>
#include <vector>

namespace {
    h5pp::v1::File make_file(std::string_view name) { return h5pp::v1::File(h5pp::format(H5PP_TEST_DIR "{}.h5", name), h5pp::FileAccess::REPLACE, 0); }

    bool compression_available() { return h5pp::hdf5::isCompressionAvaliable(); }

    bool has_deflate_filter(const h5pp::DsetInfo &info) {
        return info.h5Filters.has_value() and (info.h5Filters.value() & H5Z_FILTER_DEFLATE) == H5Z_FILTER_DEFLATE;
    }

    void require_dims(const h5pp::DsetInfo &info, const std::vector<hsize_t> &expected) {
        REQUIRE(info.dsetDims.has_value());
        REQUIRE(info.dsetDims.value() == expected);
    }

    void require_chunked(const h5pp::DsetInfo &info) {
        REQUIRE(info.h5Layout.has_value());
        REQUIRE(info.h5Layout.value() == H5D_CHUNKED);
        REQUIRE(info.dsetChunk.has_value());
    }

    void require_contiguous(const h5pp::DsetInfo &info) {
        REQUIRE(info.h5Layout.has_value());
        REQUIRE(info.h5Layout.value() == H5D_CONTIGUOUS);
    }

    void require_deflate_level(const h5pp::DsetInfo &info, int expected_level) {
        REQUIRE(info.compression.has_value());
        REQUIRE(info.compression.value() == expected_level);
        REQUIRE(has_deflate_filter(info));
    }

    void require_no_deflate(const h5pp::DsetInfo &info) {
        REQUIRE(info.compression.has_value());
        REQUIRE(info.compression.value() == -1);
        REQUIRE_FALSE(has_deflate_filter(info));
    }
}

TEST_CASE("Default compression applies to chunked writes but does not force chunking", "[compression][default]") {
    auto file = make_file("compressedDataset-default");

    std::vector<double> big_data(10000 * 1024, 2.3);
    std::vector<double> medium_data(10000, 2.3);
    file.setCompressionLevel(9);
    REQUIRE(file.getCompressionLevel() == (compression_available() ? 9 : 0));

    file.writeDataset(big_data, "compressedWriteGroup/bigVector");
    auto big_info = file.getDatasetInfo("compressedWriteGroup/bigVector");

    require_dims(big_info, {big_data.size()});
    require_chunked(big_info);
    if(compression_available()) require_deflate_level(big_info, 9);
    else require_no_deflate(big_info);

    file.writeDataset(medium_data, "compressedWriteGroup/mediumVector");
    auto medium_info = file.getDatasetInfo("compressedWriteGroup/mediumVector");

    require_dims(medium_info, {medium_data.size()});
    require_contiguous(medium_info);
    require_no_deflate(medium_info);

    REQUIRE(file.readDataset<std::vector<double>>("compressedWriteGroup/mediumVector") == medium_data);
}

TEST_CASE("Compression helpers cover overrides, clamping and ignored requests", "[compression][apis]") {
    auto file = make_file("compressedDataset-apis");

    std::vector<double> data(10000, 2.3);
    file.setCompressionLevel(2);
    REQUIRE(file.getCompressionLevel() == (compression_available() ? 2 : 0));

    file.writeDataset_chunked(data, "compressedWriteGroup/overrideLevel", std::nullopt, std::nullopt, std::nullopt, std::nullopt, 7);
    auto override_info = file.getDatasetInfo("compressedWriteGroup/overrideLevel");
    require_chunked(override_info);
    if(compression_available()) require_deflate_level(override_info, 7);
    else require_no_deflate(override_info);

    file.writeDataset_compressed(data, "compressedWriteGroup/clampedLevel", 42);
    auto clamped_info = file.getDatasetInfo("compressedWriteGroup/clampedLevel");
    require_chunked(clamped_info);
    if(compression_available()) require_deflate_level(clamped_info, 9);
    else require_no_deflate(clamped_info);

    file.writeDataset(data,
                      "compressedWriteGroup/explicitContiguous",
                      {data.size()},
                      H5D_CONTIGUOUS,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      7);
    auto ignored_info = file.getDatasetInfo("compressedWriteGroup/explicitContiguous");
    require_contiguous(ignored_info);
    require_no_deflate(ignored_info);

    REQUIRE(file.readDataset<std::vector<double>>("compressedWriteGroup/overrideLevel") == data);
    REQUIRE(file.readDataset<std::vector<double>>("compressedWriteGroup/clampedLevel") == data);
    REQUIRE(file.readDataset<std::vector<double>>("compressedWriteGroup/explicitContiguous") == data);
}

#ifdef H5PP_USE_EIGEN3
TEST_CASE("Eigen tensors still write through the compression path", "[compression][eigen]") {
    auto file = make_file("compressedDataset-eigen");

    if(not compression_available()) {
        SUCCEED("Compression is unavailable in this HDF5 build");
        return;
    }

    file.setCompressionLevel(9);
    Eigen::Tensor<double, 4> bigTensor(40, 180, 40, 5);
    bigTensor.setConstant(1.0);

    file.writeDataset(bigTensor, "compressedWriteGroup/bigTensor");
    auto info = file.getDatasetInfo("compressedWriteGroup/bigTensor");

    require_chunked(info);
    require_dims(info, {40, 180, 40, 5});
    require_deflate_level(info, 9);

    auto readback = file.readDataset<Eigen::Tensor<double, 4>>("compressedWriteGroup/bigTensor");
    REQUIRE(readback.size() == bigTensor.size());
    REQUIRE(std::equal(readback.data(), readback.data() + readback.size(), bigTensor.data()));
}
#endif

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
