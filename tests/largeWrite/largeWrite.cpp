
#include <complex>
#include <h5pp/h5pp.h>
#include <iostream>

int main() {
    std::string outputFilename = "output/largeWrite.h5";
    size_t      logLevel       = 0;
    h5pp::File  file(outputFilename, h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE, logLevel);
    file.writeDataset("teststring", "simpleWriteGroup/String");
    std::vector<std::complex<double>> vectorComplexDouble(10000, {10.0, 5.0});
    file.writeDataset(vectorComplexDouble, "largeWriteGroup/vectorComplexDouble");

#ifdef H5PP_EIGEN3
    Eigen::MatrixXi  matrixInt           = Eigen::MatrixXi::Random(500, 500);
    Eigen::MatrixXd  matrixDouble        = Eigen::MatrixXd::Random(500, 500);
    Eigen::MatrixXcd matrixComplexDouble = Eigen::MatrixXcd::Random(500, 500);
    file.writeDataset(matrixInt, "largeWriteGroup/matrixInt");
    file.writeDataset(matrixDouble, "largeWriteGroup/matrixDouble");
    file.writeDataset(matrixComplexDouble, "largeWriteGroup/matrixComplexDouble");
    Eigen::Matrix<h5pp::Type::Compound::H5T_COMPLEX_STRUCT<double>, Eigen::Dynamic, Eigen::Dynamic> test =
        matrixComplexDouble.transpose().cast<h5pp::Type::Compound::H5T_COMPLEX_STRUCT<double>>();
    file.writeDataset(test, "largeWriteGroup/transpose");
#endif
    return 0;
}
