#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/h5pp.h>
#include <string_view>
#include <vector>

namespace {
    h5pp::File make_file(std::string_view name) { return h5pp::File(h5pp::format(H5PP_TEST_DIR "{}.h5", name), h5pp::FileAccess::REPLACE, 0); }

    void require_dims(const h5pp::DsetInfo &info, const std::vector<hsize_t> &expected) {
        REQUIRE(info.dsetDims.has_value());
        REQUIRE(info.dsetDims.value() == expected);
    }

    void require_max_dims(const h5pp::DsetInfo &info, const std::vector<hsize_t> &expected) {
        REQUIRE(info.dsetDimsMax.has_value());
        REQUIRE(info.dsetDimsMax.value() == expected);
    }

    void require_chunk_dims(const std::optional<std::vector<hsize_t>> &actual, const std::vector<hsize_t> &expected) {
        REQUIRE(actual.has_value());
        REQUIRE(actual.value() == expected);
    }

    void require_chunk_dims(const h5pp::DsetInfo &info, const std::vector<hsize_t> &expected) {
        REQUIRE(info.dsetChunk.has_value());
        REQUIRE(info.dsetChunk.value() == expected);
    }

    void require_no_chunk_dims(const std::optional<std::vector<hsize_t>> &actual) { REQUIRE_FALSE(actual.has_value()); }

    std::vector<double> make_linear_data(size_t count, double start = 0.0) {
        std::vector<double> data(count);
        for(size_t idx = 0; idx < count; idx++) data[idx] = start + static_cast<double>(idx);
        return data;
    }
}

TEST_CASE("Automatic chunking reports concrete chunk dimensions for large complex vectors", "[chunk][auto]") {
    auto file = make_file("chunkDimension-auto-complex");

    std::vector<std::complex<double>> data(10000, {10.0, 5.0});
    h5pp::DatasetCreateOptions        create;
    create.h5Layout = H5D_CHUNKED;
    file.writeDataset(data, "chunkedgroup/vectorComplexDouble", create);
    auto info = file.getDatasetInfo("chunkedgroup/vectorComplexDouble");

    REQUIRE(info.h5Layout.has_value());
    REQUIRE(info.h5Layout.value() == H5D_CHUNKED);
    require_dims(info, {data.size()});
    require_chunk_dims(info, {16384});
    require_chunk_dims(file.dataset("chunkedgroup/vectorComplexDouble").getChunkDimensions(), {16384});

    REQUIRE(file.readDataset<std::vector<std::complex<double>>>("chunkedgroup/vectorComplexDouble") == data);
}

TEST_CASE("Chunk dimension metadata covers manual, capped, extensible and non-chunked datasets", "[chunk][metadata]") {
    SECTION("Manual chunk dimensions are preserved exactly") {
        auto file = make_file("chunkDimension-manual");

        auto data = make_linear_data(1000, 1.0);
        h5pp::DatasetCreateOptions create;
        create.dims      = {1000};
        create.h5Layout  = H5D_CHUNKED;
        create.dimsChunk = {10000};
        file.writeDataset(data, "chunked/manual", create);
        auto info = file.getDatasetInfo("chunked/manual");

        REQUIRE(info.h5Layout.has_value());
        REQUIRE(info.h5Layout.value() == H5D_CHUNKED);
        require_dims(info, {1000});
        require_chunk_dims(info, {10000});
        require_chunk_dims(file.dataset("chunked/manual").getChunkDimensions(), {10000});
        REQUIRE(file.readDataset<std::vector<double>>("chunked/manual") == data);
    }

    SECTION("Automatic chunk dimensions respect fixed max dimensions") {
        auto file = make_file("chunkDimension-fixed-max");

        auto data = make_linear_data(100, 1.0);
        h5pp::DatasetCreateOptions create;
        create.dims     = {10, 10};
        create.h5Layout = H5D_CHUNKED;
        create.dimsMax  = {32, 64};
        file.writeDataset(data, "chunked/fixedMax", create);
        auto info = file.getDatasetInfo("chunked/fixedMax");

        REQUIRE(info.h5Layout.has_value());
        REQUIRE(info.h5Layout.value() == H5D_CHUNKED);
        require_dims(info, {10, 10});
        require_max_dims(info, {32, 64});
        require_chunk_dims(info, {32, 64});
        require_chunk_dims(file.dataset("chunked/fixedMax").getChunkDimensions(), {32, 64});
        REQUIRE(file.readDataset<std::vector<double>>("chunked/fixedMax") == data);
    }

    SECTION("Empty extensible datasets still get usable automatic chunk dimensions") {
        auto file = make_file("chunkDimension-empty-extensible");

        h5pp::DatasetCreateOptions create;
        create.dims     = {0, 2};
        create.h5Layout = H5D_CHUNKED;
        create.dimsMax  = {H5S_UNLIMITED, 2};
        create.h5Type   = H5Tcopy(H5T_NATIVE_DOUBLE);
        auto info       = file.dataset("chunked/extensible").create(create);

        REQUIRE(info.h5Layout.has_value());
        REQUIRE(info.h5Layout.value() == H5D_CHUNKED);
        require_dims(info, {0, 2});
        require_max_dims(info, {H5S_UNLIMITED, 2});
        require_chunk_dims(info, {46, 2});
        require_chunk_dims(file.dataset("chunked/extensible").getChunkDimensions(), {46, 2});

        h5pp::DatasetAppendOptions append;
        append.dims      = {1, 2};
        auto appended    = file.dataset("chunked/extensible").append(std::vector<double>{1.0, 2.0}, 0, append);
        require_dims(appended, {1, 2});
        require_chunk_dims(file.dataset("chunked/extensible").getChunkDimensions(), {46, 2});
        REQUIRE(file.readDataset<std::vector<double>>("chunked/extensible") == std::vector<double>{1.0, 2.0});
    }

    SECTION("Contiguous and compact datasets report no chunk dimensions") {
        auto file = make_file("chunkDimension-nonchunked");

        std::vector<int> compact_data = {1, 2, 3, 4};
        h5pp::DatasetCreateOptions compact_create;
        compact_create.h5Layout = H5D_COMPACT;
        file.writeDataset(compact_data, "layout/compact", compact_create);
        auto compact_info = file.getDatasetInfo("layout/compact");
        REQUIRE(compact_info.h5Layout.has_value());
        REQUIRE(compact_info.h5Layout.value() == H5D_COMPACT);
        require_no_chunk_dims(file.dataset("layout/compact").getChunkDimensions());

        auto contiguous_data = make_linear_data(128, 5.0);
        h5pp::DatasetCreateOptions contiguous_create;
        contiguous_create.h5Layout = H5D_CONTIGUOUS;
        file.writeDataset(contiguous_data, "layout/contiguous", contiguous_create);
        auto contiguous_info = file.getDatasetInfo("layout/contiguous");
        REQUIRE(contiguous_info.h5Layout.has_value());
        REQUIRE(contiguous_info.h5Layout.value() == H5D_CONTIGUOUS);
        require_no_chunk_dims(file.dataset("layout/contiguous").getChunkDimensions());

        REQUIRE(file.readDataset<std::vector<int>>("layout/compact") == compact_data);
        REQUIRE(file.readDataset<std::vector<double>>("layout/contiguous") == contiguous_data);
    }
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    return session.run();
}
