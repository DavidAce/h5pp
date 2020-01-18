
#include <complex>
#include <h5pp/h5pp.h>
#include <iostream>

int main() {
    // Define dummy data
    std::string outputFilename = "output/overWrite.h5";
    size_t      logLevel       = 0;
    h5pp::File  file(outputFilename, h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE, logLevel);

    using namespace std::complex_literals;
    std::vector<std::complex<double>> vectorComplexDouble(10000, {10.0, 5.0});
    std::string                       somestring = "this is a teststring";

    // Enable default extendable datasets that can be resized
    //    file.enableDefaultExtendable();
    // Now write
    file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_enabled/vectorComplexDouble", H5D_CHUNKED);
    file.writeDataset(somestring, "overWriteGroup_ext_enabled/somestring", H5D_CHUNKED);

    // Now overwrite
    file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_enabled/vectorComplexDouble");
    file.writeDataset(somestring, "overWriteGroup_ext_enabled/somestring");

    // Now increase size and overwrite again
    vectorComplexDouble = std::vector<std::complex<double>>(15000, {10.0, 5.0});
    somestring          = "this is a slightly longer string";

    file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_enabled/vectorComplexDouble");
    file.writeDataset(somestring, "overWriteGroup_ext_enabled/somestring");

    // Now decrease size and overwrite again
    vectorComplexDouble = std::vector<std::complex<double>>(1500, {10.0, 5.0});
    somestring          = "short string";

    file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_enabled/vectorComplexDouble");
    file.writeDataset(somestring, "overWriteGroup_ext_enabled/somestring");

    // All the previous datasets were extendable, and if we reached this points they have been overritten successfully.
    // Now we'll try making some non-extendable datasets and check that overwriting actually fails
    //    file.disableDefaultExtendable();

    vectorComplexDouble = std::vector<std::complex<double>>(1000, {10.0, 5.0});
    file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_disabled/vectorComplexDouble");

    try {
        // Let's try writing something smaller that should fit in the allocated space
        vectorComplexDouble = std::vector<std::complex<double>>(100, {10.0, 5.0});
        file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_disabled/vectorComplexDouble");

    } catch(std::exception &ex) { std::cout << "\n \t THE ERROR BELOW IS PART OF THE TEST AND WAS EXPECTED: " << ex.what() << std::endl; }

    try {
        // Let's try writing something larger that should fit in the allocated space
        vectorComplexDouble = std::vector<std::complex<double>>(10000, {10.0, 5.0});
        file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_disabled/vectorComplexDouble");
    } catch(std::exception &ex) { std::cout << "\n THE ERROR IS PART OF THE TEST AND IS EXPECTED: " << ex.what() << std::endl; }

    try {
        // Let's try writing something exactly the same size as before which should fit exactly in the allocated space
        vectorComplexDouble = std::vector<std::complex<double>>(1000, {10.0, 5.0});
        file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_disabled/vectorComplexDouble");
    } catch(std::exception &ex) { std::cout << "\n THIS ERROR IS PART OF THE TEST AND IS EXPECTED: " << ex.what() << std::endl; }

    // Strings are a special case that shouldn't fail for non-e
    somestring = "this is a teststring";
    file.writeDataset(somestring, "overWriteGroup_ext_disabled/somestring");
    file.writeDataset(somestring, "overWriteGroup_ext_disabled/somestring");

    // This time we should check that making large enough dataset defaults to extendable even if
    // we disabled default extendable
    vectorComplexDouble = std::vector<std::complex<double>>(32 * 1024, {10.0, 5.0});
    file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_disabled_chunked/vectorComplexDouble");
    try {
        vectorComplexDouble = std::vector<std::complex<double>>(128 * 1024, {10.0, 5.0});
        file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_disabled_chunked/vectorComplexDouble");

    } catch(std::exception &ex) { throw std::runtime_error("Automatic change from contiguous to chunked when size is large failed: " + std::string(ex.what())); }

#ifdef H5PP_EIGEN3
    // Do the same for Eigen types
    // Write chunked datasets that can be resized

    // Define dummy data
    Eigen::MatrixXi                                          matrixInt           = Eigen::MatrixXi::Random(100, 100);
    Eigen::MatrixXd                                          matrixDouble        = Eigen::MatrixXd::Random(100, 100);
    Eigen::MatrixXcd                                         matrixComplexDouble = Eigen::MatrixXcd::Random(100, 100);
    Eigen::Map<Eigen::MatrixXcd>                             matrixMapComplexDouble(matrixComplexDouble.data(), matrixComplexDouble.rows(), matrixComplexDouble.cols());
    Eigen::TensorMap<Eigen::Tensor<std::complex<double>, 2>> tensorMapComplexDouble(matrixComplexDouble.data(), matrixComplexDouble.rows(), matrixComplexDouble.cols());

    // Now write
    file.writeDataset(matrixInt, "overWriteGroup_ext_enabled/matrixInt", H5D_CHUNKED);
    file.writeDataset(matrixDouble, "overWriteGroup_ext_enabled/matrixDouble", H5D_CHUNKED);
    file.writeDataset(matrixComplexDouble, "overWriteGroup_ext_enabled/matrixComplexDouble", H5D_CHUNKED);
    file.writeDataset(matrixMapComplexDouble, "overWriteGroup_ext_enabled/matrixMapComplexDouble", H5D_CHUNKED);
    file.writeDataset(tensorMapComplexDouble, "overWriteGroup_ext_enabled/tensorMapComplexDouble", H5D_CHUNKED);

    // Now overwrite
    file.writeDataset(matrixInt, "overWriteGroup_ext_enabled/matrixInt");
    file.writeDataset(matrixDouble, "overWriteGroup_ext_enabled/matrixDouble");
    file.writeDataset(matrixComplexDouble, "overWriteGroup_ext_enabled/matrixComplexDouble");

    // Now increase size and overwrite again
    matrixInt           = Eigen::MatrixXi::Random(200, 200);
    matrixDouble        = Eigen::MatrixXd::Random(200, 200);
    matrixComplexDouble = Eigen::MatrixXcd::Random(200, 200);

    file.writeDataset(matrixInt, "overWriteGroup_ext_enabled/matrixInt");
    file.writeDataset(matrixDouble, "overWriteGroup_ext_enabled/matrixDouble");
    file.writeDataset(matrixComplexDouble, "overWriteGroup_ext_enabled/matrixComplexDouble");

    // Now decrease size and overwrite again
    matrixInt           = Eigen::MatrixXi::Random(20, 20);
    matrixDouble        = Eigen::MatrixXd::Random(20, 20);
    matrixComplexDouble = Eigen::MatrixXcd::Random(20, 20);

    file.writeDataset(matrixInt, "overWriteGroup_ext_enabled/matrixInt");
    file.writeDataset(matrixDouble, "overWriteGroup_ext_enabled/matrixDouble");
    file.writeDataset(matrixComplexDouble, "overWriteGroup_ext_enabled/matrixComplexDouble");

#endif

    return 0;
}
