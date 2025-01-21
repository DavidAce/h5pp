
#include <complex>
#include <h5pp/h5pp.h>

int main() {
    std::string outputFilename = "output/appendToDataset.h5";
    size_t      logLevel       = 0;
    h5pp::File  file(outputFilename, h5pp::FileAccess::REPLACE, logLevel);

    std::vector<double> data = {1, 2, 3, 4};
    file.writeDataset(data, "group/VectorDoubletemp", {4, 1}, H5D_CHUNKED, {4, 100});
    file.writeDataset(data, "group/VectorDoubletemp2", {});
    //    h5pp::Options options({})
    file.writeDataset(data, "group/VectorDouble0");
    file.writeDataset(data, "group/VectorDouble1", {4});
    //    h5pp::hid::h5t test2 ({4});
    //    h5pp::hid::h5t test3 = {4};
    //    h5pp::hid::h5t test4 = 4;
    //    file.writeDataset(data,{4,1}, "group/VectorDouble2");
    //    file.writeDataset(data,{4,1}, "group/VectorDouble3",{5,100});
    //    file.writeDataset(data,{4,1}, "group/VectorDouble4",{6,100},std::nullopt);
    file.writeDataset(data, "group/VectorDouble5", {4, 1}, H5D_CHUNKED, {4, 100}, {4ul, H5S_UNLIMITED});
    //    file.writeDataset(data,{4,1}, "group/VectorDouble6",{H5S_UNLIMITED,H5S_UNLIMITED},std::nullopt);
    //    file.writeDataset(data,{4,1}, "group/VectorDouble7",{4,100}, H5D_COMPACT);
    //    file.writeDataset(data,{4,1}, "group/VectorDouble8",{4,100}, H5D_CONTIGUOUS);
    //    file.writeDataset(data,{4,1}, "group/VectorDouble9",{4,100}, H5D_CHUNKED);
    //    file.writeDataset(data,{4,1}, "group/VectorDouble10",{4,100}, H5D_CHUNKED);
    file.writeDataset(data, "group/VectorDouble6", {});
    file.writeDataset(data, "group/VectorDouble7", {}, {});
    //    file.writeDataset(data, "group/VectorDouble4",{},{});

    data = {5, 6, 7, 8};
    file.appendToDataset(data, "group/VectorDouble5", 1);
    file.appendToDataset(data, "group/VectorDouble5", 1);

    data = {10, 11};
    //    file.appendToDataset(data, "group/VectorDouble5",0,{1,2});
    //    file.appendToDataset(data, "group/VectorDouble5",0,{2,1});

    //    file.appendToDataset(data, "group/VectorDouble4",1);

    data      = {1, 2, 3, 4};
    auto info = file.getDatasetInfo("group/VectorDouble5");
    file.createDataset("group/VectorDouble8", info.h5Type.value(), H5D_CHUNKED, {data.size(), 0ul});
    file.createDataset("group/VectorDouble8_alt", info.h5Type.value(), H5D_CHUNKED, {static_cast<int>(data.size()), 0});
    file.appendToDataset(data, "group/VectorDouble8", 1, {data.size(), 1ul});
    file.appendToDataset(data, "group/VectorDouble8_alt", 1, {static_cast<int>(data.size()), 1});
    return 0;
}
