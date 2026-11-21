#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <h5pp/v1/h5pp.h>
#include <string>
#include <vector>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }

    const std::vector<std::optional<H5D_layout_t>> layouts      = {std::nullopt, H5D_COMPACT, H5D_CONTIGUOUS, H5D_CHUNKED};
    const std::vector<std::string>                 layout_names = {"auto", "compact", "contiguous", "chunked"};

    void require_zero_vector(h5pp::File &file, std::string_view path) {
        REQUIRE(file.dataset(path).read<std::vector<double>>() == std::vector<double>{0, 0, 0, 0});
    }
}

TEST_CASE("v1 dataset calls work alongside the v2 API", "[v1][v2][dataset]") {
    auto file = h5pp::File(make_path("datasetApi"), h5pp::FileAccess::REPLACE, 0);

    for(size_t idx = 0; idx < layouts.size(); ++idx) {
        SECTION(h5pp::format("v2 API create: {}", layout_names[idx])) {
            h5pp::DatasetCreateOptions create;
            create.h5Type   = H5Tcopy(H5T_NATIVE_DOUBLE);
            create.dims     = {4};
            create.h5Layout = layouts[idx];

            auto path         = h5pp::format("createGroup/vectorDouble_{}", layout_names[idx]);
            auto created_info = file.dataset(path).create(create).getInfo();
            REQUIRE(created_info.dsetPath.value() == path);
            require_zero_vector(file, path);
        }
    }

    std::vector<double> data = {1, 2, 3, 4};

    h5pp::DatasetCreateOptions chunked_create;
    chunked_create.h5Type    = H5Tcopy(H5T_NATIVE_DOUBLE);
    chunked_create.dims      = {4};
    chunked_create.h5Layout  = H5D_CHUNKED;
    chunked_create.dimsChunk = {2};
    chunked_create.dimsMax   = {H5S_UNLIMITED};

    file.writeDataset(data, "manual/writeV1Api", chunked_create);
    auto v1_info = file.getDatasetInfo("manual/writeV1Api");
    REQUIRE(v1_info.h5Layout.value() == H5D_CHUNKED);
    REQUIRE(v1_info.dsetChunk.value() == std::vector<hsize_t>{2});
    REQUIRE(v1_info.dsetDimsMax.value() == std::vector<hsize_t>{H5S_UNLIMITED});
    REQUIRE(file.readDataset<std::vector<double>>("manual/writeV1Api") == data);

    h5pp::DatasetCreateOptions typed_create;
    typed_create.h5Type   = H5Tcopy(H5T_NATIVE_DOUBLE);
    typed_create.dims     = {4};
    typed_create.h5Layout = H5D_CONTIGUOUS;
    file.dataset("manual/writeV2Api").create(data, typed_create).write(data);
    REQUIRE(file.dataset("manual/writeV2Api").getInfo().h5Layout.value() == H5D_CONTIGUOUS);
    REQUIRE(file.readDataset<std::vector<double>>("manual/writeV2Api") == data);

    h5pp::DatasetWriteOptions write;
    write.resizePolicy = h5pp::ResizePolicy::FIT;
    file.writeDataset(std::vector<double>{5, 6}, "manual/writeV1Api", write);
    REQUIRE(file.readDataset<std::vector<double>>("manual/writeV1Api") == std::vector<double>{5, 6});
    REQUIRE(file.dataset("manual/writeV1Api").getInfo().dsetDims.value() == std::vector<hsize_t>{2});

    std::vector<double> buffer;
    file.readDataset(buffer, "manual/writeV1Api");
    REQUIRE(buffer == std::vector<double>{5, 6});
    file.writeAttribute(std::string("m"), "manual/writeV1Api", "unit");
    REQUIRE(file.readAttribute<std::string>("manual/writeV1Api", "unit") == "m");
    REQUIRE(file.linkExists("manual/writeV1Api"));
    REQUIRE(file.attributeExists("manual/writeV1Api", "unit"));
}

TEST_CASE("v1 table copying uses source metadata and explicit creation options", "[v1][v2][table]") {
    h5pp::File     file(make_path("tableApi"), h5pp::FileAccess::REPLACE, 0);
    h5pp::hid::h5t rowType = H5Tcreate(H5T_COMPOUND, sizeof(int));
    REQUIRE(H5Tinsert(rowType, "value", 0, H5T_NATIVE_INT) >= 0);
    const std::vector<int> values = {1, 2, 3};
    file.table("source").create(rowType, "source rows").appendRecords(values);
    h5pp::TableCreateOptions options;
    options.dimsChunk   = {2};
    options.compression = 1;
    auto copied         = file.appendTableRecords(file.openFileHandle(), "source", "copy", h5pp::TableSelection::ALL, options);
    CHECK(copied.tableTitle.value() == "source rows");
    CHECK(copied.chunkDims.value() == std::vector<hsize_t>{2});
    CHECK(h5pp::hdf5::getDeflateLevel(copied.h5DsetCreate.value()) > 0);
    CHECK(file.table("copy").readRecords<std::vector<int>>() == values);
}

TEMPLATE_TEST_CASE("Basic v1 attribute writes work unchanged in v2", "[v1][v2][attribute]", h5pp::v1::File, h5pp::File) {
    TestType file(make_path("attributeCompatibility"), h5pp::FileAccess::REPLACE, 0);
    file.writeDataset(std::vector<double>{1, 2, 3, 4}, "myStdVector");
    file.writeAttribute("m/s", "myStdVector", "units");
    CHECK(file.template readAttribute<std::string>("myStdVector", "units") == "m/s");
    file.writeAttribute(std::string("kg"), "myStdVector", "units");
    CHECK(file.template readAttribute<std::string>("myStdVector", "units") == "kg");
    file.writeAttribute(42, "myStdVector", "answer");
    CHECK(file.template readAttribute<int>("myStdVector", "answer") == 42);
    const std::vector<double> values{1, 2, 3, 4};
    file.writeAttribute(values, "myStdVector", "values");
    CHECK(file.template readAttribute<std::vector<double>>("myStdVector", "values") == values);
}

int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
