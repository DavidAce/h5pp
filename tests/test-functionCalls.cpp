#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <string_view>
#include <vector>

namespace {
    h5pp::File make_file() { return h5pp::File("output/functionCalls.h5", h5pp::FileAccess::REPLACE, 0); }

    const std::vector<std::optional<H5D_layout_t>> layouts      = {std::nullopt, H5D_COMPACT, H5D_CONTIGUOUS, H5D_CHUNKED};
    const std::vector<std::string>                 layout_names = {"auto", "compact", "contiguous", "chunked"};

    void require_zero_vector(h5pp::File &file, std::string_view path) {
        REQUIRE(file.readDataset<std::vector<double>>(path) == std::vector<double>{0, 0, 0, 0});
    }
}

TEST_CASE("createDataset accepts options-only and inferred signatures across layouts", "[function-calls][create]") {
    auto file = make_file();

    h5pp::Options options;
    options.h5Type   = H5Tcopy(H5T_NATIVE_DOUBLE);
    options.dataDims = {4};

    std::vector<double> data = {1, 2, 3, 4};
    for(size_t idx = 0; idx < layouts.size(); idx++) {
        SECTION(h5pp::format("options only: {}", layout_names[idx])) {
            auto path         = h5pp::format("optionsCreateGroup/vectorDouble_{}", layout_names[idx]);
            options.linkPath  = path;
            options.h5Layout  = layouts[idx];
            auto created_info = file.createDataset(options);
            REQUIRE(created_info.dsetPath.value() == path);
            require_zero_vector(file, path);
        }

        SECTION(h5pp::format("inferred from data: {}", layout_names[idx])) {
            auto path         = h5pp::format("inferCreateGroup/vectorDouble_{}", layout_names[idx]);
            options.linkPath  = path;
            options.h5Layout  = layouts[idx];
            auto created_info = file.createDataset(data, options);
            REQUIRE(created_info.dsetPath.value() == path);
            require_zero_vector(file, path);
        }
    }
}

TEST_CASE("createDataset and writeDataset support manual overload combinations", "[function-calls][overloads]") {
    auto file = make_file();

    std::vector<double> data = {1, 2, 3, 4};

    auto inferred = file.createDataset(data, "manual/inferred");
    REQUIRE(inferred.dsetDims.value() == std::vector<hsize_t>{4});
    require_zero_vector(file, "manual/inferred");

    auto chunked = file.createDataset(data, "manual/chunked", {4}, H5D_CHUNKED, {2}, {H5S_UNLIMITED}, 3);
    REQUIRE(chunked.h5Layout.value() == H5D_CHUNKED);
    REQUIRE(chunked.dsetChunk.value() == std::vector<hsize_t>{2});
    REQUIRE(chunked.dsetDimsMax.value() == std::vector<hsize_t>{H5S_UNLIMITED});
    require_zero_vector(file, "manual/chunked");

    h5pp::hid::h5t create_type = H5Tcopy(H5T_NATIVE_DOUBLE);
    auto           typed       = file.createDataset("manual/typed", create_type, H5D_CONTIGUOUS, {4});
    REQUIRE(typed.h5Layout.value() == H5D_CONTIGUOUS);
    require_zero_vector(file, "manual/typed");

    auto written_layout_first = file.writeDataset(data, "manual/writeLayoutFirst", H5D_CHUNKED, {4}, {2}, {H5S_UNLIMITED});
    REQUIRE(written_layout_first.h5Layout.value() == H5D_CHUNKED);
    REQUIRE(file.readDataset<std::vector<double>>("manual/writeLayoutFirst") == data);

    h5pp::hid::h5t type               = H5Tcopy(H5T_NATIVE_DOUBLE);
    auto           written_type_first = file.writeDataset(data, "manual/writeTypeFirst", type, {4}, H5D_CONTIGUOUS);
    REQUIRE(written_type_first.h5Layout.value() == H5D_CONTIGUOUS);
    REQUIRE(file.readDataset<std::vector<double>>("manual/writeTypeFirst") == data);

    auto compact = file.writeDataset_compact(data, "manual/writeCompact");
    auto chunk   = file.writeDataset_chunked(data, "manual/writeChunked", {4}, {2}, {H5S_UNLIMITED}, std::nullopt, 1);
    REQUIRE(compact.h5Layout.value() == H5D_COMPACT);
    REQUIRE(chunk.h5Layout.value() == H5D_CHUNKED);
    REQUIRE(file.readDataset<std::vector<double>>("manual/writeCompact") == data);
    REQUIRE(file.readDataset<std::vector<double>>("manual/writeChunked") == data);
}

TEST_CASE("writeDataset via Options preserves resize policy and writes through", "[function-calls][options-write]") {
    auto file = make_file();

    std::vector<double> initial = {1, 2, 3, 4};
    std::vector<double> updated = {5, 6, 7, 8};

    h5pp::Options options;
    options.linkPath      = "optionsWrite/vector";
    options.dataDims      = {4};
    options.h5Layout      = H5D_CHUNKED;
    options.dsetChunkDims = {2};
    options.dsetMaxDims   = {H5S_UNLIMITED};
    options.resizePolicy  = h5pp::ResizePolicy::FIT;

    auto written_info = file.writeDataset(initial, options);
    REQUIRE(written_info.h5Layout.value() == H5D_CHUNKED);
    REQUIRE(file.readDataset<std::vector<double>>("optionsWrite/vector") == initial);

    file.writeDataset(updated, options);
    REQUIRE(file.readDataset<std::vector<double>>("optionsWrite/vector") == updated);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
