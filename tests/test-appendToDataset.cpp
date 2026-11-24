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

    h5pp::File make_file(std::string_view name) {
        return h5pp::File(h5pp::format(H5PP_TEST_DIR "{}.h5", name), h5pp::FileAccess::REPLACE, 0);
    }
}

TEST_CASE("Dataset append preserves growth semantics across explicit creations", "[append]") {
    auto file = make_file("appendToDataset");

    std::vector<double> data = {1, 2, 3, 4};

    h5pp::DatasetCreateOptions seed;
    seed.dims      = {4, 1};
    seed.h5Layout  = H5D_CHUNKED;
    seed.dimsChunk = {4, 100};
    file.dataset("group/VectorDoubletemp").create(data, seed).write(data, {.dims = seed.dims});

    file.dataset("group/VectorDoubletemp2").write(data);
    file.dataset("group/VectorDouble0").write(data);

    h5pp::DatasetWriteOptions dimsOnlyWrite;
    dimsOnlyWrite.dims = {4};
    file.dataset("group/VectorDouble1").write(data, dimsOnlyWrite);

    h5pp::DatasetCreateOptions extensible;
    extensible.dims      = {4, 1};
    extensible.h5Layout  = H5D_CHUNKED;
    extensible.dimsChunk = {4, 100};
    extensible.dimsMax   = {4, H5S_UNLIMITED};
    file.dataset("group/VectorDouble5").create(data, extensible).write(data, {.dims = extensible.dims});

    file.dataset("group/VectorDouble6").write(data);
    file.dataset("group/VectorDouble7").write(data);

    require_chunked_layout(file.dataset("group/VectorDoubletemp").getInfo());
    require_dims(file.dataset("group/VectorDoubletemp").getInfo(), {4, 1});
    require_chunked_layout(file.dataset("group/VectorDouble5").getInfo());
    require_dims(file.dataset("group/VectorDouble5").getInfo(), {4, 1});
    require_max_dims(file.dataset("group/VectorDouble5").getInfo(), {4, H5S_UNLIMITED});

    REQUIRE(file.dataset("group/VectorDoubletemp2").read<std::vector<double>>() == data);
    REQUIRE(file.dataset("group/VectorDouble0").read<std::vector<double>>() == data);
    REQUIRE(file.dataset("group/VectorDouble1").read<std::vector<double>>() == data);
    REQUIRE(file.dataset("group/VectorDouble6").read<std::vector<double>>() == data);
    REQUIRE(file.dataset("group/VectorDouble7").read<std::vector<double>>() == data);

    auto append_info = file.dataset("group/VectorDouble5").append(std::vector<double>{5, 6, 7, 8}, 1).getInfo();
    require_dims(append_info, {4, 2});
    append_info = file.dataset("group/VectorDouble5").append(std::vector<double>{9, 10, 11, 12}, 1).getInfo();
    require_dims(append_info, {4, 3});

    REQUIRE(file.dataset("group/VectorDouble5").read<std::vector<double>>() == std::vector<double>{1, 5, 9, 2, 6, 10, 3, 7, 11, 4, 8, 12});

    h5pp::DatasetCreateOptions emptyColumns;
    emptyColumns.dims           = {data.size(), 0};
    emptyColumns.h5Layout       = H5D_CHUNKED;
    emptyColumns.h5Type         = file.dataset("group/VectorDouble5").getInfo().h5Type;
    [[maybe_unused]] auto info0 = file.dataset("group/VectorDouble8").create(emptyColumns);
    [[maybe_unused]] auto info1 = file.dataset("group/VectorDouble8_alt").create(emptyColumns);

    h5pp::DatasetAppendOptions appendDims;
    appendDims.dims = {data.size(), 1};
    auto info8      = file.dataset("group/VectorDouble8").append(data, 1, appendDims).getInfo();
    auto info8_alt  = file.dataset("group/VectorDouble8_alt").append(data, 1, appendDims).getInfo();
    require_dims(info8, {4, 1});
    require_dims(info8_alt, {4, 1});
    REQUIRE(file.dataset("group/VectorDouble8").read<std::vector<double>>() == data);
    REQUIRE(file.dataset("group/VectorDouble8_alt").read<std::vector<double>>() == data);
}

