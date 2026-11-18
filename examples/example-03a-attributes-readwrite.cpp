#include <h5pp/h5pp.h>
#include <string>

/*
 * An attribute is similar to a dataset, but can be attached to datasets, groups or files.
 * Attributes are often used to store descriptive metadata.
 *
 * This example shows how to add a couple of attributes to a dataset "intGroup/myInt".
 */

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-03a-attributes-readwrite.h5", h5pp::FileAccess::REPLACE);

    // Write an integer to file.
    file.dataset("v2Api/intGroup/myInt").write(42);

    // We can now add attributes to the dataset.
    file.dataset("v2Api/intGroup/myInt").attribute("myInt_stringAttribute").write(std::string("this is some info about my int"));
    file.dataset("v2Api/intGroup/myInt").attribute("myInt_doubleAttribute").write(3.14);

    // List all attributes associated with our dataset.
    h5pp::print("v2 API attribute names: {}\n", file.link("v2Api/intGroup/myInt").getAttributeNames());

    // Read the attribute data back.
    auto stringAttribute = file.dataset("v2Api/intGroup/myInt").attribute("myInt_stringAttribute").read<std::string>();
    auto doubleAttribute = file.dataset("v2Api/intGroup/myInt").attribute("myInt_doubleAttribute").read<double>();

    // Print the data.
    h5pp::print("v2 API stringAttribute read: {}\n", stringAttribute);
    h5pp::print("v2 API doubleAttribute read: {}\n", doubleAttribute);
    return 0;
}
