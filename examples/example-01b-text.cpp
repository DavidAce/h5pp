#include <h5pp/h5pp.h>
#include <string>

/*
 * In this example we use h5pp to write and read an std::string.
 * By default, h5pp uses UTF-8 and stores std::string data as a variable-length array, i.e. H5T_VARIABLE.
 * For fixed-size datasets use const char * or specify the size of the std::string buffer in the
 * dataset options when writing.
 */

void v2Api(h5pp::File &file) {
    // Initialize some text
    std::string writeText = "Hello world";

    // Write and read data using the v2 API.
    file.dataset("v2Api/stringData").write(writeText);
    std::string readText = file.dataset("v2Api/stringData").read<std::string>();

    h5pp::print("v2 API wrote dataset: {}\n", writeText);
    h5pp::print("v2 API read  dataset: {}\n", readText);
}

void v1Api(h5pp::File &file) {
    // Initialize some text
    std::string writeText = "Hello world";

    // Write and read data using the v1 API.
    file.writeDataset(writeText, "v1Api/stringData");
    auto readText = file.readDataset<std::string>("v1Api/stringData");

    h5pp::print("v1 API wrote dataset: {}\n", writeText);
    h5pp::print("v1 API read  dataset: {}\n", readText);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-01b-text.h5", h5pp::FileAccess::REPLACE);
    v2Api(file);  // Show the v2 API
    v1Api(file); // Show the v1 API
    return 0;
}
