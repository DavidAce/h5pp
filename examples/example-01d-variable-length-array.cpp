#include <h5pp/h5pp.h>

/*
 * In this example write a variable-length array into a single entry in a dataset.
 * h5pp provides a variable-length wrapper h5pp::vlen_t which
 *
 */

int main() {
    h5pp::File file("exampledir/example-01d-variable-length-array.h5", h5pp::FileAccess::REPLACE); // Initialize a file

    h5pp::varr_t<int> writeVlen = std::vector<int>{0, 1, 2, 3}; // Initialize a variable-length int array. This is a single dataset entry!
    file.writeDataset(writeVlen, "vlenIntData");                // Write data to file in dataset named "integerData"

    h5pp::varr_t<int> readVlen;                // Allocate for a new variable-length int array
    file.readDataset(readVlen, "vlenIntData"); // Read data

    // h5pp::vlen_t<T> can be copied to std::vector<T> on the fly
    std::vector<int> readVlen_alt = file.readDataset<h5pp::varr_t<int>>("vlenIntData"); // Or read by assignment

    h5pp::print("Wrote vlen dataset: {}\n", writeVlen);
    h5pp::print("Read  vlen dataset: {} | alt: {}\n", readVlen, readVlen_alt);
    return 0;
}


/* Output

Wrote vlen dataset: [0, 1, 2, 3]
Read  vlen dataset: [0, 1, 2, 3] | alt: [0, 1, 2, 3]

 */