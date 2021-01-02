#include <h5pp/h5pp.h>

/* This example shows how to enable chunking in HDF5 datasets and to set the chunking dimensions manually.
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
     *         it will assume that each chunk is an n-cube of size 10 KB to 1000 KB,
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
     * Note 3: The 4th argument "{1000}" specifies the dataset dimensions, or shape. This argument is optional
     *         and can be skipped using "std::nullopt". All arguments of type "h5pp::OptDimsType" can
     *         be skipped with std::nullopt.
     *         See example 08a where the dimension parameter is used to reinterpret
     *         the shape of the given container (e.g. {500,2} or {200,200,600} are also valid).
     *         If h5pp detects no argument, or if std::nullopt is given, then h5pp will deduce this
     *         argument based on the given data container. If you pass a C-style pointer to a raw data buffer
     *         then this argument can't be deduced so you must give the dimensions explicitly.
     */

    // Write data
    auto dsetInfo = file.writeDataset(v_write, "myStdVectorDouble", H5D_CHUNKED, {1000}, {10000});

    // Print dataset metadata
    h5pp::print("Wrote dataset: {}\n", dsetInfo.string());

    return 0;
}