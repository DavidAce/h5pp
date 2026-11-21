#include <catch2/catch_all.hpp>
#include <h5pp/v1/h5pp.h>
#include <numeric>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::format(H5PP_TEST_DIR "v1-hyperslab-{}.h5", name);
    }
}

TEST_CASE("v1 writes flat buffers into multidimensional selections without resizing", "[hyperslab][write]") {
    auto           layout = GENERATE(H5D_COMPACT, H5D_CONTIGUOUS, H5D_CHUNKED);
    h5pp::v1::File file(make_path("write"), h5pp::FileAccess::REPLACE, 0);
    h5pp::Options  options;
    options.linkPath     = "matrix";
    options.dataDims     = {5, 5};
    options.h5Layout     = layout;
    options.resizePolicy = h5pp::ResizePolicy::OFF;
    auto            info = file.writeDataset(std::vector<int>(25, 0), options);
    h5pp::Hyperslab slab({1, 2}, {2, 2});
    SECTION("writeHyperslab") { file.writeHyperslab(std::vector<int>{1, 2, 3, 4}, "matrix", slab); }
    SECTION("Reused dataset metadata") {
        info.dsetSlab = slab;
        file.writeDataset(std::vector<int>{1, 2, 3, 4}, info);
    }
    std::vector<int> expected(25, 0);
    expected[7]  = 1;
    expected[8]  = 2;
    expected[12] = 3;
    expected[13] = 4;
    CHECK(file.readDataset<std::vector<int>>("matrix") == expected);
    CHECK(file.getDatasetInfo("matrix").dsetDims.value() == std::vector<hsize_t>{5, 5});
    CHECK_THROWS(file.writeHyperslab(std::vector<int>{1, 2, 3}, "matrix", slab));
    CHECK(file.readDataset<std::vector<int>>("matrix") == expected);
}

TEST_CASE("v1 reads only the selected elements into new and existing buffers", "[hyperslab][read]") {
    h5pp::v1::File   file(make_path("read"), h5pp::FileAccess::REPLACE, 0);
    std::vector<int> values(25);
    std::iota(values.begin(), values.end(), 0);
    auto                   info = file.writeDataset(values, "matrix", {5, 5});
    h5pp::Hyperslab        slab({3, 1}, {2, 4});
    const std::vector<int> expected{16, 17, 18, 19, 21, 22, 23, 24};
    CHECK(file.readHyperslab<std::vector<int>>("matrix", slab) == expected);
    std::vector<int> buffer(25, -1);
    file.readHyperslab(buffer, "matrix", slab);
    CHECK(buffer == expected);
    file.readHyperslab(buffer, "matrix", slab);
    CHECK(buffer == expected);
    info.dsetSlab = slab;
    CHECK(file.readDataset<std::vector<int>>(info) == expected);
#ifdef H5PP_USE_EIGEN3
    auto matrix = file.readHyperslab<Eigen::MatrixXi>("matrix", slab);
    REQUIRE(matrix.rows() == 2);
    REQUIRE(matrix.cols() == 4);
    CHECK(matrix(0, 0) == 16);
    CHECK(matrix(1, 3) == 24);
#endif
}

TEST_CASE("v1 packs strided hyperslab blocks without gaps", "[hyperslab][read]") {
    h5pp::v1::File   file(make_path("blocks"), h5pp::FileAccess::REPLACE, 0);
    std::vector<int> values(64);
    std::iota(values.begin(), values.end(), 0);
    file.writeDataset(values, "matrix", {8, 8});
    h5pp::Hyperslab slab({1, 0}, {2, 2}, {3, 3}, {1, 2});
    CHECK(file.readHyperslab<std::vector<int>>("matrix", slab) == std::vector<int>{8, 9, 11, 12, 32, 33, 35, 36});
#ifdef H5PP_USE_EIGEN3
    auto matrix = file.readHyperslab<Eigen::MatrixXi>("matrix", slab);
    REQUIRE(matrix.rows() == 2);
    REQUIRE(matrix.cols() == 4);
    CHECK(matrix(0, 2) == 11);
    CHECK(matrix(1, 3) == 36);
#endif
}

int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
