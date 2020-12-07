#include <h5pp/h5pp.h>
int main() {

    // Initialize a file
    h5pp::File file("exampledir/example-03a-attributes.h5", h5pp::FilePermission::REPLACE);

    // Write an integer to file

    file.writeDataset(42, "intGroup/myInt");

    // We can now write metadata, or "attributes" to the int which now exists on file
    file.writeAttribute("this is some info about my int", "myInt_stringAttribute", "intGroup/myInt");
    file.writeAttribute(3.14, "myInt_doubleAttribute", "intGroup/myInt");

    // List all attributes associated with our int.
    // The following will be printed:
    //      myInt_stringAttribute
    //      myInt_doubleAttribute
    auto allAttributes = file.getAttributeNames("intGroup/myInt");
    for(auto &attr : allAttributes) printf("%s\n",attr.c_str());

    // Read the attribute data back
    auto stringAttribute = file.readAttribute<std::string>("myInt_stringAttribute", "intGroup/myInt");
    auto doubleAttribute = file.readAttribute<double>("myInt_doubleAttribute", "intGroup/myInt");

    // Print the data
    h5pp::print("StringAttribute read: {}\n", stringAttribute);
    h5pp::print("doubleAttribute read: {}\n", doubleAttribute);

    return 0;
}