TEST_CASE("Append handles more types and more contrived growth patterns", "[append]") {
    SECTION("Append rows into a zero-sized leading axis") {
        auto file = make_file("appendToDataset-rows");

        h5pp::DatasetCreateOptions create;
        create.dims                 = {0, 3};
        create.dimsChunk            = {1, 3};
        create.dimsMax              = {H5S_UNLIMITED, 3};
        create.h5Layout             = H5D_CHUNKED;
        create.h5Type               = H5Tcopy(H5T_NATIVE_DOUBLE);
        [[maybe_unused]] auto info0 = file.dataset("group/rows").create(create);

        h5pp::DatasetAppendOptions append;
        append.dims = {1, 3};
        auto info   = file.dataset("group/rows").append(std::vector<double>{1, 2, 3}, 0, append).getInfo();
        require_dims(info, {1, 3});
        info = file.dataset("group/rows").append(std::vector<double>{4, 5, 6}, 0, append).getInfo();
        require_dims(info, {2, 3});

        REQUIRE(file.dataset("group/rows").read<std::vector<double>>() == std::vector<double>{1, 2, 3, 4, 5, 6});
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

        h5pp::DatasetCreateOptions create;
        create.dims      = {initial.size()};
        create.dimsChunk = {initial.size()};
        create.dimsMax   = {H5S_UNLIMITED};
        create.h5Layout  = H5D_CHUNKED;
        file.dataset("group/complex").create(initial, create).write(initial);

        auto info = file.dataset("group/complex").append(more, 0).getInfo();
        require_dims(info, {expect.size()});

        REQUIRE(file.dataset("group/complex").read<std::vector<std::complex<double>>>() == expect);
    }

    SECTION("Append fixed-size integer arrays into a 1d unlimited dataset") {
        auto file = make_file("appendToDataset-integers");

        std::vector<long long>   initial = {10, 20};
        std::array<long long, 3> more    = {30, 40, 50};
        std::vector<long long>   expect  = initial;
        expect.insert(expect.end(), more.begin(), more.end());

        h5pp::DatasetCreateOptions create;
        create.dims      = {initial.size()};
        create.dimsChunk = {initial.size()};
        create.dimsMax   = {H5S_UNLIMITED};
        create.h5Layout  = H5D_CHUNKED;
        file.dataset("group/integers").create(initial, create).write(initial);

        h5pp::DatasetAppendOptions append;
        append.dims = {more.size()};
        auto info   = file.dataset("group/integers").append(more, 0, append).getInfo();
        require_dims(info, {expect.size()});

        REQUIRE(file.dataset("group/integers").read<std::vector<long long>>() == expect);
    }
}

TEST_CASE("Append rejects invalid resize scenarios", "[append]") {
    auto file = make_file("appendToDataset-errors");

    h5pp::DatasetCreateOptions fixed;
    fixed.dims     = {4};
    fixed.h5Layout = H5D_CONTIGUOUS;
    file.dataset("group/fixed").create(std::vector<double>{1, 2, 3, 4}, fixed).write(std::vector<double>{1, 2, 3, 4});
    REQUIRE_THROWS(file.dataset("group/fixed").append(std::vector<double>{5, 6}, 0));

    h5pp::DatasetCreateOptions columns;
    columns.dims      = {4, 1};
    columns.h5Layout  = H5D_CHUNKED;
    columns.dimsChunk = {4, 1};
    columns.dimsMax   = {4, H5S_UNLIMITED};
    file.dataset("group/columns")
        .create(std::vector<double>{1, 2, 3, 4}, columns)
        .write(std::vector<double>{1, 2, 3, 4}, {.dims = columns.dims});

    h5pp::DatasetAppendOptions wrongCols;
    wrongCols.dims = {5, 1};
    REQUIRE_THROWS(file.dataset("group/columns").append(std::vector<double>{10, 11, 12, 13, 14}, 1, wrongCols));

    h5pp::DatasetAppendOptions wrongRows;
    wrongRows.dims = {2, 1};
    REQUIRE_THROWS(file.dataset("group/columns").append(std::vector<double>{20, 21}, 0, wrongRows));
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    return session.run();
}
