#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <vector>

namespace {
    h5pp::File make_file() { return h5pp::File(H5PP_TEST_DIR "directWrite.h5", h5pp::FileAccess::REPLACE, 0); }
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

TEST_CASE("Chunkwise writes update only the selected slab", "[direct-write][chunkwise]") {
    if constexpr(not h5pp::has_direct_chunk) {
        SUCCEED("Direct chunk writing is unavailable in this HDF5 build");
        return;
    } else {
        auto  file = make_file();
        auto &low  = file.advanced().getLowLevelFile();

        std::vector<int>           fill(12 * 12, 1);
        h5pp::DatasetCreateOptions createOptions;
        createOptions.h5Layout    = H5D_CHUNKED;
        createOptions.dims        = {12, 12};
        createOptions.dimsChunk   = {12, 12};
        createOptions.dimsMax     = {90, 90};
        createOptions.compression = 0;
        low.dataset("dset_direct").create(fill, createOptions).write(fill, {.dims = createOptions.dims});
        [[maybe_unused]] auto dsetInfo = low.dataset("dset_direct").getInfo();

        std::vector<int> block(6 * 6);
        for(size_t idx = 0; idx < block.size(); idx++) block[idx] = static_cast<int>(idx);

        h5pp::Options options;
        options.linkPath   = "dset_direct";
        auto chunkInfo     = h5pp::scan::readDsetInfo(low.advanced().openFileHandle(), options, low.plists);
        chunkInfo.dsetSlab = h5pp::Hyperslab({4, 4}, {6, 6});
        h5pp::hdf5::selectHyperslab(chunkInfo.h5Space.value(), chunkInfo.dsetSlab.value());

        options.dataDims = {6, 6};
        auto data_info   = h5pp::scan::scanDataInfo(block, options);
        h5pp::hdf5::writeDataset_chunkwise(block, data_info, chunkInfo, low.plists);

        auto written = file.dataset("dset_direct").read<std::vector<int>>();
        REQUIRE(written.size() == fill.size());
        REQUIRE(file.dataset("dset_direct").getInfo().dsetDims.value() == std::vector<hsize_t>{12, 12});

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
    }
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    return session.run();
}
