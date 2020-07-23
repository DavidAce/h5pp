#include <h5pp/h5pp.h>

// This example shows how to enable chunking in HDF5 datasets
// Datasets with H5D_CHUNKED layout are more versatile than other layouts, i.e.
// H5D_CONTIGUOUS (default) or H5D_COMPACT.
// While H5D_CHUNKED has a bit more overhead it enables
//      * unlimited datasets
//      * resizeable datasets
//      * compression
//
// H5D_CHUNKED have a fixed number of dimensions (rank).

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-step6-chunking-automatic.h5", h5pp::FilePermission::REPLACE);

    // Initialize a vector  doubles
    std::vector<double> v_write(1000);

    // Fill vector with some data
    for(size_t i = 0; i < v_write.size(); i++) v_write[i] = static_cast<double>(i);

    // Write data.
    // Note 1: h5pp deduces "good" chunking dimensions automatically.
    //         Because h5pp doesn't know the future size of a dataset,
    //         it will guess that each chunk is an n-cube of ~10 to 1000 KB.
    //         depending on the initial size and type.
    // Note 2: Chunk dimensions affect IO performance and compression efficiency,
    //         so setting this manually can be desirable for optimal performance.
    file.writeDataset(v_write, "myStdVectorDouble",H5D_CHUNKED);

    // Print
    printf("Wrote dataset: \n");
    for(auto &d : v_write) printf("%f\n", d);

    // Read data. The vector is resized automatically by h5pp.
    std::vector<double> v_read;
    file.readDataset(v_read, "myStdVectorDouble");

    // Print
    printf("Read dataset: \n");
    for(auto &d : v_read) printf("%f\n", d);

    // Alternatively, read by assignment
    auto v_read_alt = file.readDataset<std::vector<double>>("myStdVectorDouble");

    printf("Read dataset alternate: \n");
    for(auto &d : v_read_alt) printf("%f\n", d);

    return 0;
}