#include <h5pp/h5pp.h>

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-step1-io-int.h5", h5pp::FilePermission::REPLACE);

    int writeInt = 42;
    file.writeDataset(42,"integerData");     // Write data to file in dataset named "integerData"

    int readInt;
    file.readDataset(readInt,"integerData"); // Read data

    // Alternatively, you can read by assignment
    int readInt_alt = file.readDataset<int>("integerData");

    printf("Wrote dataset: %d\n", writeInt);
    printf("Read  dataset: %d | alt: %d \n", readInt,readInt_alt);
    return 0;
}