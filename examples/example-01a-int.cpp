#include <h5pp/h5pp.h>

void longForm(h5pp::File &file) {
    int writeInt = 42; // Initialize an int

    file.dataset("longForm/integerData").write(writeInt); // Write data using the long-form dataset handle
    int readInt = file.dataset("longForm/integerData").read<int>(); // Read data using the long-form dataset handle

    h5pp::print("Long form wrote dataset: {}\n", writeInt);
    h5pp::print("Long form read  dataset: {}\n", readInt);
}

void shortForm(h5pp::File &file) {
    int writeInt = 42; // Initialize an int

    file.writeDataset(writeInt, "shortForm/integerData"); // Write data using the explicit File short form
    int readInt = file.readDataset<int>("shortForm/integerData"); // Read data using the explicit File short form

    h5pp::print("Short form wrote dataset: {}\n", writeInt);
    h5pp::print("Short form read  dataset: {}\n", readInt);
}

int main() {
    h5pp::File file(H5PP_EXAMPLE_DIR "example-01a-int.h5", h5pp::FileAccess::REPLACE); // Initialize a file
    longForm(file);                                                               // Show the canonical handle-based API
    shortForm(file);                                                              // Show the compatibility short form
    return 0;
}
