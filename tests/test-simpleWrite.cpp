
#include <complex>
#include <h5pp/h5pp.h>

int main() {
    std::string          outputFilename = "output/simpleWrite.h5";
    size_t               logLevel       = 0;
    h5pp::File           file(outputFilename, h5pp::FileAccess::REPLACE, logLevel);
    bool                 Boolean   = true;
    std::string          String    = "This is a string";
    char                 Char[100] = "This is a char array";
    double               Double    = 2.0;
    std::complex<double> ComplexDouble(3, 4);
    int                  arrayInt[5] = {1, 2, 3, 4, 5};

    std::vector<int>                  vectorInt(10, 42);
    std::vector<long>                 vectorLong(10, 42);
    std::vector<unsigned int>         vectorUint(10, 42);
    std::vector<unsigned long>        vectorUlong(10, 42);
    std::vector<float>                vectorFloat(10, 42);
    std::vector<double>               vectorDouble(10, 42);
    std::vector<std::complex<int>>    vectorComplexInt(10, std::complex<int>(42, 7));
    std::vector<std::complex<double>> vectorComplexDouble(10, {10.0, 5.0});

    h5pp::varr_t<double>              vlenDouble = std::vector<double>(10, 42.0);
    std::vector<h5pp::varr_t<double>> vectorVlenDouble;
    for(size_t i = 0; i < 10; i++) vectorVlenDouble.emplace_back(std::vector<double>(i, static_cast<double>(i)));

    struct Field2 {
        double x;
        double y;
    };
    struct Field3 {
        float x;
        float y;
        float z;
    };
    Field2 field2{0.53, 0.45};
    Field3 field3{0.54f, 0.56f, 0.58f};

    std::vector<Field2> field2array(10, {0.3, 0.8});
    std::vector<Field3> field3array(10, {0.3f, 0.8f, 1.4f});

    // Test normal write usage
    file.writeDataset(Boolean, "simpleWriteGroup/Boolean");
    file.writeDataset(false, "simpleWriteGroup/BooleanRval");
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
    file.writeDataset(field2, "simpleWriteGroup/field2");
    file.writeDataset(field3, "simpleWriteGroup/field3");
    file.writeDataset(field2array, "simpleWriteGroup/field2array");
    file.writeDataset(field3array, "simpleWriteGroup/field3array");

    // Test passing pointers
    file.writeDataset(vectorInt.data(), "simpleWriteGroup/vectorInt", vectorInt.size());
    file.writeDataset(vectorDouble.data(), "simpleWriteGroup/vectorDouble", vectorDouble.size());
    file.writeDataset(vectorInt.data(), "simpleWriteGroup/vectorInt", vectorInt.size());

    // Test variable-length data
    file.writeDataset(vlenDouble, "simpleWriteGroup/vlenDouble");
    file.writeDataset(vectorVlenDouble, "simpleWriteGroup/vectorVlenDouble");

#ifdef H5PP_USE_EIGEN3
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
    if(h5pp::hdf5::isCompressionAvaliable()) {
        file.setCompressionLevel(9);
        Eigen::Tensor<double, 4> bigTensor(40, 180, 40, 5);
        bigTensor.setConstant(1.0);
        file.writeDataset(bigTensor, "compressedWriteGroup/bigTensor");
    }
#endif

#if defined(H5PP_USE_FLOAT128) || defined(H5PP_USE_QUADMATH)
    h5pp::fp128 twopi_real = 6.28318530717958623199592693708837032318115234375;
    file.writeDataset(twopi_real, "simpleWriteGroup/twopi_real");
    h5pp::cx128 twopi_cplx;
    auto twopi_std_cplx = reinterpret_cast<std::complex<h5pp::fp128>*>(&twopi_cplx);
    file.writeDataset(*twopi_std_cplx, "simpleWriteGroup/twopi_cplx");
#endif

    return 0;
}
