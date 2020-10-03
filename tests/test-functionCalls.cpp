#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <complex>
#include <h5pp/h5pp.h>
#include <iostream>

// Here we try all the function signatures
std::string outputFilename = "output/functionCalls.h5";
size_t      logLevel       = 2;
h5pp::File  file(outputFilename, h5pp::FilePermission::REPLACE, logLevel);

std::vector<std::optional<H5D_layout_t>> layouts      = {std::nullopt, H5D_COMPACT, H5D_CONTIGUOUS, H5D_CHUNKED};
std::vector<std::string>                 layout_names = {"auto", "compact", "contiguous", "chunked"};

TEST_CASE("Test createDataset given options call signatures", "[options]") {
    h5pp::Options options;
    options.h5Type       = H5Tcopy(H5T_NATIVE_DOUBLE);
    options.dataDims      = {4};
    std::string dsetGroup = "optionsCreateGroup/";
    for(size_t idx = 0; idx < layouts.size(); idx++) {
        SECTION(h5pp::format("Create from options only: {}", layout_names[idx])) {
            std::string dsetName = dsetGroup + "vectorDouble_" + layout_names[idx];
            options.linkPath = dsetName;
            options.h5Layout     = layouts[idx];
            file.createDataset(options);
            REQUIRE_THAT(file.readDataset<std::vector<double>>(dsetName), Catch::Equals<double>({0, 0, 0, 0}));
        }
    }
}

TEST_CASE("Test createDataset inferred call signatures", "[infer]") {
    h5pp::Options options;
    options.dataDims              = {4};
    std::string         dsetGroup = "inferCreateGroup/";
    std::vector<double> data      = {1, 2, 3, 4};
    for(size_t idx = 0; idx < layouts.size(); idx++) {
        SECTION(h5pp::format("Create inferred from data: {}", layout_names[idx])) {
            std::string dsetPath = dsetGroup + "vectorDouble_" + layout_names[idx];
            options.linkPath     = dsetPath;
            options.h5Layout     = layouts[idx];
            file.createDataset(data, options);
            REQUIRE_THAT(file.readDataset<std::vector<double>>(dsetPath), Catch::Equals<double>({0, 0, 0, 0}));
        }
    }
}

int main(int argc, char *argv[]) {
    Catch::Session session; // There must be exactly one instance
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) // Indicates a command line error
        return returnCode;

    //    session.configData().showSuccessfulTests = true;
    //    session.configData().reporterName = "compact";
    return session.run();
}


// TEST_CASE("Test createDataset manual call signatures", "[manual]") {
//    std::string         dsetGroup = "manualCreateGroup/";
//    std::vector<double> data      = {1, 2, 3, 4};
//    for(size_t idx = 0; idx < layouts.size(); idx++) {
//        SECTION(h5pp::format("Create inferred from data and parsed arguments: {}", layout_names[idx])) {
//            std::string dsetName = dsetGroup + "integral_data_dims_vectorDouble_" + layout_names[idx];
//            file.createDataset(data, 4 , dsetName, std::nullopt,std::nullopt,std::nullopt,std::nullopt,9);
//            REQUIRE_THAT(file.readDataset<std::vector<double>>(dsetName), Catch::Equals<double>({0, 0, 0, 0}));
//        }
//        SECTION(h5pp::format("Create inferred from data and parsed arguments: {}", layout_names[idx])) {
//            std::string dsetName = dsetGroup + "iterable_data_dims_vectorDouble_" + layout_names[idx];
//            file.createDataset(data, {4} , dsetName, std::nullopt,std::nullopt,std::nullopt,std::nullopt,9);
//            REQUIRE_THAT(file.readDataset<std::vector<double>>(dsetName), Catch::Equals<double>({0, 0, 0, 0}));
//        }
//        SECTION(h5pp::format("Create inferred from data and parsed arguments: {}", layout_names[idx])) {
//            std::string dsetName = dsetGroup + "iterable_dset_dims_vectorDouble_" + layout_names[idx];
//            file.createDataset(data, {4} , dsetName, {8} ,std::nullopt,std::nullopt,std::nullopt,9);
//            REQUIRE_THAT(file.readDataset<std::vector<double>>(dsetName), Catch::Equals<double>({0, 0, 0, 0}));
//        }
//    }
//}

//
//
// int main() {
//    // Here we try all the function signatures
//    std::string                       outputFilename = "output/functionCalls.h5";
//    size_t                            logLevel       = 0;
//    h5pp::File                        file(outputFilename, h5pp::FilePermission::REPLACE, logLevel);
//
//
//    h5pp::Options options;
//    options.h5_type = H5Tcopy(H5T_NATIVE_DOUBLE);
//    options.h5_layout = H5D_CHUNKED;
//    options.dsetDims = {4};
//    file.createDataset("createGroup/vectorDouble0",options);
//
//
//    std::vector<double> data = {1,2,3,4};
//    exit(0);
//
//    file.writeDataset(data, "group/VectorDouble0");
//    file.writeDataset(data, "group/VectorDouble1",{4,1},{4,100});
//    file.writeDataset(data, "group/VectorDouble2",{4,1},{});
//    file.writeDataset(data, "group/VectorDouble3", {},{4,100});
//    file.writeDataset(data, "group/VectorDouble4", {});
//    file.writeDataset(data, "group/VectorDouble4",{},{});
////    file.writeDataset(data, "group/VectorDouble4",{},{});
//
//    data = {5,6,7,8};
//    file.appendToDataset(data, "group/VectorDouble1",1);
//    file.appendToDataset(data, "group/VectorDouble2",1);
//    file.appendToDataset(data, "group/VectorDouble3",1);
//
//    data = {10,11};
//    file.appendToDataset(data, "group/VectorDouble1",0,{1,2});
//    file.appendToDataset(data, "group/VectorDouble2",0,{2,1});
//    file.appendToDataset(data, "group/VectorDouble3",0,{2});
//
////    file.appendToDataset(data, "group/VectorDouble4",1);
//
//
//    return 0;
//}
