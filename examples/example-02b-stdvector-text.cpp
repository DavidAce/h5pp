#include <h5pp/h5pp.h>
#include <string>
#include <vector>

/*
 * In this example we write text data to a file.
 * In h5pp, each string is treated as a single scalar unit of variable length.
 * Hence, size corresponds to the number of strings, not the length of a string.
 */

namespace {
    std::vector<std::string> makeMultipleStrings() {
        std::vector<std::string> multipleStrings;
        multipleStrings.emplace_back("this is a vector");
        multipleStrings.emplace_back("of strings");
        multipleStrings.emplace_back("of varying lengths");
        return multipleStrings;
    }
}

void v2Api(h5pp::File &file) {
    // Write and read a single string.
    std::string singleString = "This is a dummy string";
    file.dataset("v2Api/example-group/mySingleString").write(singleString);
    auto singleStringRead = file.dataset("v2Api/example-group/mySingleString").read<std::string>();

    // We can write multiple strings at a time ...
    auto multipleStrings = makeMultipleStrings();
    file.dataset("v2Api/example-group/multipleStrings").write(multipleStrings);

    // ... and read them back in one go.
    auto multipleStringsRead = file.dataset("v2Api/example-group/multipleStrings").read<std::vector<std::string>>();

    // Or read them all into a single string, using \n as separator for each element.
    auto multipleStringsAsString = file.dataset("v2Api/example-group/multipleStrings").read<std::string>();

    h5pp::print("v2 API wrote: {}\n", singleString);
    h5pp::print("v2 API read : {}\n", singleStringRead);
    h5pp::print("v2 API multiple strings as vector of strings: \n{}\n", multipleStringsRead);
    h5pp::print("v2 API multiple strings as string separated by \\n: \n{}\n", multipleStringsAsString);
}

void v1Api(h5pp::File &file) {
    // Write and read a single string.
    std::string singleString = "This is a dummy string";
    file.writeDataset(singleString, "v1Api/example-group/mySingleString");
    auto singleStringRead = file.readDataset<std::string>("v1Api/example-group/mySingleString");

    // We can write multiple strings at a time ...
    auto multipleStrings = makeMultipleStrings();
    file.writeDataset(multipleStrings, "v1Api/example-group/multipleStrings");

    // ... and read them back in one go.
    auto multipleStringsRead = file.readDataset<std::vector<std::string>>("v1Api/example-group/multipleStrings");

    // Or read them all into a single string, using \n as separator for each element.
    auto multipleStringsAsString = file.readDataset<std::string>("v1Api/example-group/multipleStrings");

    h5pp::print("v1 API wrote: {}\n", singleString);
    h5pp::print("v1 API read : {}\n", singleStringRead);
    h5pp::print("v1 API multiple strings as vector of strings: \n{}\n", multipleStringsRead);
    h5pp::print("v1 API multiple strings as string separated by \\n: \n{}\n", multipleStringsAsString);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02b-stdvector-text.h5", h5pp::FileAccess::REPLACE, 2);
    v2Api(file);  // Show the v2 API
    v1Api(file); // Show the v1 API
    return 0;
}
