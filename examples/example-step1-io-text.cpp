#include <h5pp/h5pp.h>

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-step1-io-text.h5", h5pp::FilePermission::REPLACE);

    std::string writeText = "Hello world";
    file.writeDataset(writeText,"stringData"); // Write data to file in dataset named "stringData"

    std::string readText;
    file.readDataset(readText,"stringData"); // Read data. The string readText is resized appropriately by h5pp

    // Alternatively, you can read by assignment
    auto readText_alt = file.readDataset<std::string>("stringData");


    printf("Wrote dataset: %s\n", writeText.c_str());
    printf("Read  dataset: %s | alt: %s \n", readText.c_str(),readText_alt.c_str());

    return 0;
}