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

void longForm(h5pp::File &file) {
    // Write and read a single string.
    std::string singleString = "This is a dummy string";
    file.dataset("longForm/example-group/mySingleString").write(singleString);
    auto singleStringRead = file.dataset("longForm/example-group/mySingleString").read<std::string>();

    // We can write multiple strings at a time ...
    auto multipleStrings = makeMultipleStrings();
    file.dataset("longForm/example-group/multipleStrings").write(multipleStrings);

    // ... and read them back in one go.
    auto multipleStringsRead = file.dataset("longForm/example-group/multipleStrings").read<std::vector<std::string>>();

    // Or read them all into a single string, using \n as separator for each element.
    auto multipleStringsAsString = file.dataset("longForm/example-group/multipleStrings").read<std::string>();

    h5pp::print("Long form wrote: {}\n", singleString);
    h5pp::print("Long form read : {}\n", singleStringRead);
    h5pp::print("Long form multiple strings as vector of strings: \n{}\n", multipleStringsRead);
    h5pp::print("Long form multiple strings as string separated by \\n: \n{}\n", multipleStringsAsString);
}

void shortForm(h5pp::File &file) {
    // Write and read a single string.
    std::string singleString = "This is a dummy string";
    file.writeDataset(singleString, "shortForm/example-group/mySingleString");
    auto singleStringRead = file.readDataset<std::string>("shortForm/example-group/mySingleString");

    // We can write multiple strings at a time ...
    auto multipleStrings = makeMultipleStrings();
    file.writeDataset(multipleStrings, "shortForm/example-group/multipleStrings");

    // ... and read them back in one go.
    auto multipleStringsRead = file.readDataset<std::vector<std::string>>("shortForm/example-group/multipleStrings");

    // Or read them all into a single string, using \n as separator for each element.
    auto multipleStringsAsString = file.readDataset<std::string>("shortForm/example-group/multipleStrings");

    h5pp::print("Short form wrote: {}\n", singleString);
    h5pp::print("Short form read : {}\n", singleStringRead);
    h5pp::print("Short form multiple strings as vector of strings: \n{}\n", multipleStringsRead);
    h5pp::print("Short form multiple strings as string separated by \\n: \n{}\n", multipleStringsAsString);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02b-stdvector-text.h5", h5pp::FileAccess::REPLACE, 2);
    longForm(file);  // Show the canonical handle-based API
    shortForm(file); // Show the compatibility short form
    return 0;
}
