#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <numeric>
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

    std::vector<double>       data5x5(25, 0.0);
    h5pp::DatasetWriteOptions asMatrix;
    asMatrix.dims = {5, 5};
    file.dataset("data5x5").write(data5x5, asMatrix);

    std::vector<double>       patchData = {1.0, 2.0, 3.0, 4.0};
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

    std::vector<double>       data5x5(25, 0.0);
    h5pp::DatasetWriteOptions asMatrix;
    asMatrix.dims = {5, 5};
    file.dataset("data5x5").write(data5x5, asMatrix);

    std::vector<double> data3x3 = {
        0.0,
        0.0,
        0.0,
        0.0,
        1.0,
        2.0,
        0.0,
        3.0,
        4.0,
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

TEST_CASE("Selected variable-length strings are packed into the read buffer", "[hyperslab][text]") {
    h5pp::File               file(make_path("hyperslab-strings"), h5pp::FileAccess::REPLACE, 0);
    std::vector<std::string> values{"zero", "one", "two", "three", "four", "five"};
    file.dataset("strings").write(values);
    CHECK(file.dataset("strings").select({4}, {2}).read<std::vector<std::string>>() == std::vector<std::string>{"four", "five"});
    h5pp::DatasetWriteOptions options;
    options.dims = {2, 3};
    file.dataset("matrix").write(values, options);
    CHECK(file.dataset("matrix").select({0, 1}, {2, 2}).read<std::vector<std::string>>() ==
          std::vector<std::string>{"one", "two", "four", "five"});
}

static_assert(std::is_constructible_v<h5pp::Dataset, h5pp::File &, std::string_view>);
static_assert(not std::is_constructible_v<h5pp::Dataset, h5pp::File &, std::string_view, h5pp::Hyperslab>);

namespace {
    template<typename Dataset>
    concept CanWriteMemorySlab = requires(Dataset dataset, std::vector<int> data, h5pp::Hyperslab slab) { dataset.write(data, slab); };
    static_assert(CanWriteMemorySlab<h5pp::Dataset>);
    static_assert(not CanWriteMemorySlab<h5pp::ConstDataset>);
}

TEST_CASE("File and memory selections can each be omitted", "[hyperslab][data-slab]") {
    h5pp::File file(make_path("hyperslab-optional-selections"), h5pp::FileAccess::REPLACE, 0);
    auto       dataset = file.dataset("values");
    dataset.write(std::vector<int>(4, 0), {});
    std::vector<int> expected;
    SECTION("Neither selection") {
        dataset.write(std::vector<int>{1, 2, 3, 4});
        expected = {1, 2, 3, 4};
    }
    SECTION("File selection only") {
        dataset.select({1}, {2}).write(std::vector<int>{3, 4});
        expected = {0, 3, 4, 0};
    }
    SECTION("Memory selection only") {
        const h5pp::Hyperslab slab({1}, {4});
        dataset.write(std::vector<int>{0, 1, 2, 3, 4, 5}, slab);
        expected = {1, 2, 3, 4};
    }
    SECTION("Both selections") {
        dataset.select({1}, {2}).write(std::vector<int>{0, 1, 2, 3, 4, 5}, h5pp::Hyperslab({3}, {2}));
        expected = {0, 3, 4, 0};
    }
    CHECK(dataset.read<std::vector<int>>() == expected);
    std::vector<int> result(8, -1);
    dataset.readInto(result, h5pp::Hyperslab({2}, {4}));
    CHECK(result.size() == 8);
    CHECK(std::vector<int>(result.begin() + 2, result.begin() + 6) == expected);
    CHECK(result[0] == -1);
    CHECK(result[1] == -1);
    CHECK(result[6] == -1);
    CHECK(result[7] == -1);
    dataset.readInto(result, {});
    CHECK(result == expected);
}

TEST_CASE("Memory dimensions remain independent of the dataset selection", "[hyperslab][data-slab]") {
    const auto layout = GENERATE(H5D_COMPACT, H5D_CHUNKED);
    h5pp::File file(make_path("hyperslab-buffer-shape"), h5pp::FileAccess::REPLACE, 0);
    auto       dataset = file.dataset("matrix");
    dataset
        .create(std::vector<int>(25, 0),
                {
                    .dims     = h5pp::Dims{5, 5},
                    .h5Layout = layout
    })
        .write(std::vector<int>(25, 0), {.dims = h5pp::Dims{5, 5}});
    std::vector<int> input(100);
    std::iota(input.begin(), input.end(), 0);
    h5pp::Hyperslab source({3, 4}, {2, 2});
    auto            selection = dataset.select({1, 2}, {2, 2});
    CHECK_THROWS(selection.write(input, source));
    selection.write(input,
                    source,
                    {
                        .dims = h5pp::Dims{10, 10}
    });
    CHECK(selection.read<std::vector<int>>() == std::vector<int>{34, 35, 44, 45});
    std::vector<int> output(80, -1);
    selection.readInto(output,
                       h5pp::Hyperslab(
                           {
                               4,
                               5
    },
                           {2, 2}),
                       {.dims = h5pp::Dims{8, 10}});
    std::vector<int> expected(80, -1);
    expected[45] = 34;
    expected[46] = 35;
    expected[55] = 44;
    expected[56] = 45;
    CHECK(output == expected);
    const auto &readonly = file;
    std::fill(output.begin(), output.end(), -1);
    readonly.dataset("matrix")
        .select(
            {
                1,
                2
    },
            {2, 2})
        .readInto(output, h5pp::Hyperslab({4, 5}, {2, 2}), {.dims = h5pp::Dims{8, 10}});
    CHECK(output == expected);
    CHECK(dataset.read<std::vector<int>>().size() == 25);
}

TEST_CASE("Selections with different ranks transfer equal element counts", "[hyperslab][data-slab]") {
    const auto layout = GENERATE(H5D_COMPACT, H5D_CHUNKED);
    h5pp::File file(make_path("hyperslab-different-ranks"), h5pp::FileAccess::REPLACE, 0);
    auto       dataset = file.dataset("matrix").create(std::vector<int>(20, 0),
                                                       {
                                                           .dims     = h5pp::Dims{4, 5},
                                                           .h5Layout = layout
    });
    dataset.write(std::vector<int>(20, 0),
                  {
                      .dims = h5pp::Dims{4, 5}
    });
    auto selection = dataset.select({1, 2}, {2, 2});
    selection.write(std::vector<int>{0, 1, 2, 3, 4, 5}, h5pp::Hyperslab({1}, {4}));
    CHECK(selection.read<std::vector<int>>() == std::vector<int>{1, 2, 3, 4});
    std::vector<int> output(8, -1);
    selection.readInto(output, h5pp::Hyperslab({3}, {4}));
    CHECK(output == std::vector<int>{-1, -1, -1, 1, 2, 3, 4, -1});
    CHECK_THROWS(selection.write(std::vector<int>{1, 2, 3, 4, 5}));
    CHECK_THROWS(selection.write(std::vector<int>{1, 2, 3}, h5pp::Hyperslab({0}, {3})));
}

TEST_CASE("select returns independent copies and replaces earlier selections", "[hyperslab]") {
    h5pp::File file(make_path("hyperslab-copies"), h5pp::FileAccess::REPLACE, 0);
    auto       dataset = file.dataset("values");
    dataset.write(std::vector<int>{0, 1, 2, 3, 4, 5});
    auto first  = dataset.select({1}, {2});
    auto second = first.select({4}, {2});
    CHECK(dataset.read<std::vector<int>>() == std::vector<int>{0, 1, 2, 3, 4, 5});
    CHECK(first.read<std::vector<int>>() == std::vector<int>{1, 2});
    CHECK(second.read<std::vector<int>>() == std::vector<int>{4, 5});
}

TEST_CASE("Memory selections work with raw pointers and explicit dimensions", "[hyperslab][data-slab]") {
    h5pp::File file(make_path("hyperslab-pointers"), h5pp::FileAccess::REPLACE, 0);
    auto       dataset = file.dataset("values");
    dataset.write(std::vector<int>(4, 0));
    int             input[]{0, 1, 2, 3, 4, 5};
    const int      *source = input;
    h5pp::Hyperslab slab({1}, {4});
    CHECK_THROWS(dataset.write(source, slab));
    dataset.write(source, slab, {.dims = h5pp::Dims{6}});
    int  output[]{-1, -1, -1, -1, -1, -1};
    int *destination = output;
    CHECK_THROWS(dataset.readInto(destination, slab));
    dataset.readInto(destination, slab, {.dims = h5pp::Dims{6}});
    CHECK(std::vector<int>(output, output + 6) == std::vector<int>{-1, 1, 2, 3, 4, -1});
}

TEST_CASE("Memory selections preserve string buffer elements outside the selection", "[hyperslab][data-slab][text]") {
    h5pp::File                file(make_path("hyperslab-memory-strings"), h5pp::FileAccess::REPLACE, 0);
    auto                      dataset = file.dataset("values");
    const auto                fixed   = GENERATE(false, true);
    h5pp::DatasetWriteOptions options;
    if(fixed) {
        h5pp::hid::h5t type = H5Tcopy(H5T_C_S1);
        REQUIRE(H5Tset_size(type, 8) >= 0);
        options.h5Type = type;
    }
    dataset.write(std::vector<std::string>{"first", "second"}, options);
    std::vector<std::string> output(5, "untouched");
    dataset.readInto(output, h5pp::Hyperslab({2}, {2}));
    CHECK(output == std::vector<std::string>{"untouched", "untouched", "first", "second", "untouched"});
    dataset.write(std::vector<std::string>{"unused", "updated", "", "unused"}, h5pp::Hyperslab({1}, {2}));
    dataset.readInto(output, h5pp::Hyperslab({2}, {2}));
    CHECK(output == std::vector<std::string>{"untouched", "untouched", "updated", "", "untouched"});
    auto                      nullable = file.dataset("nullable").create(std::vector<std::string>{"shape", "only"});
    std::vector<h5pp::vstr_t> strings(5, h5pp::vstr_t("untouched"));
    nullable.readInto(strings, h5pp::Hyperslab({2}, {2}));
    CHECK(std::string_view(strings[0]) == "untouched");
    CHECK(std::string_view(strings[1]) == "untouched");
    CHECK(strings[2].empty());
    CHECK(strings[3].empty());
    CHECK(std::string_view(strings[4]) == "untouched");
}

#ifdef H5PP_USE_EIGEN3
TEST_CASE("Memory selections use the full Eigen buffer shape", "[hyperslab][data-slab][eigen]") {
    h5pp::File                file(make_path("hyperslab-eigen-buffer"), h5pp::FileAccess::REPLACE, 0);
    const auto                flatFile = GENERATE(false, true);
    h5pp::DatasetWriteOptions options;
    options.dims = flatFile ? h5pp::Dims{4} : h5pp::Dims{2, 2};
    file.dataset("matrix").write(std::vector<int>(4, 0), options);
    Eigen::MatrixXi input(5, 6);
    for(Eigen::Index row = 0; row < input.rows(); ++row)
        for(Eigen::Index col = 0; col < input.cols(); ++col) input(row, col) = static_cast<int>(10 * row + col);
    file.dataset("matrix").write(input, h5pp::Hyperslab({2, 3}, {2, 2}));
    CHECK(file.dataset("matrix").read<std::vector<int>>() == std::vector<int>{23, 24, 33, 34});
    Eigen::MatrixXi output = Eigen::MatrixXi::Constant(6, 7, -1);
    file.dataset("matrix").readInto(output, h5pp::Hyperslab({3, 4}, {2, 2}));
    Eigen::MatrixXi expected   = Eigen::MatrixXi::Constant(6, 7, -1);
    expected.block(3, 4, 2, 2) = input.block(2, 3, 2, 2);
    CHECK(output == expected);
}
#endif

TEST_CASE("Selected writes grow in file coordinates independently of the buffer", "[hyperslab][data-slab]") {
    h5pp::File file(make_path("hyperslab-grow"), h5pp::FileAccess::REPLACE, 0);
    auto       dataset = file.dataset("values").create(std::vector<int>(4, 0), {.h5Layout = H5D_CHUNKED});
    dataset.write(std::vector<int>(4, 0));
    std::vector<int> input{0, 1, 2, 3, 4, 5, 6, 7, 8};
    dataset
        .select(
            {
                5
    },
            {4})
        .write(input, h5pp::Hyperslab({1, 1}, {2, 2}), {.dims = h5pp::Dims{3, 3}});
    CHECK(dataset.read<std::vector<int>>() == std::vector<int>{0, 0, 0, 0, 0, 4, 5, 7, 8});
    CHECK(dataset.getInfo().dsetDims.value() == std::vector<hsize_t>{9});
}

TEST_CASE("Strided dataset blocks use their packed shape for whole-buffer transfers", "[hyperslab][data-slab]") {
    h5pp::File file(make_path("hyperslab-file-blocks"), h5pp::FileAccess::REPLACE, 0);
    auto       dataset = file.dataset("matrix").create(std::vector<int>(18, 0),
                                                       {
                                                           .dims     = h5pp::Dims{3, 6},
                                                           .h5Layout = H5D_CHUNKED
    });
    dataset.write(std::vector<int>(18, 0),
                  {
                      .dims = h5pp::Dims{3, 6}
    });
    h5pp::Hyperslab slab({1, 1}, {1, 2}, {1, 3}, {2, 1});
    auto            selection = dataset.select(slab);
    selection.write(std::vector<int>{1, 2, 3, 4});
    CHECK(selection.read<std::vector<int>>() == std::vector<int>{1, 2, 3, 4});
    auto all = dataset.read<std::vector<int>>();
    CHECK(all[7] == 1);
    CHECK(all[10] == 2);
    CHECK(all[13] == 3);
    CHECK(all[16] == 4);
}

TEST_CASE("Explicit memory arguments override option selections and honor read-only access", "[hyperslab][data-slab]") {
    h5pp::File file(make_path("hyperslab-options"), h5pp::FileAccess::REPLACE, 0);
    auto       dataset = file.dataset("values");
    dataset.write(std::vector<int>(2, 0));
    h5pp::DatasetWriteOptions options;
    options.dataSlab = h5pp::Hyperslab({0}, {2});
    dataset.write(std::vector<int>{1, 2, 3, 4}, h5pp::Hyperslab({2}, {2}), options);
    CHECK(dataset.read<std::vector<int>>() == std::vector<int>{3, 4});
    std::vector<int>         output(4, -1);
    h5pp::DatasetReadOptions readOptions;
    readOptions.dataSlab = h5pp::Hyperslab({0}, {2});
    dataset.readInto(output, h5pp::Hyperslab({2}, {2}), readOptions);
    CHECK(output == std::vector<int>{-1, -1, 3, 4});
    h5pp::File readonly(file.getFilePath(), h5pp::FileAccess::READONLY, 0);
    CHECK_THROWS(readonly.dataset("values").write(output, h5pp::Hyperslab({0}, {2})));
    CHECK(dataset.read<std::vector<int>>() == std::vector<int>{3, 4});
}

TEST_CASE("Writes create missing datasets from the selected data", "[hyperslab][data-slab]") {
    h5pp::File       file(make_path("hyperslab-create-selected"), h5pp::FileAccess::REPLACE, 0);
    std::vector<int> input{0, 1, 2, 3, 4, 5};
    auto             dataset = file.dataset("values");
    SECTION("Memory selection only") {
        dataset.write(input, h5pp::Hyperslab({2}, {3}));
        CHECK(dataset.read<std::vector<int>>() == std::vector<int>{2, 3, 4});
    }
    SECTION("File selection only") {
        dataset.select({2}, {3}).write(std::vector<int>{2, 3, 4});
        CHECK(dataset.read<std::vector<int>>() == std::vector<int>{0, 0, 2, 3, 4});
    }
    SECTION("Both selections with different ranks") {
        dataset.select({1, 1}, {2, 2}).write(input, h5pp::Hyperslab({1}, {4}));
        CHECK(dataset.getInfo().dsetDims.value() == std::vector<hsize_t>{3, 3});
        CHECK(dataset.read<std::vector<int>>() == std::vector<int>{0, 0, 0, 0, 1, 2, 0, 3, 4});
    }
}

TEST_CASE("Memory dimensions must fit a supplied buffer", "[hyperslab][data-slab]") {
    h5pp::File file(make_path("hyperslab-buffer-size"), h5pp::FileAccess::REPLACE, 0);
    auto       dataset = file.dataset("values");
    dataset.write(std::vector<int>{1, 2});
    const std::vector<int> input(4, 0);
    CHECK_THROWS_WITH(dataset.write(input, h5pp::Hyperslab({6}, {2}), {.dims = h5pp::Dims{8}}),
                      Catch::Matchers::ContainsSubstring("Buffer dimensions"));
    std::array<int, 4> output{-1, -1, -1, -1};
    CHECK_THROWS_WITH(dataset.readInto(output, h5pp::Hyperslab({6}, {2}), {.dims = h5pp::Dims{8}}),
                      Catch::Matchers::ContainsSubstring("Buffer dimensions"));
    CHECK(output == std::array<int, 4>{-1, -1, -1, -1});
    CHECK(dataset.read<std::vector<int>>() == std::vector<int>{1, 2});
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    return session.run(argc, argv);
}
