#include <h5pp/h5pp.h>

// This example shows how to enable compression in HDF5 datasets with H5D_CHUNKED layout.
// Datasets with H5D_CHUNKED layout are more versatile than the other layouts, i.e.
// H5D_CONTIGUOUS (default) and H5D_COMPACT.
// While H5D_CHUNKED has a bit more overhead it enables
//      * unlimited max-size datasets
//      * resizeable datasets
//      * compressed datasets (a compression filter is applied to each chunk)
//
//

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-06c-chunking-compression.h5", h5pp::FilePermission::REPLACE);

    // Set a default compression level
    // Note: h5pp uses the "deflate" compression filter which is available
    //       when HDF5 is built with zlib support.
    //       The compression level is a number from 0 (off) to 9 (highest compression).
    //       For high levels (like 7,8 or 9) there can be a large increase in computational effort
    //       with very little reduction in file size. A good trade-off between performance and
    //       file size found at levels around 2 to 5.
    file.setCompressionLevel(3);



    // Initialize a vector of doubles
    std::vector<double> v_write(1000);

    // Fill vector with some data
    for(size_t i = 0; i < v_write.size(); i++) v_write[i] = static_cast<double>(i);

    // Write data.
    //  Note: The compression level can also be given as the last argument in a writeDataset() call.
    auto dsetInfo = file.writeDataset(v_write, "myStdVectorDouble",H5D_CHUNKED);

    // Print dataset metadata
    printf("Wrote dataset: %s \n", dsetInfo.string().c_str());

    return 0;
}