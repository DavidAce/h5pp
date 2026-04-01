#include <algorithm>
#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/h5pp.h>
#include <string_view>
#include <vector>

namespace {
    h5pp::File make_file(std::string_view name) {
        h5pp::fs::create_directories("output");
        return h5pp::File(h5pp::format(H5PP_TEST_DIR "{}.h5", name), h5pp::FileAccess::REPLACE, 0);
    }

    bool compression_available() { return h5pp::hdf5::isCompressionAvaliable(); }

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

    void require_compression(const h5pp::DsetInfo &info, int expected) {
        REQUIRE(info.compression.has_value());
        REQUIRE(info.compression.value() == expected);
    }
}

TEST_CASE("Default compression applies to chunked writes but does not force chunking", "[compression][default]") {
    auto file = make_file("compressedDataset-default");

    std::vector<double> bigData(10000 * 1024, 2.3);
    std::vector<double> mediumData(10000, 2.3);
    file.setCompressionLevel(9);
    REQUIRE(file.getCompressionLevel() == (compression_available() ? 9 : 0));

    file.writeDataset(bigData, "compressedWriteGroup/bigVector");
    auto bigInfo = file.dataset("compressedWriteGroup/bigVector").getInfo();

    require_dims(bigInfo, {bigData.size()});
    require_chunked(bigInfo);
    REQUIRE(bigInfo.compression.has_value());
    REQUIRE(bigInfo.compression.value() == (compression_available() ? 9 : -1));

    file.writeDataset(mediumData, "compressedWriteGroup/mediumVector");
    auto mediumInfo = file.dataset("compressedWriteGroup/mediumVector").getInfo();

    require_dims(mediumInfo, {mediumData.size()});
    require_contiguous(mediumInfo);
    require_compression(mediumInfo, -1);

    REQUIRE(file.readDataset<std::vector<double>>("compressedWriteGroup/mediumVector") == mediumData);
}

TEST_CASE("Compression settings cover overrides, clamping and ignored requests", "[compression][apis]") {
    auto file = make_file("compressedDataset-apis");

    std::vector<double> data(10000, 2.3);
    file.setCompressionLevel(2);
    REQUIRE(file.getCompressionLevel() == (compression_available() ? 2 : 0));

    h5pp::DatasetCreateOptions overrideCreate;
    overrideCreate.h5Layout    = H5D_CHUNKED;
    overrideCreate.compression = 7;
    file.writeDataset(data, "compressedWriteGroup/overrideLevel", overrideCreate);
    auto overrideInfo = file.dataset("compressedWriteGroup/overrideLevel").getInfo();
    require_chunked(overrideInfo);
    REQUIRE(overrideInfo.compression.has_value());
    REQUIRE(overrideInfo.compression.value() == (compression_available() ? 7 : -1));

    h5pp::DatasetCreateOptions clampedCreate;
    clampedCreate.h5Layout    = H5D_CHUNKED;
    clampedCreate.compression = 42;
    file.writeDataset(data, "compressedWriteGroup/clampedLevel", clampedCreate);
    auto clampedInfo = file.dataset("compressedWriteGroup/clampedLevel").getInfo();
    require_chunked(clampedInfo);
    REQUIRE(clampedInfo.compression.has_value());
    REQUIRE(clampedInfo.compression.value() == (compression_available() ? 9 : -1));

    h5pp::DatasetCreateOptions contiguousCreate;
    contiguousCreate.dims        = std::vector<hsize_t>{data.size()};
    contiguousCreate.h5Layout    = H5D_CONTIGUOUS;
    contiguousCreate.compression = 7;
    file.writeDataset(data, "compressedWriteGroup/explicitContiguous", contiguousCreate);
    auto ignoredInfo = file.dataset("compressedWriteGroup/explicitContiguous").getInfo();
    require_contiguous(ignoredInfo);
    require_compression(ignoredInfo, -1);

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
    auto info = file.dataset("compressedWriteGroup/bigTensor").getInfo();

    require_chunked(info);
    require_dims(info, {40, 180, 40, 5});
    require_compression(info, 9);

    auto readback = file.readDataset<Eigen::Tensor<double, 4>>("compressedWriteGroup/bigTensor");
    REQUIRE(readback.size() == bigTensor.size());
    REQUIRE(std::equal(readback.data(), readback.data() + readback.size(), bigTensor.data()));
}
#endif

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
