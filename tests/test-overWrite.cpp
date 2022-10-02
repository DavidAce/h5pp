
#include <complex>
#include <h5pp/h5pp.h>

int main() {
    // Define dummy data
    std::string outputFilename = "output/overWrite.h5";
    size_t      logLevel       = 0;
    h5pp::File  file(outputFilename, h5pp::FileAccess::REPLACE, logLevel);

    std::string somestring = "this is a teststring";

    // Start with a typical type
    std::vector<std::complex<double>> vectorComplexDouble(10, {10.0, 5.0});
    // Write and overwrite
    file.writeDataset_contiguous(vectorComplexDouble, "overWriteGroup_contiguous/vectorComplexDouble");
    vectorComplexDouble = std::vector<std::complex<double>>(5, {10.0, 5.0});
    h5pp::Options   options;
    h5pp::Hyperslab slab({0}, {5});
    options.dataSlab = {slab};
    options.dsetSlab = {slab};
    options.linkPath = "overWriteGroup_contiguous/vectorComplexDouble";
    file.writeDataset(vectorComplexDouble, options);

    file.writeDataset(vectorComplexDouble, "overWriteGroup_chunked/vectorComplexDouble", std::nullopt, H5D_CHUNKED);
    file.writeDataset(vectorComplexDouble, "overWriteGroup_chunked/vectorComplexDouble");
    if(vectorComplexDouble != file.readDataset<std::vector<std::complex<double>>>("overWriteGroup_chunked/vectorComplexDouble"))
        throw std::runtime_error("vectorComplexDouble not the same after overwrite");

    // Increase size and overwirte
    vectorComplexDouble.resize(150, {10.0, 5.0});
    file.writeDataset(vectorComplexDouble, "overWriteGroup_chunked/vectorComplexDouble");
    if(vectorComplexDouble != file.readDataset<std::vector<std::complex<double>>>("overWriteGroup_chunked/vectorComplexDouble"))
        throw std::runtime_error("vectorComplexDouble not the same after resize+overwrite");

    file.writeDataset(somestring, "overWriteGroup_chunked/somestring", std::nullopt, H5D_CHUNKED);

    // Now overwrite
    file.writeDataset(vectorComplexDouble, "overWriteGroup_chunked/vectorComplexDouble");
    file.writeDataset(somestring, "overWriteGroup_chunked/somestring");

    // Now increase size and overwrite again
    // TODO: This does not work! The dataset has to be extended!
    vectorComplexDouble = std::vector<std::complex<double>>(15000, {10.0, 5.0});
    somestring          = "this is a slightly longer string";

    file.writeDataset(vectorComplexDouble, "overWriteGroup_chunked/vectorComplexDouble");
    file.writeDataset(somestring, "overWriteGroup_chunked/somestring");

    // Now decrease size and overwrite again
    vectorComplexDouble = std::vector<std::complex<double>>(1500, {10.0, 5.0});
    somestring          = "short string";
    file.resizeDataset("overWriteGroup_chunked/vectorComplexDouble", 1500, h5pp::ResizePolicy::FIT);
    file.writeDataset(vectorComplexDouble, "overWriteGroup_chunked/vectorComplexDouble");
    file.writeDataset(somestring, "overWriteGroup_chunked/somestring");

    // All the previous datasets were extendable, and if we reached this points they have been overritten successfully.
    // Now we'll try making some non-extendable datasets and check that overwriting actually fails
    //    file.disableDefaultExtendable();

    vectorComplexDouble = std::vector<std::complex<double>>(1000, {10.0, 5.0});
    file.writeDataset(vectorComplexDouble, "overWriteGroup/vectorComplexDouble");

    try {
        // Let's try writing something smaller that should fit in the allocated space
        vectorComplexDouble = std::vector<std::complex<double>>(100, {10.0, 5.0});
        file.writeDataset(vectorComplexDouble, "overWriteGroup/vectorComplexDouble");

    } catch(std::exception &ex) { h5pp::print("THE ERROR BELOW IS PART OF THE TEST AND WAS EXPECTED: \n -- {}", ex.what()); }

    try {
        // Let's try writing something larger that should fit in the allocated space
        vectorComplexDouble = std::vector<std::complex<double>>(10000, {10.0, 5.0});
        file.writeDataset(vectorComplexDouble, "overWriteGroup/vectorComplexDouble");
    } catch(std::exception &ex) { h5pp::print("THE ERROR BELOW IS PART OF THE TEST AND WAS EXPECTED: \n -- {}", ex.what()); }

    try {
        // Let's try writing something exactly the same size as before which should fit exactly in the allocated space
        vectorComplexDouble = std::vector<std::complex<double>>(1000, {10.0, 5.0});
        file.writeDataset(vectorComplexDouble, "overWriteGroup/vectorComplexDouble");
    } catch(std::exception &ex) { h5pp::print("THE ERROR BELOW IS PART OF THE TEST AND WAS EXPECTED: \n -- {}", ex.what()); }

    // Strings are a special case that shouldn't fail for non-e
    somestring = "this is a teststring";
    file.writeDataset(somestring, "overWriteGroup/somestring");
    file.writeDataset(somestring, "overWriteGroup/somestring");

    // This time we should check that making large enough dataset defaults to extendable even if
    // we disabled default extendable
    vectorComplexDouble = std::vector<std::complex<double>>(32 * 1024, {10.0, 5.0});
    file.writeDataset(vectorComplexDouble, "overWriteGroup_chunked/vectorComplexDouble");
    try {
        vectorComplexDouble = std::vector<std::complex<double>>(128 * 1024, {10.0, 5.0});
        file.writeDataset(vectorComplexDouble, "overWriteGroup_chunked/vectorComplexDouble");

    } catch(std::exception &ex) {
        throw std::runtime_error("Automatic change from contiguous to chunked when size is large failed: " + std::string(ex.what()));
    }

#ifdef H5PP_USE_EIGEN3
    // Do the same for Eigen types
    // Write chunked datasets that can be resized

    // Define dummy data
    Eigen::MatrixXi              matrixInt           = Eigen::MatrixXi::Random(100, 100);
    Eigen::MatrixXd              matrixDouble        = Eigen::MatrixXd::Random(100, 100);
    Eigen::MatrixXcd             matrixComplexDouble = Eigen::MatrixXcd::Random(100, 100);
    Eigen::Map<Eigen::MatrixXcd> matrixMapComplexDouble(matrixComplexDouble.data(), matrixComplexDouble.rows(), matrixComplexDouble.cols());
    Eigen::TensorMap<Eigen::Tensor<std::complex<double>, 2>> tensorMapComplexDouble(matrixComplexDouble.data(),
                                                                                    matrixComplexDouble.rows(),
                                                                                    matrixComplexDouble.cols());

    // Now write
    file.writeDataset(matrixInt, "overWriteGroup_chunked/matrixInt", std::nullopt, H5D_CHUNKED);
    file.writeDataset(matrixDouble, "overWriteGroup_chunked/matrixDouble", std::nullopt, H5D_CHUNKED);
    file.writeDataset(matrixComplexDouble, "overWriteGroup_chunked/matrixComplexDouble", std::nullopt, H5D_CHUNKED);
    file.writeDataset(matrixMapComplexDouble, "overWriteGroup_chunked/matrixMapComplexDouble", std::nullopt, H5D_CHUNKED);
    file.writeDataset(tensorMapComplexDouble, "overWriteGroup_chunked/tensorMapComplexDouble", std::nullopt, H5D_CHUNKED);

    // Now overwrite
    file.writeDataset(matrixInt, "overWriteGroup_chunked/matrixInt");
    file.writeDataset(matrixDouble, "overWriteGroup_chunked/matrixDouble");
    file.writeDataset(matrixComplexDouble, "overWriteGroup_chunked/matrixComplexDouble");

    // Now increase size and overwrite again
    matrixInt           = Eigen::MatrixXi::Random(200, 200);
    matrixDouble        = Eigen::MatrixXd::Random(200, 200);
    matrixComplexDouble = Eigen::MatrixXcd::Random(200, 200);

    file.writeDataset(matrixInt, "overWriteGroup_chunked/matrixInt");
    file.writeDataset(matrixDouble, "overWriteGroup_chunked/matrixDouble");
    file.writeDataset(matrixComplexDouble, "overWriteGroup_chunked/matrixComplexDouble");

    // Now decrease size and overwrite again
    matrixInt           = Eigen::MatrixXi::Random(20, 20);
    matrixDouble        = Eigen::MatrixXd::Random(20, 20);
    matrixComplexDouble = Eigen::MatrixXcd::Random(20, 20);

    file.writeDataset(matrixInt, "overWriteGroup_chunked/matrixInt");
    file.writeDataset(matrixDouble, "overWriteGroup_chunked/matrixDouble");
    file.writeDataset(matrixComplexDouble, "overWriteGroup_chunked/matrixComplexDouble");
#endif

    return 0;
}
