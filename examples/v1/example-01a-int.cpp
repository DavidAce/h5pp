#include <h5pp/v1/h5pp.h>

int main() {
    h5pp::v1::File file(H5PP_EXAMPLE_DIR "example-01a-int.h5", h5pp::FileAccess::REPLACE); // Initialize a file

    int writeInt = 42;                          // Initialize an int
    file.writeDataset(writeInt, "integerData"); // Write data to file in dataset named "integerData"

    int readInt;                              // Allocate for a new int
    file.readDataset(readInt, "integerData"); // Read data

    auto readInt_alt = file.readDataset<int>("integerData"); // Or read by assignment

    h5pp::print("Wrote dataset: {}\n", writeInt);
    h5pp::print("Read  dataset: {} | alt: {}\n", readInt, readInt_alt);
    return 0;
}