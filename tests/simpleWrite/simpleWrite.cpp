
#include <complex>
#include <h5pp/h5pp.h>
#include <iostream>

int main() {
    std::string                       outputFilename = "output/simpleWrite.h5";
    size_t                            logLevel       = 0;
    h5pp::File                        file(outputFilename, h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE, logLevel);
    std::string                       String    = "This is a string";
    char                              Char[100] = "This is a char array";
    double                            Double    = 2.0;
    std::complex<double>              ComplexDouble(3, 4);
    int                               arrayInt[5] = {1, 2, 3, 4, 5};
    std::vector<int>                  vectorInt(10, 42);
    std::vector<long>                 vectorLong(10, 42);
    std::vector<unsigned int>         vectorUint(10, 42);
    std::vector<unsigned long>        vectorUlong(10, 42);
    std::vector<float>                vectorFloat(10, 42);
    std::vector<double>               vectorDouble(10, 42);
    std::vector<std::complex<int>>    vectorComplexInt(10, std::complex<int>(42, 7));
    std::vector<std::complex<double>> vectorComplexDouble(10, {10.0, 5.0});

    struct Field2 {
        double x;
        double y;
    };
    struct Field3 {
        float x;
        float y;
        float z;
    };
    std::vector<Field2> field2array(10, {0.3, 0.8});
    std::vector<Field3> field3array(10, {0.3, 0.8, 1.4});

    // Test normal write usage
    file.writeDataset(String, "simpleWriteGroup/String");
    file.writeDataset(Char, "simpleWriteGroup/Char");
    file.writeDataset(Double, "simpleWriteGroup/Double");
    file.writeDataset(logLevel, "simpleWriteGroup/logLevel");
    file.writeDataset(ComplexDouble, "simpleWriteGroup/ComplexDouble");
    file.writeDataset(arrayInt, "simpleWriteGroup/arrayInt");
    file.writeDataset(vectorInt, "simpleWriteGroup/vectorInt");
    file.writeDataset(vectorLong, "simpleWriteGroup/vectorLong");
    file.writeDataset(vectorUint, "simpleWriteGroup/vectorUint");
    file.writeDataset(vectorUlong, "simpleWriteGroup/vectorUlong");
    file.writeDataset(vectorFloat, "simpleWriteGroup/vectorFloat");
    file.writeDataset(vectorDouble, "simpleWriteGroup/vectorDouble");
    file.writeDataset(vectorComplexInt, "simpleWriteGroup/vectorComplexInt");
    file.writeDataset(vectorComplexDouble, "simpleWriteGroup/vectorComplexDouble");
    file.writeDataset(field2array, "simpleWriteGroup/field2array");
    file.writeDataset(field3array, "simpleWriteGroup/field3array");

    // Test passing pointers
    file.writeDataset(vectorInt.data(), vectorInt.size(), "simpleWriteGroup/vectorInt");
    file.writeDataset(vectorDouble.data(), vectorDouble.size(), "simpleWriteGroup/vectorDouble");
    file.writeDataset(vectorInt.data(), vectorInt.size(), "simpleWriteGroup/vectorInt");
    return 0;

#ifdef H5PP_EIGEN3
    Eigen::MatrixXi  matrixInt(2, 2);
    Eigen::MatrixXd  matrixDouble(2, 2);
    Eigen::MatrixXcd matrixComplexDouble(2, 2);
    matrixInt << 1, 2, 3, 4;
    matrixDouble << 1.5, 2.5, 3.5, 4.5;
    matrixComplexDouble.setRandom();

    // Test normal write usage
    file.writeDataset(matrixInt, "simpleWriteGroup/matrixInt");
    file.writeDataset(matrixDouble, "simpleWriteGroup/matrixDouble");
    file.writeDataset(matrixComplexDouble, "simpleWriteGroup/matrixComplexDouble");

    // Test compressed writes
    if(h5pp::checkIfCompressionIsAvailable()) {
        file.setCompressionLevel(9);
        Eigen::Tensor<double, 4> bigTensor(40, 180, 40, 5);
        bigTensor.setConstant(1.0);
        file.writeDataset(bigTensor, "compressedWriteGroup/bigTensor");
    }
#endif

    return 0;
}
