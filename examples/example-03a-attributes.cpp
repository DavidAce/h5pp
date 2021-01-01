#include <h5pp/h5pp.h>

/*
 * An attribute is similar to a dataset, but can be appended to other HDF5 objects like datasets, groups or files.
 * Users can append any number of attributes to a given HDF5 object. A common use case for attributes
 * is to add descriptive metadata to an HDF5 object.
 *
 * This example shows how to add a couple of attributes to a dataset "intGroup/myInt"
 */

int main() {

    // Initialize a file
    h5pp::File file("exampledir/example-03a-attributes.h5", h5pp::FilePermission::REPLACE);

    // Write an integer to file

    file.writeDataset(42, "intGroup/myInt");

    // We can now add attributes to the dataset
    file.writeAttribute("this is some info about my int", "myInt_stringAttribute", "intGroup/myInt");
    file.writeAttribute(3.14, "myInt_doubleAttribute", "intGroup/myInt");

    // List all attributes associated with our dataset. The following will be printed:
    //      {"myInt_stringAttribute", "myInt_doubleAttribute"}
    h5pp::print("{}\n",file.getAttributeNames("intGroup/myInt"));


    // Read the attribute data back
    auto stringAttribute = file.readAttribute<std::string>("myInt_stringAttribute", "intGroup/myInt");
    auto doubleAttribute = file.readAttribute<double>("myInt_doubleAttribute", "intGroup/myInt");

    // Print the data
    h5pp::print("StringAttribute read: {}\n", stringAttribute);
    h5pp::print("doubleAttribute read: {}\n", doubleAttribute);

    return 0;
}