
#include <complex>
#include <h5pp/h5pp.h>

int main() {
    // Define dummy data
    std::string outputFilename = "output/chunkDimension.h5";
    size_t      logLevel       = 0;
    h5pp::File  file(outputFilename, h5pp::FileAccess::REPLACE, logLevel);

    using namespace std::complex_literals;
    std::vector<std::complex<double>> vectorComplexDouble(10000, {10.0, 5.0});

    file.writeDataset(vectorComplexDouble, "chunkedgroup/vectorComplexDouble", H5D_CHUNKED);

    auto chunkDims = file.getDatasetChunkDimensions("chunkedgroup/vectorComplexDouble");

    if(not chunkDims) throw h5pp::runtime_error("Failed to enable chunking on dataset: {}", "chunkedgroup/vectorComplexDouble");
    else h5pp::logger::log->info("Created dataset chunkedgroup/vectorComplexDouble with chunk dimensions {}", chunkDims.value());
    return 0;
}
