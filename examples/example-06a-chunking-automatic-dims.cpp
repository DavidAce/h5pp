#include <h5pp/h5pp.h>

/* This example shows how to enable chunking of HDF5 datasets and allow
 * h5pp to determine the chunk dimensions automatically.
 * Datasets with H5D_CHUNKED layout are more versatile than the other layouts, i.e.
 * H5D_CONTIGUOUS (default) and H5D_COMPACT.
 * The H5D_CHUNKED layout has a bit more overhead and enables
 *      - unlimited max-size datasets
 *      - resizeable datasets
 *      - compressed datasets (a compression filter is applied to each chunk)
 *
 * NOTE:
 * In HDF5 a dataset must have fixed rank (number of dimensions), and it cannot be changed after creation.
 * The rank of chunks must be equal to the rank of the corresponding dataset.
 * Example:
 *      Let a dataset have 3 dimensions {10000,5000,1000}. In other words, it has rank 3.
 *      Reasonable chunk dimensions could be {100,100,100}. Also rank 3.
 *      HDF5 will treat whole chunks in buffered IO operations such as reads,
 *      writes and compression.
 */

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-06a-chunking-automatic.h5", h5pp::FilePermission::REPLACE);

    // Initialize a vector of doubles
    std::vector<double> v_write(1000);

    // Fill vector with some data
    for(size_t i = 0; i < v_write.size(); i++) v_write[i] = static_cast<double>(i);

    /*
     * Note 1: h5pp deduces "good" chunking dimensions automatically when none are given.
     *         Because h5pp doesn't know the future size of a dataset,
     *         it will guess that each chunk is an n-cube of ~10 to 1000 KB.
     *         depending on the initial size and type.
     * Note 2: Chunk dimensions affect IO performance and compression efficiency,
     *         so setting chunking dimensions automatically may be OK but not
     *         necessarily optimal for performance (the next example shows how to
     *         set chunking dimensions manually)
     */

    // Write data.
    auto dsetInfo = file.writeDataset(v_write, "myStdVectorDouble", H5D_CHUNKED);

    // Print dataset metadata
    h5pp::print("Wrote dataset: {}\n", dsetInfo.string());

    return 0;
}