#include <h5pp/h5pp.h>
#include <vector>

/*
 * This example introduces the rich AttrInfo returned by getInfo().
 *
 * Attributes are small pieces of metadata attached to datasets, groups, or files.
 * In v2, you inspect them through an attribute handle rather than using AttrInfo as the main write path.
 */
int main() {
    // Initialize a file.
    h5pp::File file(H5PP_EXAMPLE_DIR "example-05b-attribute-info.h5", h5pp::FileAccess::REPLACE);

    // Write a dataset and attach two attributes to it.
    file.dataset("group/dataset").write(42);
    file.attribute("group/dataset", "stringAttribute").write(std::string("this is some dummy string"));
    file.attribute("group/dataset", "vectorAttribute").write(std::vector<int>{1, 2, 3, 4});

    // Start with the string attribute.
    auto stringAttribute = file.attribute("group/dataset", "stringAttribute");
    auto attributeInfo   = stringAttribute.getInfo();
    if(attributeInfo.linkPath) h5pp::print("String attribute link : {}\n", attributeInfo.linkPath.value());
    if(attributeInfo.attrName) h5pp::print("String attribute name : {}\n", attributeInfo.attrName.value());
    if(attributeInfo.attrSize) h5pp::print("String attribute size : {}\n", attributeInfo.attrSize.value());
    if(attributeInfo.attrByte) h5pp::print("String attribute bytes: {}\n", attributeInfo.attrByte.value());
    if(attributeInfo.attrRank) h5pp::print("String attribute rank : {}\n", attributeInfo.attrRank.value());
    if(attributeInfo.attrDims) h5pp::print("String attribute dims : {}\n", attributeInfo.attrDims.value());

    // Then inspect the vector attribute.
    auto vectorAttribute = file.attribute("group/dataset", "vectorAttribute");
    attributeInfo        = vectorAttribute.getInfo();
    if(attributeInfo.linkPath) h5pp::print("Vector attribute link : {}\n", attributeInfo.linkPath.value());
    if(attributeInfo.attrName) h5pp::print("Vector attribute name : {}\n", attributeInfo.attrName.value());
    if(attributeInfo.attrSize) h5pp::print("Vector attribute size : {}\n", attributeInfo.attrSize.value());
    if(attributeInfo.attrByte) h5pp::print("Vector attribute bytes: {}\n", attributeInfo.attrByte.value());
    if(attributeInfo.attrRank) h5pp::print("Vector attribute rank : {}\n", attributeInfo.attrRank.value());
    if(attributeInfo.attrDims) h5pp::print("Vector attribute dims : {}\n", attributeInfo.attrDims.value());

    return 0;
}
