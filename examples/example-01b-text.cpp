#include <h5pp/h5pp.h>
#include <string>

/*
 * In this example we use h5pp to write and read an std::string.
 * By default, h5pp uses UTF-8 and stores std::string data as a variable-length array, i.e. H5T_VARIABLE.
 * For fixed-size datasets specify the size of the std::string buffer in the
 * dataset options when writing.
 */

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-01b-text.h5", h5pp::FileAccess::REPLACE);

    // Initialize some text
    std::string writeText = "Hello world";

    // Write and read data using the v2 API.
    file.dataset("v2Api/stringData").write(writeText);
    std::string readText = file.dataset("v2Api/stringData").read<std::string>();

    h5pp::print("v2 API wrote dataset: {}\n", writeText);
    h5pp::print("v2 API read  dataset: {}\n", readText);
    return 0;
}
