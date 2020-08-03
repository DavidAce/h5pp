#include <h5pp/h5pp.h>
#include <iostream>
// In this example we write text-data to a file in a
// In h5pp, each string is treated as a single "scalar" unit of variable-length
// Hence, size corresponds to the number of strings, not the length of a string.

int main() {
    // Initialize a file
    h5pp::File  file("exampledir/example-02b-text.h5", h5pp::FilePermission::REPLACE, 0);
    std::string singleString = "This is a dummy string";

    // Write to file
    file.writeDataset(singleString, "example-group/mySingleString");

    // Read from file
    auto singleStringRead = file.readDataset<std::string>("example-group/mySingleString");

    // Print it
    std::cout << "h5pp wrote: " << singleString << std::endl;
    std::cout << "h5pp read : " << singleStringRead << std::endl;

    // We can write multiple strings at a time ...
    std::vector<std::string> multipleStrings;
    multipleStrings.emplace_back("this is a vector");
    multipleStrings.emplace_back("of strings");
    multipleStrings.emplace_back("of varying lengths");
    file.writeDataset(multipleStrings, "example-group/multipleStrings");

    // and read them back in one go...
    auto multipleStringsRead = file.readDataset<std::vector<std::string>>("example-group/multipleStrings");
    std::cout << "Multiple strings as vector\n";
    for(auto &str : multipleStringsRead) std::cout << str << std::endl;

    // or read them all into a single string! We use \n as separator for each element.
    auto multipleStringsAsString = file.readDataset<std::string>("example-group/multipleStrings");
    std::cout << "Multiple strings as string: \n" << multipleStringsAsString << std::endl;

    return 0;
}