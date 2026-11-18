#include <h5pp/h5pp.h>
#include <vector>

/*
 * In this example we write a variable-length array into a single entry in a dataset.
 * h5pp provides the variable-length wrapper h5pp::varr_t for this purpose.
 */

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-01d-variable-length-array.h5", h5pp::FileAccess::REPLACE);

    // Initialize a variable-length int array. This is a single dataset entry.
    h5pp::varr_t<int> writeVlen = std::vector<int>{0, 1, 2, 3};
    file.dataset("v2Api/vlenIntData").write(writeVlen);                                       // Write data using the v2 API
    h5pp::varr_t<int> readVlen = file.dataset("v2Api/vlenIntData").read<h5pp::varr_t<int>>(); // Read data using the v2 API

    h5pp::print("v2 API wrote vlen dataset: {}\n", writeVlen);
    h5pp::print("v2 API read  vlen dataset: {}\n", readVlen);
    return 0;
}

/* Output

Wrote vlen dataset: [0, 1, 2, 3]
Read  vlen dataset: [0, 1, 2, 3] | alt: [0, 1, 2, 3]

 */
