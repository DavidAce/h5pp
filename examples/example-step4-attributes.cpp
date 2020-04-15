#include <h5pp/h5pp.h>
#include <iostream>
int main() {

    // Initialize a file
    h5pp::File file("exampledir/example4.h5", h5pp::FilePermission::REPLACE);
    // Write an integer to file
    file.writeDataset(42, "intGroup/myInt");
    // We can now write metadata, or "attributes" to the int.
    file.writeAttribute("this is some info about my int", "myInt_stringAttribute", "intGroup/myInt");
    file.writeAttribute(3.14, "myInt_doubleAttribute", "intGroup/myInt");

    // List all attributes associated with our int.
    // The following will print:
    //      myInt_stringAttribute
    //      myInt_doubleAttribute
    auto allAttributes = file.getAttributeNames("intGroup/myInt");
    for(auto &attr : allAttributes) std::cout << attr << std::endl;

    // Read the attribute data back
    auto stringAttribute = file.readAttribute<std::string>("myInt_stringAttribute", "intGroup/myInt");
    auto doubleAttribute = file.readAttribute<double>("myInt_doubleAttribute", "intGroup/myInt");
    std::cout << "StringAttribute = " << stringAttribute << std::endl;
    std::cout << "doubleAttribute = " << doubleAttribute << std::endl;
    return 0;
}