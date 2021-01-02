#include <h5pp/h5pp.h>

/*
 * This example shows how to enable compression in HDF5 datasets with H5D_CHUNKED layout.
 * Datasets with H5D_CHUNKED layout are more versatile than the other layouts, i.e.
 * H5D_CONTIGUOUS (default) and H5D_COMPACT.
 * The H5D_CHUNKED layout has a bit more overhead and enables
 *      - unlimited max-size datasets
 *      - resizeable datasets
 *      - compressed datasets (a compression filter is applied to each chunk)
 *
 */

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-06c-chunking-compression.h5", h5pp::FilePermission::REPLACE);

    /*
     * Note: h5pp uses the "deflate" compression filter which is available
     *       when HDF5 is built with zlib support.
     *       The compression level is a number from 0 (off) to 9 (highest compression).
     *       For high levels (like 7,8 or 9) there can be a large increase in computational effort
     *       with very little reduction in file size. A good trade-off between performance and
     *       file size found at levels around 2 to 5.
     */

    // Set a default compression level
    file.setCompressionLevel(3);

    // Initialize a vector of doubles
    std::vector<double> v_write(1000);

    // Fill vector with some data
    for(size_t i = 0; i < v_write.size(); i++) v_write[i] = static_cast<double>(i);

    // Write data.
    //  Note: The compression level can also be given as the last argument in a writeDataset() call.
    auto dsetInfo = file.writeDataset(v_write, "myStdVectorDouble", H5D_CHUNKED);

    // Print dataset metadata
    h5pp::print("Wrote dataset: {}\n", dsetInfo.string());

    // Write data alternative version 1
    // The compression level can also be given as the last argument in a writeDataset() call.
    // Tip: An IDE with autocomplete can be very useful to get this right...
    file.writeDataset(v_write, "myStdVectorDouble_alt1", H5D_CHUNKED, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, 3);

    // Write data alternative version 2
    // Slighly les verbose is to use writeDataset_chunked
    // Tip: An IDE with autocomplete can be very useful to get this right...
    file.writeDataset_chunked(v_write, "myStdVectorDouble_alt2", std::nullopt, std::nullopt, std::nullopt, std::nullopt, 3);

    // Write data alternative version 3
    // A special member function writeDataset_compressed(...) can be used to express the intention to compress in the most compact form
    // Note: Compression always implies H5D_CHUNKED layout.
    file.writeDataset_compressed(v_write, "myStdVectorDouble_alt3", 3);

    return 0;
}