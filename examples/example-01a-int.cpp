#include <h5pp/h5pp.h>

int main() {
    h5pp::File file(H5PP_EXAMPLE_DIR "example-01a-int.h5", h5pp::FileAccess::REPLACE); // Initialize a file

    int writeInt = 42; // Initialize an int

    file.dataset("v2Api/integerData").write(writeInt);           // Write data using the v2 API
    int readInt = file.dataset("v2Api/integerData").read<int>(); // Read data using the v2 API

    h5pp::print("v2 API wrote dataset: {}\n", writeInt);
    h5pp::print("v2 API read  dataset: {}\n", readInt);
    return 0;
}
