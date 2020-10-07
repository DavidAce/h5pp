#include <h5pp/h5pp.h>
// In this example we write text-data to a file in a
// In h5pp, each string is treated as a single "scalar" unit of variable-length
// Hence, size corresponds to the number of strings, not the length of a string.

int main() {
    // Initialize a file
    h5pp::File  file("exampledir/example-02b-text.h5", h5pp::FilePermission::REPLACE, 2);
    std::string singleString = "This is a dummy string";

    // Write to file
    file.writeDataset(singleString, "example-group/mySingleString");
    h5pp::print("h5pp wrote: {}\n",singleString);

    // Read from file
    auto singleStringRead = file.readDataset<std::string>("example-group/mySingleString");
    h5pp::print("h5pp read : {}\n",singleStringRead);

    // We can write multiple strings at a time ...
    std::vector<std::string> multipleStrings;
    multipleStrings.emplace_back("this is a vector");
    multipleStrings.emplace_back("of strings");
    multipleStrings.emplace_back("of varying lengths");
    file.writeDataset(multipleStrings, "example-group/multipleStrings");

    // and read them back in one go...
    auto multipleStringsRead = file.readDataset<std::vector<std::string>>("example-group/multipleStrings");
    h5pp::print("Multiple strings as vector of strings: \n{}\n",multipleStringsRead);

    // or read them all into a single string! We use \n as separator for each element.
    auto multipleStringsAsString = file.readDataset<std::string>("example-group/multipleStrings");
    h5pp::print("Multiple strings as string separated by \\n: \n{}\n",multipleStringsAsString);
    return 0;
}