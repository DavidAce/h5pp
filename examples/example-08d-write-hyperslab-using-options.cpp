#include <h5pp/h5pp.h>
#include <vector>

// This example shows how to write data into a portion of a dataset, a so-called hyperslab.
// In example 08b we selected only the dataset region. Here we also select a subregion in memory with h5pp::DatasetWriteOptions::dataSlab.

/********************************************************************
   Note that the HDF5 C-API uses row-major layout!
*********************************************************************/

int main() {
    // Initialize a file.
    h5pp::File file(H5PP_EXAMPLE_DIR "example-08d-write-hyperslab-using-options.h5", h5pp::FileAccess::REPLACE);

    // Initialize a vector with size 25 filled with zeros.
    std::vector<double> data5x5(25, 0.0);

    // Write the data to a dataset, but interpret it as a 5x5 matrix.
    h5pp::DatasetWriteOptions asMatrix;
    asMatrix.dims = {5, 5}; // Interpret the flat buffer as a 5x5 matrix
    file.dataset("data5x5").write(data5x5, asMatrix);

    // This time we prepare a 3x3 patch, but we only want to write its centered 2x2 region.
    //
    // 0 0 0
    // 0 1 2
    // 0 3 4
    //
    // The centered 2x2 block should end up in the 5x5 dataset with top left corner at position (1,2).
    std::vector<double> data3x3 = {
        0.0, 0.0, 0.0,
        0.0, 1.0, 2.0,
        0.0, 3.0, 4.0,
    };

    h5pp::DatasetWriteOptions options;
    options.dims     = {3, 3};                          // Interpret the incoming buffer as a 3x3 matrix
    options.dataSlab = h5pp::Hyperslab({1, 1}, {2, 2}); // Select the centered 2x2 block in memory

    // Select a 2x2 hyperslab on the dataset and write the selected 2x2 block from memory there.
    file.dataset("data5x5").select({1, 2}, {2, 2}).write(data3x3, options);

    // Print the result.
    auto read5x5 = file.dataset("data5x5").read<std::vector<double>>();
    h5pp::print("Read 5x5 matrix:\n");
    for(size_t row = 0; row < 5ul; row++) {
        for(size_t col = 0; col < 5ul; col++) {
            h5pp::print("{} ", read5x5[row * 5 + col]);
        }
        h5pp::print("\n");
    }

    return 0;
}
