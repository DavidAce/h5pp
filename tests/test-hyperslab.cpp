#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <string>
#include <string_view>
#include <vector>

namespace {
    std::string make_path(std::string_view name) {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }
}

TEST_CASE("hyperslab write", "[hyperslab][write]") {
    h5pp::File file(make_path("hyperslab"), h5pp::FileAccess::REPLACE, 0);

    std::vector<double> data5x5(25, 0.0);
    h5pp::DatasetWriteOptions asMatrix;
    asMatrix.dims = {5, 5};
    file.dataset("data5x5").write(data5x5, asMatrix);

    std::vector<double> patchData = {1.0, 2.0, 3.0, 4.0};
    h5pp::DatasetWriteOptions patch;
    patch.dims = {2, 2};
    file.dataset("data5x5").select({1, 2}, {2, 2}).write(patchData, patch);

    auto readBack = file.readDataset<std::vector<double>>("data5x5");
    REQUIRE(readBack[7] == Catch::Approx(1.0));
    REQUIRE(readBack[8] == Catch::Approx(2.0));
    REQUIRE(readBack[12] == Catch::Approx(3.0));
    REQUIRE(readBack[13] == Catch::Approx(4.0));
}

TEST_CASE("hyperslab read", "[hyperslab][read]") {
    h5pp::File file(make_path("hyperslab-read"), h5pp::FileAccess::REPLACE, 0);

    std::vector<double> data5x5(25, 0.0);
    for(size_t idx = 0; idx < data5x5.size(); ++idx) data5x5[idx] = static_cast<double>(idx);

    h5pp::DatasetWriteOptions asMatrix;
    asMatrix.dims = {5, 5};
    file.dataset("data5x5").write(data5x5, asMatrix);

    auto readPatch = file.dataset("data5x5").select({1, 2}, {2, 2}).read<std::vector<double>>();
    REQUIRE(readPatch == std::vector<double>{7.0, 8.0, 12.0, 13.0});
}

TEST_CASE("hyperslab write with explicit data slab", "[hyperslab][data-slab]") {
    h5pp::File file(make_path("hyperslab-data-slab"), h5pp::FileAccess::REPLACE, 0);

    std::vector<double> data5x5(25, 0.0);
    h5pp::DatasetWriteOptions asMatrix;
    asMatrix.dims = {5, 5};
    file.dataset("data5x5").write(data5x5, asMatrix);

    std::vector<double> data3x3 = {
        0.0, 0.0, 0.0,
        0.0, 1.0, 2.0,
        0.0, 3.0, 4.0,
    };

    h5pp::DatasetWriteOptions patch;
    patch.dims     = {3, 3};
    patch.dataSlab = h5pp::Hyperslab({1, 1}, {2, 2});
    file.dataset("data5x5").select({1, 2}, {2, 2}).write(data3x3, patch);

    auto readBack = file.readDataset<std::vector<double>>("data5x5");
    REQUIRE(readBack[7] == Catch::Approx(1.0));
    REQUIRE(readBack[8] == Catch::Approx(2.0));
    REQUIRE(readBack[12] == Catch::Approx(3.0));
    REQUIRE(readBack[13] == Catch::Approx(4.0));
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    return session.run(argc, argv);
}
