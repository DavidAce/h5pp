#include <h5pp/h5pp.h>

/*
 *
 * This example introduces the AttrInfo struct.
 *
 * An attribute is similar to a dataset, but can be appended to other HDF5 objects like datasets, groups or files.
 * Users can append any number of attributes to a given HDF5 object. A common use case for attributes
 * is to add descriptive metadata to an HDF5 object.
 *
 * When transferring attribute data to/from file h5pp scans the attribute type, shape, link path
 * and many other such properties. The results from a scan populates a struct of type "AttrInfo".

 * The AttrInfo of an attribute can be obtained with h5pp::File::getAttributeInfo(<link path>,<attr name>),
 * but is also returned from a h5pp::File::writeAttribute(...) operation.
 *
 * The scanning process introduces some overhead, which is why reusing the
 * struct can be desirable, in particular to speed up repeated operations.
 *
 */

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-05b-attribute-info.h5", h5pp::FilePermission::REPLACE);

    // Write a dataset to file
    file.writeDataset(42, "group/dataset");

    // Write some attributes into the dataset
    file.writeAttribute("this a some dummy string", "stringAttribute", "group/dataset");
    file.writeAttribute(std::vector<int>{1, 2, 3, 4}, "vectorAttribute", "group/dataset");

    // Let's start with the string attribute
    // Get a struct populated with information about the attribute
    auto attrInfo = file.getAttributeInfo("group/dataset", "stringAttribute");

    // Access the properties of the attribute
    if(attrInfo.linkPath) h5pp::print("String attribute link : {}\n", attrInfo.linkPath.value());
    if(attrInfo.attrName) h5pp::print("String attribute name : {}\n", attrInfo.attrName.value());
    if(attrInfo.attrSize) h5pp::print("String attribute size : {}\n", attrInfo.attrSize.value());
    if(attrInfo.attrByte) h5pp::print("String attribute bytes: {}\n", attrInfo.attrByte.value());
    if(attrInfo.attrRank) h5pp::print("String attribute rank : {}\n", attrInfo.attrRank.value());
    if(attrInfo.attrDims) h5pp::print("String attribute dims : {}\n", attrInfo.attrDims.value()); // A string is scalar: should print "{}"

    // And so on... OR, just use .string()
    h5pp::print("attrInfo.string(): {}\n", attrInfo.string());

    // The second attribute is treated similarly
    // Get a struct populated with information about the attribute
    attrInfo = file.getAttributeInfo("group/dataset", "vectorAttribute");

    // Access the properties of the attribute
    if(attrInfo.linkPath) h5pp::print("Vector attribute link : {}\n", attrInfo.linkPath.value());
    if(attrInfo.attrName) h5pp::print("Vector attribute name : {}\n", attrInfo.attrName.value());
    if(attrInfo.attrSize) h5pp::print("Vector attribute size : {}\n", attrInfo.attrSize.value());
    if(attrInfo.attrByte) h5pp::print("Vector attribute bytes: {}\n", attrInfo.attrByte.value());
    if(attrInfo.attrRank) h5pp::print("Vector attribute rank : {}\n", attrInfo.attrRank.value());
    if(attrInfo.attrDims) h5pp::print("Vector attribute dims : {}\n", attrInfo.attrDims.value());

    // And so on... OR, just use .string()
    h5pp::print("attrInfo.string(): {}\n", attrInfo.string());

    return 0;
}