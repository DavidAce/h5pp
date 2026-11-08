#include <h5pp/h5pp.h>
#include <string>

/*
 * An attribute is similar to a dataset, but can be attached to datasets, groups or files.
 * Attributes are often used to store descriptive metadata.
 *
 * This example shows how to add a couple of attributes to a dataset "intGroup/myInt".
 */

void v2Api(h5pp::File &file) {
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
}

void v1Api(h5pp::File &file) {
    // Write an integer to file.
    file.writeDataset(42, "v1Api/intGroup/myInt");

    // We can now add attributes to the dataset.
    file.writeAttribute("v1Api/intGroup/myInt", "myInt_stringAttribute", std::string("this is some info about my int"));
    file.writeAttribute("v1Api/intGroup/myInt", "myInt_doubleAttribute", 3.14);

    // List all attributes associated with our dataset.
    h5pp::print("v1 API attribute names: {}\n", file.link("v1Api/intGroup/myInt").getAttributeNames());

    // Read the attribute data back.
    auto stringAttribute = file.readAttribute<std::string>("v1Api/intGroup/myInt", "myInt_stringAttribute");
    auto doubleAttribute = file.readAttribute<double>("v1Api/intGroup/myInt", "myInt_doubleAttribute");

    // Print the data.
    h5pp::print("v1 API stringAttribute read: {}\n", stringAttribute);
    h5pp::print("v1 API doubleAttribute read: {}\n", doubleAttribute);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-03a-attributes-readwrite.h5", h5pp::FileAccess::REPLACE);
    v2Api(file);  // Show the v2 API
    v1Api(file); // Show the v1 API
    return 0;
}
