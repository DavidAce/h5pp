#include <h5pp/h5pp.h>
#include <vector>

/*
 * This example shows how to enable compression in HDF5 datasets with H5D_CHUNKED layout.
 * For advanced dataset creation, such as chunking and compression, use a dataset handle and options.
 */

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-06c-chunking-compression.h5", h5pp::FileAccess::REPLACE);

    // Set a default compression level. This is used when the dataset is created with chunked layout.
    file.setCompressionLevel(3);

    // Initialize and fill a vector of doubles.
    std::vector<double> vWrite(1000);
    for(size_t i = 0; i < vWrite.size(); i++) vWrite[i] = static_cast<double>(i);

    // Describe how the dataset should be created.
    h5pp::DatasetCreateOptions createOptions;
    createOptions.h5Layout    = H5D_CHUNKED; // Compression requires chunked layout
    createOptions.dimsChunk   = {1000};      // Use one chunk with 1000 elements
    createOptions.compression = 3;           // Override the default compression level for this dataset

    // Ensure the dataset exists with the requested creation properties, then write the data.
    auto dataset = file.dataset("myStdVectorDouble").ensure(createOptions);
    dataset.write(vWrite);

    // Read the data back using the dataset handle.
    auto vRead = dataset.read<std::vector<double>>();
    h5pp::print("Read compressed dataset with {} elements\n", vRead.size());

    return 0;
}
