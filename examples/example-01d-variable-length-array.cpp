#include <h5pp/h5pp.h>
#include <vector>

/*
 * In this example we write a variable-length array into a single entry in a dataset.
 * h5pp provides the variable-length wrapper h5pp::varr_t for this purpose.
 */

void longForm(h5pp::File &file) {
    // Initialize a variable-length int array. This is a single dataset entry.
    h5pp::varr_t<int> writeVlen = std::vector<int>{0, 1, 2, 3};
    file.dataset("longForm/vlenIntData").write(writeVlen); // Write data using the long-form dataset handle
    h5pp::varr_t<int> readVlen = file.dataset("longForm/vlenIntData").read<h5pp::varr_t<int>>(); // Read data using the long-form dataset handle

    h5pp::print("Long form wrote vlen dataset: {}\n", writeVlen);
    h5pp::print("Long form read  vlen dataset: {}\n", readVlen);
}

void shortForm(h5pp::File &file) {
    // Initialize a variable-length int array. This is a single dataset entry.
    h5pp::varr_t<int> writeVlen = std::vector<int>{0, 1, 2, 3};
    file.writeDataset(writeVlen, "shortForm/vlenIntData"); // Write data using the explicit File short form
    auto readVlen = file.readDataset<h5pp::varr_t<int>>("shortForm/vlenIntData"); // Read data using the explicit File short form

    h5pp::print("Short form wrote vlen dataset: {}\n", writeVlen);
    h5pp::print("Short form read  vlen dataset: {}\n", readVlen);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-01d-variable-length-array.h5", h5pp::FileAccess::REPLACE);
    longForm(file);  // Show the canonical handle-based API
    shortForm(file); // Show the compatibility short form
    return 0;
}

/* Output

Wrote vlen dataset: [0, 1, 2, 3]
Read  vlen dataset: [0, 1, 2, 3] | alt: [0, 1, 2, 3]

 */
