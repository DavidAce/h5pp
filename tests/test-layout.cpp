#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <string_view>
#include <vector>

namespace {
    h5pp::File make_file(std::string_view name) { return h5pp::File(h5pp::format(H5PP_TEST_DIR "{}.h5", name), h5pp::FileAccess::REPLACE, 0); }

    void require_layout(const h5pp::DsetInfo &info, H5D_layout_t expected) {
        REQUIRE(info.h5Layout);
        REQUIRE(info.h5Layout.value() == expected);
    }

    void require_chunked(const h5pp::DsetInfo &info) {
        require_layout(info, H5D_CHUNKED);
        REQUIRE(info.dsetChunk);
        REQUIRE_FALSE(info.dsetChunk->empty());
    }
}

TEST_CASE("Automatic layout selection follows the compact, contiguous and chunked byte thresholds", "[layout][auto][thresholds]") {
    auto file = make_file("layout-thresholds");

    const auto compact_count    = h5pp::constants::maxSizeCompact / sizeof(double) - 1ul;
    const auto contiguous_count = h5pp::constants::maxSizeCompact / sizeof(double);
    const auto chunked_count    = h5pp::constants::maxSizeContiguous / sizeof(double);

    std::vector<double> compact_data(compact_count, 1.0);
    std::vector<double> contiguous_data(contiguous_count, 2.0);
    std::vector<double> chunked_data(chunked_count, 3.0);

    file.writeDataset(compact_data, "layout/compact");
    file.writeDataset(contiguous_data, "layout/contiguous");
    file.writeDataset(chunked_data, "layout/chunked");

    auto compact_info    = file.dataset("layout/compact").getInfo();
    auto contiguous_info = file.dataset("layout/contiguous").getInfo();
    auto chunked_info    = file.dataset("layout/chunked").getInfo();

    require_layout(compact_info, H5D_COMPACT);
    require_layout(contiguous_info, H5D_CONTIGUOUS);
    require_chunked(chunked_info);

    REQUIRE(file.readDataset<std::vector<double>>("layout/compact") == compact_data);
    REQUIRE(file.readDataset<std::vector<double>>("layout/contiguous") == contiguous_data);
    REQUIRE(file.readDataset<std::vector<double>>("layout/chunked") == chunked_data);
}

TEST_CASE("Automatic layout selection treats multidimensional data by total byte size", "[layout][auto][multidim]") {
    auto file = make_file("layout-multidim");

    std::vector<double> compact_data(63 * 64, 1.0);   // 32256 bytes
    std::vector<double> contiguous_data(64 * 64, 2.0); // 32768 bytes
    std::vector<double> chunked_data(256 * 256, 3.0);  // 524288 bytes

    h5pp::DatasetWriteOptions compact_write;
    compact_write.dims = {63, 64};
    h5pp::DatasetWriteOptions contiguous_write;
    contiguous_write.dims = {64, 64};
    h5pp::DatasetWriteOptions chunked_write;
    chunked_write.dims = {256, 256};

    file.dataset("layout/compact2d").write(compact_data, compact_write);
    file.dataset("layout/contiguous2d").write(contiguous_data, contiguous_write);
    file.dataset("layout/chunked2d").write(chunked_data, chunked_write);

    auto compact_info    = file.dataset("layout/compact2d").getInfo();
    auto contiguous_info = file.dataset("layout/contiguous2d").getInfo();
    auto chunked_info    = file.dataset("layout/chunked2d").getInfo();

    require_layout(compact_info, H5D_COMPACT);
    require_layout(contiguous_info, H5D_CONTIGUOUS);
    require_chunked(chunked_info);
    REQUIRE(chunked_info.dsetChunk.value() == std::vector<hsize_t>{256, 256});

    REQUIRE(file.readDataset<std::vector<double>>("layout/compact2d") == compact_data);
    REQUIRE(file.readDataset<std::vector<double>>("layout/contiguous2d") == contiguous_data);
    REQUIRE(file.readDataset<std::vector<double>>("layout/chunked2d") == chunked_data);
}

TEST_CASE("Automatic layout selection forces chunking for extensible datasets regardless of size", "[layout][auto][extensible]") {
    auto file = make_file("layout-extensible");

    std::vector<int> data = {1, 2};

    h5pp::DatasetCreateOptions unlimited_create;
    unlimited_create.dims    = {data.size()};
    unlimited_create.dimsMax = {H5S_UNLIMITED};
    file.writeDataset(data, "layout/unlimited", unlimited_create);

    h5pp::DatasetCreateOptions capped_create;
    capped_create.dims    = {data.size()};
    capped_create.dimsMax = {4};
    file.writeDataset(data, "layout/capped", capped_create);

    auto unlimited_info = file.dataset("layout/unlimited").getInfo();
    auto capped_info    = file.dataset("layout/capped").getInfo();

    require_chunked(unlimited_info);
    require_chunked(capped_info);
    REQUIRE(unlimited_info.dsetDimsMax);
    REQUIRE(unlimited_info.dsetDimsMax.value() == std::vector<hsize_t>{H5S_UNLIMITED});
    REQUIRE(capped_info.dsetDimsMax);
    REQUIRE(capped_info.dsetDimsMax.value() == std::vector<hsize_t>{4});

    REQUIRE(file.readDataset<std::vector<int>>("layout/unlimited") == data);
    REQUIRE(file.readDataset<std::vector<int>>("layout/capped") == data);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
