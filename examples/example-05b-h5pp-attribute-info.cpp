#include <h5pp/h5pp.h>

/*
 * When reading and writing to attributes to file, h5pp scans HDF5 and C++ objects,
 * and populates a struct "AttrInfo", which is returned to the user.
 * The scanning process introduces some overhead, which is why reusing the
 * struct can be desirable to speed up repeated operations.
 *
 * This example shows how to use attribute metadata.
 * Attributes are written onto a dummy dataset and the attribute metadata
 * is obtained with file.getAttributeInfo(<link path>).
 */

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-05b-attribute-info.h5", h5pp::FilePermission::REPLACE);

    // Write a dummy dataset to file
    file.writeDataset(42, "dummyGroup/dummyDataset");

    // Write some dummy attributes into the dataset
    file.writeAttribute("this a some dummy string", "dummyStringAttribute", "dummyGroup/dummyDataset");
    file.writeAttribute(std::vector<int>{1, 2, 3, 4}, "dummyVectorAttribute", "dummyGroup/dummyDataset");

    // Let's start with the string attribute
    // Get a struct populated with information about the attribute
    auto stringAttributeInfo = file.getAttributeInfo("dummyGroup/dummyDataset", "dummyStringAttribute");

    // Access the properties of the attribute
    if(stringAttributeInfo.linkPath) h5pp::print("String attribute link : {}\n", stringAttributeInfo.linkPath.value());
    if(stringAttributeInfo.attrName) h5pp::print("String attribute name : {}\n", stringAttributeInfo.attrName.value());
    if(stringAttributeInfo.attrSize) h5pp::print("String attribute size : {}\n", stringAttributeInfo.attrSize.value());
    if(stringAttributeInfo.attrByte) h5pp::print("String attribute bytes: {}\n", stringAttributeInfo.attrByte.value());
    if(stringAttributeInfo.attrRank) h5pp::print("String attribute rank : {}\n", stringAttributeInfo.attrRank.value());
    if(stringAttributeInfo.attrDims)
        h5pp::print("String attribute dims : {}\n", stringAttributeInfo.attrDims.value()); // Scalar -- should print "{}"

    // And so on... OR, just use .string()
    h5pp::print("AttrInfo::string(): {}\n", stringAttributeInfo.string());

    // The second attribute is treated similarly
    // Get a struct populated with information about the attribute
    auto vectorAttributeInfo = file.getAttributeInfo("dummyGroup/dummyDataset", "dummyVectorAttribute");

    // Access the properties of the attribute
    if(vectorAttributeInfo.linkPath) h5pp::print("Vector attribute link : {}\n", vectorAttributeInfo.linkPath.value());
    if(vectorAttributeInfo.attrName) h5pp::print("Vector attribute name : {}\n", vectorAttributeInfo.attrName.value());
    if(vectorAttributeInfo.attrSize) h5pp::print("Vector attribute size : {}\n", vectorAttributeInfo.attrSize.value());
    if(vectorAttributeInfo.attrByte) h5pp::print("Vector attribute bytes: {}\n", vectorAttributeInfo.attrByte.value());
    if(vectorAttributeInfo.attrRank) h5pp::print("Vector attribute rank : {}\n", vectorAttributeInfo.attrRank.value());
    if(vectorAttributeInfo.attrDims) h5pp::print("Vector attribute dims : {}\n", vectorAttributeInfo.attrDims.value());

    // And so on... OR, just use .string()
    h5pp::print("AttrInfo::string(): {}\n", vectorAttributeInfo.string());

    return 0;
}