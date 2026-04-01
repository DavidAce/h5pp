#include <h5pp/h5pp.h>
#include <vector>

// This example shows how to write data into a portion of a dataset, a so-called hyperslab.
// In v2, the simplest form is to select a hyperslab on the dataset handle and write directly there.

/********************************************************************
   Note that the HDF5 C-API uses row-major layout!
*********************************************************************/

int main() {
    // Initialize a file.
    h5pp::File file(H5PP_EXAMPLE_DIR "example-08b-write-hyperslab.h5", h5pp::FileAccess::REPLACE);

    // Initialize a vector with size 25 filled with zeros.
    std::vector<double> data5x5(25, 0.0);
    h5pp::DatasetWriteOptions asMatrix;
    asMatrix.dims = {5, 5};      // Interpret the flat buffer as a 5x5 matrix
    file.dataset("data5x5").write(data5x5, asMatrix);

    // Initialize a small vector with size 4 that we will interpret as a 2x2 matrix.
    std::vector<double> data2x2 = {1.0, 2.0, 3.0, 4.0};
    h5pp::DatasetWriteOptions patch;
    patch.dims = {2, 2};      // Interpret the patch as a 2x2 matrix

    // Select a 2x2 hyperslab in the dataset with top-left corner at position (1,2), and write the patch there.
    file.dataset("data5x5").select({1, 2}, {2, 2}).write(data2x2, patch);

    // Print the resulting 5x5 matrix.
    auto read5x5 = file.dataset("data5x5").read<std::vector<double>>();
    h5pp::print("Patched 5x5 matrix:\n");
    for(size_t row = 0; row < 5ul; row++) {
        for(size_t col = 0; col < 5ul; col++) {
            h5pp::print("{} ", read5x5[row * 5 + col]);
        }
        h5pp::print("\n");
    }
    return 0;
}
