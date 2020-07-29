#include <h5pp/h5pp.h>
int main() {

    // Initialize a file
    h5pp::File file("exampledir/example-step5-attribute-info.h5", h5pp::FilePermission::REPLACE);

    // Write a dummy dataset to file
    file.writeDataset(42, "dummyGroup/dummyDataset");

    // Write some dummy attributes into the dataset
    file.writeAttribute("this a some dummy string", "dummyStringAttribute", "dummyGroup/dummyDataset");
    file.writeAttribute(std::vector<int>{1,2,3,4}, "dummyVectorAttribute", "dummyGroup/dummyDataset");

    // Let's start with the string attribute
    // Get a struct populated with information about the attribute
    auto stringAttributeInfo = file.getAttributeInfo("dummyGroup/dummyDataset", "dummyStringAttribute");

    // Access the properties of the attribute
    if(stringAttributeInfo.attrName) printf("String Attribute name : %s \n", stringAttributeInfo.attrName->c_str());
    if(stringAttributeInfo.attrSize) printf("String Attribute size : %llu \n", stringAttributeInfo.attrSize.value());
    if(stringAttributeInfo.attrByte) printf("String Attribute bytes: %lu \n", stringAttributeInfo.attrByte.value());
    if(stringAttributeInfo.attrRank) printf("String attribute rank : %d \n", stringAttributeInfo.attrRank.value());
    if(stringAttributeInfo.attrDims) {
        printf("String attribute dims : {");
        // Note that a single string is a scalar entry, so it has no extents and the following
        // should print nothing
        for(auto & dim: stringAttributeInfo.attrDims.value()) printf("%llu,", dim);
        printf("}\n");
    }

    // And so on... OR, just use .string() to get a pre-formatted string
    printf("info:  %s\n", stringAttributeInfo.string().c_str());



    // The second attribute is treated similarly
    // Get a struct populated with information about the attribute
    auto vectorAttributeInfo = file.getAttributeInfo("dummyGroup/dummyDataset", "dummyVectorAttribute");

    // Access the properties of the attribute
    if(vectorAttributeInfo.attrName) printf("Vector attribute name : %s \n", vectorAttributeInfo.attrName->c_str());
    if(vectorAttributeInfo.attrSize) printf("Vector attribute size : %llu \n", vectorAttributeInfo.attrSize.value());
    if(vectorAttributeInfo.attrByte) printf("Vector attribute bytes: %lu \n", vectorAttributeInfo.attrByte.value());
    if(vectorAttributeInfo.attrRank) printf("Vector attribute rank : %d \n", vectorAttributeInfo.attrRank.value());
    if(vectorAttributeInfo.attrDims) {
        printf("Vector attribute dims : {");
        // This time the vector has four elements in 1 dimension.
        for(auto & dim: vectorAttributeInfo.attrDims.value()) printf("%llu,", dim);
        printf("}\n");
    }

    // And so on... OR, just use .string() to get a pre-formatted string
    printf("info:  %s\n", vectorAttributeInfo.string().c_str());


    return 0;
}