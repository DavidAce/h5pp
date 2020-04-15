
#include <complex>
#include <h5pp/h5pp.h>
#include <iostream>

int main() {
    // Define dummy data
    std::string outputFilename = "output/chunkDimension.h5";
    size_t      logLevel       = 0;
    h5pp::File  file(outputFilename, h5pp::FilePermission::REPLACE, logLevel);

    using namespace std::complex_literals;
    std::vector<std::complex<double>> vectorComplexDouble(10000, {10.0, 5.0});

    file.writeDataset(vectorComplexDouble, "chunkedgroup/vectorComplexDouble");
    return 0;
}
