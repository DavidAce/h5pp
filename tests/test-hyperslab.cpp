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

    auto readBack = file.dataset("data5x5").read<std::vector<double>>();
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

    auto readBack = file.dataset("data5x5").read<std::vector<double>>();
    REQUIRE(readBack[7] == Catch::Approx(1.0));
    REQUIRE(readBack[8] == Catch::Approx(2.0));
    REQUIRE(readBack[12] == Catch::Approx(3.0));
    REQUIRE(readBack[13] == Catch::Approx(4.0));
}

TEST_CASE("Hyperslabs describe scalar, all and empty dataspace selections", "[hyperslab]") {
    SECTION("scalar") {
        h5pp::hid::h5s  space = H5Screate(H5S_SCALAR);
        h5pp::Hyperslab slab(space);
        CHECK(slab.empty());
        CHECK(slab.string().empty());
    }
    SECTION("all elements") {
        const hsize_t   dims[] = {4, 6};
        h5pp::hid::h5s  space  = H5Screate_simple(2, dims, nullptr);
        h5pp::Hyperslab slab(space);
        CHECK(slab.select_type == H5S_SEL_ALL);
        REQUIRE(slab.offset.has_value());
        REQUIRE(slab.extent.has_value());
        CHECK(slab.offset.value() == std::vector<hsize_t>{0, 0});
        CHECK(slab.extent.value() == std::vector<hsize_t>{4, 6});
        CHECK_FALSE(slab.empty());
        CHECK(slab.string(false).empty());
    }
    SECTION("no elements") {
        const hsize_t  dims[] = {4, 6};
        h5pp::hid::h5s space  = H5Screate_simple(2, dims, nullptr);
        REQUIRE(H5Sselect_none(space) >= 0);
        h5pp::Hyperslab slab(space);
        CHECK(slab.select_type == H5S_SEL_NONE);
        CHECK(slab.empty());
    }
}

#if H5_VERSION_GE(1, 10, 0)
TEST_CASE("Regular hyperslabs preserve offsets, counts, strides and blocks", "[hyperslab]") {
    const hsize_t  dims[]   = {8, 10};
    const hsize_t  offset[] = {1, 1};
    const hsize_t  stride[] = {3, 4};
    const hsize_t  count[]  = {2, 2};
    const hsize_t  blocks[] = {2, 3};
    h5pp::hid::h5s space    = H5Screate_simple(2, dims, nullptr);
    REQUIRE(H5Sselect_hyperslab(space, H5S_SELECT_SET, offset, stride, count, blocks) >= 0);
    h5pp::Hyperslab slab(space);
    CHECK(slab.select_type == H5S_SEL_HYPERSLABS);
    REQUIRE(slab.offset.has_value());
    REQUIRE(slab.extent.has_value());
    REQUIRE(slab.stride.has_value());
    REQUIRE(slab.blocks.has_value());
    CHECK(slab.offset.value() == std::vector<hsize_t>{1, 1});
    CHECK(slab.extent.value() == std::vector<hsize_t>{2, 2});
    CHECK(slab.stride.value() == std::vector<hsize_t>{3, 4});
    CHECK(slab.blocks.value() == std::vector<hsize_t>{2, 3});
    CHECK_THAT(slab.string(), Catch::Matchers::ContainsSubstring("stride"));
    CHECK_THAT(slab.string(), Catch::Matchers::ContainsSubstring("blocks"));

    h5pp::hid::h5s restored = H5Screate_simple(2, dims, nullptr);
    slab.select_oper        = H5S_SELECT_OR;
    slab.applySelection(restored);
    CHECK(H5Sget_select_npoints(restored) == 24);
    CHECK(H5Sselect_shape_same(space, restored) > 0);
    hsize_t first[2], last[2];
    REQUIRE(H5Sget_select_bounds(restored, first, last) >= 0);
    CHECK(std::vector<hsize_t>(first, first + 2) == std::vector<hsize_t>{1, 1});
    CHECK(std::vector<hsize_t>(last, last + 2) == std::vector<hsize_t>{5, 7});
}

#endif

TEST_CASE("Hyperslabs reject point and irregular selections", "[hyperslab][validation]") {
    const hsize_t  dims[] = {8, 10};
    h5pp::hid::h5s space  = H5Screate_simple(2, dims, nullptr);
    SECTION("point selection") {
        const hsize_t points[][2] = {
            {0, 0},
            {2, 3}
        };
        REQUIRE(H5Sselect_elements(space, H5S_SELECT_SET, 2, points[0]) >= 0);
        REQUIRE_THROWS_WITH(h5pp::Hyperslab(space), Catch::Matchers::ContainsSubstring("Unsupported selection type"));
    }
#if H5_VERSION_GE(1, 10, 0)
    SECTION("irregular hyperslab") {
        const hsize_t first[]  = {0, 0};
        const hsize_t second[] = {2, 3};
        const hsize_t count[]  = {1, 1};
        REQUIRE(H5Sselect_hyperslab(space, H5S_SELECT_SET, first, nullptr, count, nullptr) >= 0);
        REQUIRE(H5Sselect_hyperslab(space, H5S_SELECT_OR, second, nullptr, count, nullptr) >= 0);
        REQUIRE(H5Sis_regular_hyperslab(space) == 0);
        REQUIRE_THROWS_WITH(h5pp::Hyperslab(space), Catch::Matchers::ContainsSubstring("irregular"));
    }
#endif
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    return session.run(argc, argv);
}
