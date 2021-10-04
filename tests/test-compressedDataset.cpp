
#include <complex>
#include <h5pp/h5pp.h>
#include <iostream>

int main() {
    std::string outputFilename = "output/compressedDataset.h5";
    size_t      logLevel       = 0;
    h5pp::File  file(outputFilename, h5pp::FilePermission::REPLACE, logLevel);

    file.setCompressionLevel(9);
    std::vector<double> bigVector(10000 * 1024, 2.3);
    file.writeDataset(bigVector, "compressedWriteGroup/bigVector");

#ifdef H5PP_EIGEN3
    // Test compressed writes
    if(h5pp::hdf5::isCompressionAvaliable()) {
        file.setCompressionLevel(9);
        Eigen::Tensor<double, 4> bigTensor(40, 180, 40, 5);
        bigTensor.setConstant(1.0);
        file.writeDataset(bigTensor, "compressedWriteGroup/bigTensor");
    }
#endif
    return 0;
}
