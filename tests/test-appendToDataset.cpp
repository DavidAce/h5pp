#include <array>
#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/h5pp.h>
#include <string_view>

namespace {
    void require_dims(const h5pp::DsetInfo &info, const std::vector<hsize_t> &expected) {
        REQUIRE(info.dsetDims.has_value());
        REQUIRE(info.dsetDims.value() == expected);
    }

    void require_max_dims(const h5pp::DsetInfo &info, const std::vector<hsize_t> &expected) {
        REQUIRE(info.dsetDimsMax.has_value());
        REQUIRE(info.dsetDimsMax.value() == expected);
    }

    void require_chunked_layout(const h5pp::DsetInfo &info) {
        REQUIRE(info.h5Layout.has_value());
        REQUIRE(info.h5Layout.value() == H5D_CHUNKED);
    }

    h5pp::File make_file(std::string_view name) { return h5pp::File(h5pp::format("output/{}.h5", name), h5pp::FileAccess::REPLACE, 0); }
}

TEST_CASE("Legacy append smoke test now checks behavior", "[append][legacy]") {
    auto file = make_file("appendToDataset-legacy");

    std::vector<double> data = {1, 2, 3, 4};
    file.writeDataset(data, "group/VectorDoubletemp", {4, 1}, H5D_CHUNKED, {4, 100});
    file.writeDataset(data, "group/VectorDoubletemp2", {});
    file.writeDataset(data, "group/VectorDouble0");
    file.writeDataset(data, "group/VectorDouble1", {4});
    file.writeDataset(data, "group/VectorDouble5", {4, 1}, H5D_CHUNKED, {4, 100}, {4, H5S_UNLIMITED});
    file.writeDataset(data, "group/VectorDouble6", {});
    file.writeDataset(data, "group/VectorDouble7", {}, {});

    require_chunked_layout(file.getDatasetInfo("group/VectorDoubletemp"));
    require_dims(file.getDatasetInfo("group/VectorDoubletemp"), {4, 1});
    require_chunked_layout(file.getDatasetInfo("group/VectorDouble5"));
    require_dims(file.getDatasetInfo("group/VectorDouble5"), {4, 1});
    require_max_dims(file.getDatasetInfo("group/VectorDouble5"), {4, H5S_UNLIMITED});

    REQUIRE(file.readDataset<std::vector<double>>("group/VectorDoubletemp2") == data);
    REQUIRE(file.readDataset<std::vector<double>>("group/VectorDouble0") == data);
    REQUIRE(file.readDataset<std::vector<double>>("group/VectorDouble1") == data);
    REQUIRE(file.readDataset<std::vector<double>>("group/VectorDouble6") == data);
    REQUIRE(file.readDataset<std::vector<double>>("group/VectorDouble7") == data);

    auto append_info = file.appendToDataset(std::vector<double>{5, 6, 7, 8}, "group/VectorDouble5", 1);
    require_dims(append_info, {4, 2});
    append_info = file.appendToDataset(std::vector<double>{9, 10, 11, 12}, "group/VectorDouble5", 1);
    require_dims(append_info, {4, 3});

    REQUIRE(file.readDataset<std::vector<double>>("group/VectorDouble5") == std::vector<double>{1, 5, 9, 2, 6, 10, 3, 7, 11, 4, 8, 12});

    auto seed_info = file.getDatasetInfo("group/VectorDouble5");
    file.createDataset("group/VectorDouble8", seed_info.h5Type.value(), H5D_CHUNKED, {data.size(), 0});
    file.createDataset("group/VectorDouble8_alt", seed_info.h5Type.value(), H5D_CHUNKED, {data.size(), 0});

    auto info8     = file.appendToDataset(data, "group/VectorDouble8", 1, {data.size(), 1});
    auto info8_alt = file.appendToDataset(data, "group/VectorDouble8_alt", 1, {data.size(), 1});
    require_dims(info8, {4, 1});
    require_dims(info8_alt, {4, 1});
    REQUIRE(file.readDataset<std::vector<double>>("group/VectorDouble8") == data);
    REQUIRE(file.readDataset<std::vector<double>>("group/VectorDouble8_alt") == data);
}

TEST_CASE("Append handles more types and more contrived growth patterns", "[append][types]") {
    SECTION("Append rows into a zero-sized leading axis") {
        auto           file    = make_file("appendToDataset-rows");
        h5pp::hid::h5t h5_type = H5Tcopy(H5T_NATIVE_DOUBLE);
        file.createDataset("group/rows", h5_type, H5D_CHUNKED, {0, 3}, {1, 3}, {H5S_UNLIMITED, 3});

        auto info = file.appendToDataset(std::vector<double>{1, 2, 3}, "group/rows", 0, {1, 3});
        require_dims(info, {1, 3});
        info = file.appendToDataset(std::vector<double>{4, 5, 6}, "group/rows", 0, {1, 3});
        require_dims(info, {2, 3});

        REQUIRE(file.readDataset<std::vector<double>>("group/rows") == std::vector<double>{1, 2, 3, 4, 5, 6});
    }

    SECTION("Append complex values into a 1d unlimited dataset") {
        auto file = make_file("appendToDataset-complex");

        std::vector<std::complex<double>> initial = {
            {1.0, 2.0},
            {3.0, 4.0}
        };
        std::vector<std::complex<double>> more = {
            { 5.0, -1.0},
            {-2.0,  8.0}
        };
        std::vector<std::complex<double>> expect = initial;
        expect.insert(expect.end(), more.begin(), more.end());

        file.writeDataset(initial, "group/complex", {initial.size()}, H5D_CHUNKED, {initial.size()}, {H5S_UNLIMITED});
        auto info = file.appendToDataset(more, "group/complex", 0);
        require_dims(info, {expect.size()});

        REQUIRE(file.readDataset<std::vector<std::complex<double>>>("group/complex") == expect);
    }

    SECTION("Append fixed-size integer arrays into a 1d unlimited dataset") {
        auto file = make_file("appendToDataset-integers");

        std::vector<long long>   initial = {10, 20};
        std::array<long long, 3> more    = {30, 40, 50};
        std::vector<long long>   expect  = initial;
        expect.insert(expect.end(), more.begin(), more.end());

        file.writeDataset(initial, "group/integers", {initial.size()}, H5D_CHUNKED, {initial.size()}, {H5S_UNLIMITED});
        auto info = file.appendToDataset(more, "group/integers", 0, {more.size()});
        require_dims(info, {expect.size()});

        REQUIRE(file.readDataset<std::vector<long long>>("group/integers") == expect);
    }
}

TEST_CASE("Append rejects invalid resize scenarios", "[append][errors]") {
    auto file = make_file("appendToDataset-errors");

    file.writeDataset(std::vector<double>{1, 2, 3, 4}, "group/fixed", {4}, H5D_CONTIGUOUS);
    REQUIRE_THROWS(file.appendToDataset(std::vector<double>{5, 6}, "group/fixed", 0));

    file.writeDataset(std::vector<double>{1, 2, 3, 4}, "group/columns", {4, 1}, H5D_CHUNKED, {4, 1}, {4, H5S_UNLIMITED});
    REQUIRE_THROWS(file.appendToDataset(std::vector<double>{10, 11, 12, 13, 14}, "group/columns", 1, {5, 1}));
    REQUIRE_THROWS(file.appendToDataset(std::vector<double>{20, 21}, "group/columns", 0, {2, 1}));
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
