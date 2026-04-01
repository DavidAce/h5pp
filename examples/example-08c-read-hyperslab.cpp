#include <h5pp/h5pp.h>
#include <vector>

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-08c-read-hyperslab.h5", h5pp::FileAccess::REPLACE, 0);

    // Fill a flat vector with the values 0,1,2,...,24.
    std::vector<double> data5x5(25, 0.0);
    for(size_t idx = 0; idx < data5x5.size(); ++idx) data5x5[idx] = static_cast<double>(idx);

    // Write the vector, but interpret it as a 5x5 matrix.
    h5pp::DatasetWriteOptions asMatrix;
    asMatrix.dims = {5, 5};
    file.dataset("data5x5").write(data5x5, asMatrix);

    // Read back only the 2x2 block with top-left corner at position (1,2).
    auto data2x2 = file.dataset("data5x5").select({1, 2}, {2, 2}).read<std::vector<double>>();
    h5pp::print("Read hyperslab: {}\n", data2x2);
}
