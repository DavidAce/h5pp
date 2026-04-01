#include <h5pp/h5pp.h>
#include <vector>

/* This example shows how to enable chunking in HDF5 datasets and to set the chunk dimensions manually.
 * Datasets with H5D_CHUNKED layout are more versatile than the other layouts, i.e.
 * H5D_CONTIGUOUS (default) and H5D_COMPACT.
 * The H5D_CHUNKED layout has a bit more overhead and enables
 *      - unlimited max-size datasets
 *      - resizable datasets
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
    h5pp::File file(H5PP_EXAMPLE_DIR "example-06b-chunking-manual-dims.h5", h5pp::FileAccess::REPLACE);

    // Initialize a vector of doubles
    std::vector<double> vWrite(1000);

    // Fill vector with some data
    for(size_t i = 0; i < vWrite.size(); i++) vWrite[i] = static_cast<double>(i);

    /*
     * Note 1: h5pp can deduce "good" chunk dimensions automatically when none are given.
     *         Since h5pp doesn't know the future size of a dataset,
     *         it will assume that each chunk is an n-cube of size 10 KB to 1000 KB,
     *         depending on the initial size and type of the given container.
     *         Chunk dimensions affect IO performance and compression efficiency,
     *         so setting chunk dimensions automatically may be OK but not
     *         necessarily optimal for performance.
     *
     * Note 2: In this example we set the chunk dimensions manually. It is not always obvious what
     *         good chunk dimensions are: best is to benchmark your particular case.
     *         A good rule of thumb is to aim for 10 KB to 1000 KB worth of elements in suitable
     *         shape. For 1-dimensional arrays there is only one option for the dimensions.
     *         In this example we choose chunks containing 10000 elements, so each
     *         chunk becomes 80 KB (because sizeof(double) * 10K = 80 KB).
     *
     * Note 3: dims describes the dataset dimensions, or shape. This field is optional.
     *         See example 08a where the dimension parameter is used to reinterpret
     *         the shape of the given container (for example {500,2} or {200,200,600} are also valid).
     *         If you pass a C-style pointer to a raw data buffer then dims cannot be deduced,
     *         so you must give it explicitly.
     */

    // Describe how the dataset should be created.
    h5pp::DatasetCreateOptions createOptions;
    createOptions.h5Layout  = H5D_CHUNKED;
    createOptions.dims      = {1000};
    createOptions.dimsChunk = {10000};

    // Write data.
    auto dataset = file.dataset("myStdVectorDouble").ensure(createOptions);
    dataset.write(vWrite);

    // Print selected dataset metadata.
    auto dsetInfo = dataset.getInfo();
    if(dsetInfo.dsetPath) h5pp::print("Dataset path : {}\n", dsetInfo.dsetPath.value());
    if(dsetInfo.dsetDims) h5pp::print("Dataset dims : {}\n", dsetInfo.dsetDims.value());
    if(dsetInfo.dsetChunk) h5pp::print("Chunk dims   : {}\n", dsetInfo.dsetChunk.value());
    if(dsetInfo.h5Layout) h5pp::print("Layout       : {}\n", static_cast<int>(dsetInfo.h5Layout.value()));

    return 0;
}
