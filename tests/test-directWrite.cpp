#include <h5pp/h5pp.h>
#include <iostream>
#include <vector>

struct Table {
    double x = 0, y = 0;
};

int main() {
    if constexpr(h5pp::has_direct_chunk) {
        /* Create the data space */
        auto file = h5pp::File("output/directWrite.h5", h5pp::FilePermission::REPLACE, 0);

        auto coord5 = h5pp::util::ind2sub({3, 4}, 5);
        auto coord7 = h5pp::util::ind2sub({3, 4}, 7);

        if(std::vector<hsize_t>{1, 1} != coord5) throw std::logic_error(h5pp::format("ind2sub not working [1,1] != {}", coord5));
        if(std::vector<hsize_t>{1, 3} != coord7) throw std::logic_error(h5pp::format("ind2sub not working [1,3] != {}", coord7));

        auto index5 = h5pp::util::sub2ind({3, 4}, {1, 1});
        auto index7 = h5pp::util::sub2ind({3, 4}, {1, 3});
        if(5 != index5) throw std::logic_error(h5pp::format("sub2ind not working 5 != {}", index5));
        if(7 != index7) throw std::logic_error(h5pp::format("sub2ind not working 7 != {}", index7));

        auto slab1 = h5pp::Hyperslab({1, 1}, {3, 3});
        auto slab2 = h5pp::Hyperslab({3, 1}, {3, 3});
        auto slab3 = h5pp::Hyperslab({0, 5}, {3, 3});

        auto slab12_check = h5pp::Hyperslab({3, 1}, {1, 3});
        auto slab12_compt = h5pp::hdf5::getSlabOverlap(slab1, slab2);
        if(slab12_check.offset.value() != slab12_compt.offset.value())
            throw std::runtime_error(
                h5pp::format("slabs are not the same <slab1|slab2> {} | slab12 {}", slab12_check.string(), slab12_compt.string()));
        if(slab12_check.extent.value() != slab12_compt.extent.value())
            throw std::runtime_error(
                h5pp::format("slabs are not the same <slab1|slab2> {} | slab12 {}", slab12_check.string(), slab12_compt.string()));

        //    h5pp::hid::h5t type = H5Tcopy(H5T_NATIVE_INT);
        //        auto           dsetInfo = file.createDataset(type, "dset_direct", {12, 12}, H5D_CHUNKED, {3, 3}, {12, 12}, 9);

        std::vector<int> fill(144, 1);
        //    auto dsetInfo = file.createDataset("dset_direct", H5T_NATIVE_INT , H5D_CHUNKED, {0,0});
        auto             dsetInfo =
            file.writeDataset(fill, "dset_direct", H5D_CHUNKED, {12, 12}, {90, 90}, std::nullopt, std::nullopt, std::nullopt, 0);

        /* Initialize data to write */
        std::vector<int> data(36, 0);
        for(size_t i = 0; i < data.size(); i++) data[i] = static_cast<int>(i);

        dsetInfo.dsetSlab = h5pp::Hyperslab({4, 4}, {6, 6});
        h5pp::hdf5::selectHyperslab(dsetInfo.h5Space.value(), h5pp::Hyperslab({4, 4}, {6, 6}));
        h5pp::Options options;
        options.dataDims = {6, 6};
        auto dataInfo    = h5pp::scan::scanDataInfo(data, options);

        h5pp::hdf5::writeDataset_chunkwise(data, dataInfo, dsetInfo, file.plists);
        //    h5pp::hdf5::writeDataset(data,dataInfo,dsetInfo, file.plists);

        auto matrix = file.readDataset<Eigen::MatrixXi>("dset_direct");
        std::cout << matrix << std::endl;
        // Register the compound type
        h5pp::hid::h5t MY_HDF5_TABLE_TYPE;
        MY_HDF5_TABLE_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Table));
        H5Tinsert(MY_HDF5_TABLE_TYPE, "x", HOFFSET(Table, x), H5T_NATIVE_DOUBLE);
        H5Tinsert(MY_HDF5_TABLE_TYPE, "y", HOFFSET(Table, y), H5T_NATIVE_DOUBLE);
        auto tableInfo = file.createTable(MY_HDF5_TABLE_TYPE, "somegroup/someTable", "someTable", std::nullopt, true);

        std::vector<Table> elems = {{1, 1}, {2, 2}};
        tableInfo                = file.appendTableRecords(elems, "somegroup/someTable");

        auto table = file.readTableRecords<std::vector<Table>>("somegroup/someTable");
        for(auto &t : table) h5pp::logger::log->info("table: {} {}", t.x, t.y);

        std::vector<Table> more_elems = {{3, 3}, {4, 4}};
        file.appendTableRecords(more_elems, "somegroup/someTable");
        table = file.readTableRecords<std::vector<Table>>("somegroup/someTable", 0);

        for(auto &t : table) h5pp::logger::log->info("table: {} {}", t.x, t.y);
    }
}
