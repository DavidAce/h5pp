#include <h5pp/h5pp.h>

/*
 * In this example we use h5pp to write and read an std::string
 * By default, h5pp stores std::string data as a variable-length array, i.e. H5T_VARIABLE.
 * For fixed-size datasets use const char * or specify the size of the std::string buffer.
 * By default, UTF-8 encoding is used.
 *
 */


int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-01b-text.h5", h5pp::FilePermission::REPLACE);

    // Initialize some text
    std::string writeText = "Hello world";

    // Write data to file in dataset named "stringData"
    file.writeDataset(writeText, "stringData");

    // Create an empty string
    std::string readText;

    // Read data. The string readText is resized appropriately by h5pp
    file.readDataset(readText, "stringData");

    // Or, read by assignment
    auto readText_alt = file.readDataset<std::string>("stringData");

    h5pp::print("Wrote dataset: {}\n", writeText);
    h5pp::print("Read  dataset: {} | alt: {}\n", readText, readText_alt);

    return 0;
}