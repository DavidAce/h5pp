#include <h5pp/h5pp.h>
#include <string>

/*
 * In this example we use h5pp to write and read an std::string.
 * By default, h5pp uses UTF-8 and stores std::string data as a variable-length array, i.e. H5T_VARIABLE.
 * For fixed-size datasets use const char * or specify the size of the std::string buffer in the
 * dataset options when writing.
 */

void longForm(h5pp::File &file) {
    // Initialize some text
    std::string writeText = "Hello world";

    // Write and read data using the long-form dataset handle.
    file.dataset("longForm/stringData").write(writeText);
    std::string readText = file.dataset("longForm/stringData").read<std::string>();

    h5pp::print("Long form wrote dataset: {}\n", writeText);
    h5pp::print("Long form read  dataset: {}\n", readText);
}

void shortForm(h5pp::File &file) {
    // Initialize some text
    std::string writeText = "Hello world";

    // Write and read data using the explicit File short form.
    file.writeDataset(writeText, "shortForm/stringData");
    auto readText = file.readDataset<std::string>("shortForm/stringData");

    h5pp::print("Short form wrote dataset: {}\n", writeText);
    h5pp::print("Short form read  dataset: {}\n", readText);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-01b-text.h5", h5pp::FileAccess::REPLACE);
    longForm(file);  // Show the canonical handle-based API
    shortForm(file); // Show the compatibility short form
    return 0;
}
