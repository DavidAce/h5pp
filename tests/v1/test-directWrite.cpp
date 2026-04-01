#include <catch2/catch_all.hpp>
#include <h5pp/v1/h5pp.h>
#include <vector>

struct Table {
    double x = 0;
    double y = 0;

    bool operator==(const Table &other) const { return x == other.x and y == other.y; }
};

namespace {
    h5pp::v1::File make_file() { return h5pp::v1::File(H5PP_TEST_DIR "directWrite.h5", h5pp::FileAccess::REPLACE, 0); }
}

TEST_CASE("Direct-write helpers keep index and overlap calculations consistent", "[direct-write][helpers]") {
    REQUIRE(h5pp::util::ind2sub({3, 4}, 5) == std::vector<hsize_t>{1, 1});
    REQUIRE(h5pp::util::ind2sub({3, 4}, 7) == std::vector<hsize_t>{1, 3});
    REQUIRE(h5pp::util::sub2ind({3, 4}, {1, 1}) == 5);
    REQUIRE(h5pp::util::sub2ind({3, 4}, {1, 3}) == 7);

    auto slab1 = h5pp::Hyperslab({1, 1}, {3, 3});
    auto slab2 = h5pp::Hyperslab({3, 1}, {3, 3});
    auto slab3 = h5pp::Hyperslab({0, 5}, {3, 3});

    auto overlap12 = h5pp::hdf5::getSlabOverlap(slab1, slab2);
    auto overlap13 = h5pp::hdf5::getSlabOverlap(slab1, slab3);

    REQUIRE(overlap12.offset.value() == std::vector<hsize_t>{3, 1});
    REQUIRE(overlap12.extent.value() == std::vector<hsize_t>{1, 3});
    REQUIRE(overlap13.extent.value() == std::vector<hsize_t>{2, 0});
}

TEST_CASE("Chunkwise writes update only the selected slab and table appends still work", "[direct-write][chunkwise]") {
    if constexpr(not h5pp::has_direct_chunk) {
        SUCCEED("Direct chunk writing is unavailable in this HDF5 build");
        return;
    } else {
        auto file = make_file();

        std::vector<int> fill(12 * 12, 1);
        auto dset_info = file.writeDataset(fill, "dset_direct", H5D_CHUNKED, {12, 12}, {12, 12}, {90, 90}, std::nullopt, std::nullopt, 0);

        std::vector<int> block(6 * 6);
        for(size_t idx = 0; idx < block.size(); idx++) block[idx] = static_cast<int>(idx);

        dset_info.dsetSlab = h5pp::Hyperslab({4, 4}, {6, 6});
        h5pp::hdf5::selectHyperslab(dset_info.h5Space.value(), dset_info.dsetSlab.value());

        h5pp::Options options;
        options.dataDims = {6, 6};
        auto data_info   = h5pp::scan::scanDataInfo(block, options);
        h5pp::hdf5::writeDataset_chunkwise(block, data_info, dset_info, file.plists);

        auto written = file.readDataset<std::vector<int>>("dset_direct");
        REQUIRE(written.size() == fill.size());
        REQUIRE(file.getDatasetInfo("dset_direct").dsetDims.value() == std::vector<hsize_t>{12, 12});

        for(size_t row = 0; row < 12; row++) {
            for(size_t col = 0; col < 12; col++) {
                auto flat_index = row * 12 + col;
                if(row >= 4 and row < 10 and col >= 4 and col < 10) {
                    auto block_index = (row - 4) * 6 + (col - 4);
                    REQUIRE(written[flat_index] == block[block_index]);
                } else {
                    REQUIRE(written[flat_index] == 1);
                }
            }
        }

        h5pp::hid::h5t table_type = H5Tcreate(H5T_COMPOUND, sizeof(Table));
        REQUIRE(H5Tinsert(table_type, "x", HOFFSET(Table, x), H5T_NATIVE_DOUBLE) >= 0);
        REQUIRE(H5Tinsert(table_type, "y", HOFFSET(Table, y), H5T_NATIVE_DOUBLE) >= 0);

        auto table_info = file.createTable(table_type, "somegroup/someTable", "someTable", std::nullopt, true);
        REQUIRE(table_info.tablePath.value() == "somegroup/someTable");

        std::vector<Table> first = {
            {1, 1},
            {2, 2}
        };
        std::vector<Table> more = {
            {3, 3},
            {4, 4}
        };
        file.appendTableRecords(first, "somegroup/someTable");
        file.appendTableRecords(more, "somegroup/someTable");

        REQUIRE(file.readTableRecords<std::vector<Table>>("somegroup/someTable") == std::vector<Table>{
                                                                                        {1, 1},
                                                                                        {2, 2},
                                                                                        {3, 3},
                                                                                        {4, 4}
        });
        REQUIRE(file.readTableRecords<std::vector<Table>>("somegroup/someTable", 2) == more);
    }
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
