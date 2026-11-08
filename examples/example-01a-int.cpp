#include <h5pp/h5pp.h>

void v2Api(h5pp::File &file) {
    int writeInt = 42; // Initialize an int

    file.dataset("v2Api/integerData").write(writeInt); // Write data using the v2 API
    int readInt = file.dataset("v2Api/integerData").read<int>(); // Read data using the v2 API

    h5pp::print("v2 API wrote dataset: {}\n", writeInt);
    h5pp::print("v2 API read  dataset: {}\n", readInt);
}

void v1Api(h5pp::File &file) {
    int writeInt = 42; // Initialize an int

    file.writeDataset(writeInt, "v1Api/integerData"); // Write data using the v1 API
    int readInt = file.readDataset<int>("v1Api/integerData"); // Read data using the v1 API

    h5pp::print("v1 API wrote dataset: {}\n", writeInt);
    h5pp::print("v1 API read  dataset: {}\n", readInt);
}

int main() {
    h5pp::File file(H5PP_EXAMPLE_DIR "example-01a-int.h5", h5pp::FileAccess::REPLACE); // Initialize a file
    v2Api(file);                                                               // Show the v2 API
    v1Api(file);                                                              // Show the v1 API
    return 0;
}
