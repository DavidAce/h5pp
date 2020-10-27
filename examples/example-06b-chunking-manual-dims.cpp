#include <h5pp/h5pp.h>

/* This example shows how to enable chunking in HDF5 datasets and setting
 * the chunking dimensions manually
 * Datasets with H5D_CHUNKED layout are more versatile than the other layouts, i.e.
 * H5D_CONTIGUOUS (default) and H5D_COMPACT.
 * While H5D_CHUNKED has a bit more overhead it enables
 *      * unlimited max-size datasets
 *      * resizeable datasets
 *      * compressed datasets (a compression filter is applied to each chunk)
 *
 * In HDF5, datasets have a fixed number of dimensions, or rank, which cannot be changed after dataset creation.
 * The rank of chunk dimensions must be equal to the rank of dataset dimensions.
 *
 */
int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-06b-chunking-manual.h5", h5pp::FilePermission::REPLACE);

    // Initialize a vector of doubles
    std::vector<double> v_write(1000);

    // Fill vector with some data
    for(size_t i = 0; i < v_write.size(); i++) v_write[i] = static_cast<double>(i);

    /*
     * Note 1: h5pp deduces "good" chunking dimensions automatically when none are given.
     *         Since h5pp doesn't know the future size of a dataset,
     *         it will guess assume that each chunk is an n-cube of size 10 KB to 1000 KB,
     *         depending on the initial size and type of the given container.
     *         Chunk dimensions affect IO performance and compression efficiency,
     *         so setting chunking dimensions automatically may be OK but not
     *         necessarily optimal for performance.
     *
     * Note 2: In this example we set the chunk dimensions manually. It is not always obvious what
     *         good chunking dimensions are: best is to benchmark your particular case.
     *         A good rule of thumb is to aim for 10 KB to 1000 KB worth of elements in suitable
     *         shape. For 1-dimensional arrays there is only one option for the dimensions.
     *         In this example we choose chunks containing 10000 elements, so each
     *         chunk becomes 80 KB (because sizeof(double) * 10K = 80KB).
     *
     * Note 3: The dataset size argument "{1000}" is optional and can be skipped using "std::nullopt".
     *         In fact, all arguments taken as type "h5pp::OptDimsType" have this property.
     *         If h5pp detects no argument or if std::nullopt is given, h5pp will deduce the shape
     *         based on the given data container. If you pass a C-style pointer to a raw data buffer
     *         then shapes can't be deduced so you must pass dimensions explicitly.
     */

    // Write data
    auto dsetInfo = file.writeDataset(v_write, "myStdVectorDouble", H5D_CHUNKED, {1000}, {10000});

    // Print dataset metadata
    h5pp::print("Wrote dataset: {}\n", dsetInfo.string());

    return 0;
}