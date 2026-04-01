#include <h5pp/h5pp.h>
#include <string>

/*
 * An attribute is similar to a dataset, but can be attached to datasets, groups or files.
 * Attributes are often used to store descriptive metadata.
 *
 * This example shows how to add a couple of attributes to a dataset "intGroup/myInt".
 */

void longForm(h5pp::File &file) {
    // Write an integer to file.
    file.dataset("longForm/intGroup/myInt").write(42);

    // We can now add attributes to the dataset.
    file.dataset("longForm/intGroup/myInt").attribute("myInt_stringAttribute").write(std::string("this is some info about my int"));
    file.dataset("longForm/intGroup/myInt").attribute("myInt_doubleAttribute").write(3.14);

    // List all attributes associated with our dataset.
    h5pp::print("Long form attribute names: {}\n", file.link("longForm/intGroup/myInt").getAttributeNames());

    // Read the attribute data back.
    auto stringAttribute = file.dataset("longForm/intGroup/myInt").attribute("myInt_stringAttribute").read<std::string>();
    auto doubleAttribute = file.dataset("longForm/intGroup/myInt").attribute("myInt_doubleAttribute").read<double>();

    // Print the data.
    h5pp::print("Long form stringAttribute read: {}\n", stringAttribute);
    h5pp::print("Long form doubleAttribute read: {}\n", doubleAttribute);
}

void shortForm(h5pp::File &file) {
    // Write an integer to file.
    file.writeDataset(42, "shortForm/intGroup/myInt");

    // We can now add attributes to the dataset.
    file.writeAttribute("shortForm/intGroup/myInt", "myInt_stringAttribute", std::string("this is some info about my int"));
    file.writeAttribute("shortForm/intGroup/myInt", "myInt_doubleAttribute", 3.14);

    // List all attributes associated with our dataset.
    h5pp::print("Short form attribute names: {}\n", file.link("shortForm/intGroup/myInt").getAttributeNames());

    // Read the attribute data back.
    auto stringAttribute = file.readAttribute<std::string>("shortForm/intGroup/myInt", "myInt_stringAttribute");
    auto doubleAttribute = file.readAttribute<double>("shortForm/intGroup/myInt", "myInt_doubleAttribute");

    // Print the data.
    h5pp::print("Short form stringAttribute read: {}\n", stringAttribute);
    h5pp::print("Short form doubleAttribute read: {}\n", doubleAttribute);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-03a-attributes-readwrite.h5", h5pp::FileAccess::REPLACE);
    longForm(file);  // Show the canonical handle-based API
    shortForm(file); // Show the compatibility short form
    return 0;
}
