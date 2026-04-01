#include <h5pp/h5pp.h>
#include <typeindex>
#include <vector>

/*
 * An attribute is similar to a dataset, but can be appended to other HDF5 objects like datasets, groups or files.
 * Users can append any number of attributes to a given HDF5 object. A common use case for attributes
 * is to add descriptive metadata to an HDF5 object.
 *
 * This example shows how to get all attributes from a link and print their contents.
 */

size_t getSizeFromDims(const h5pp::v2::OptDims &dims) {
    if(not dims or dims->empty()) return 1;
    size_t size = 1;
    for(auto dim : dims.value()) size *= static_cast<size_t>(dim);
    return size;
}

struct LongFormAttributeReader {
    h5pp::File       &file;
    std::string_view  linkPath;

    template<typename T>
    T operator()(std::string_view attrName) const {
        return file.attribute(linkPath, attrName).template read<T>();
    }
};

struct ShortFormAttributeReader {
    h5pp::File       &file;
    std::string_view  linkPath;

    template<typename T>
    T operator()(std::string_view attrName) const {
        return file.readAttribute<T>(linkPath, attrName);
    }
};

template<typename ReadAttribute>
void traverseAttributes(h5pp::File &file, std::string_view linkPath, ReadAttribute readAttribute) {
    for(const auto &attrName : file.link(linkPath).getAttributeNames()) {
        auto info = file.attribute(linkPath, attrName).getInfo();
        if(not info.h5Type) continue;

        auto [cppTypeIndex, cppTypeName, cppTypeBytes] = h5pp::type::getCppType(info.h5Type.value());
        auto h5Size = getSizeFromDims(info.attrDims);
        auto message = h5pp::format("Attribute {}::{} [{} | size {}]", linkPath, attrName, cppTypeName, h5Size);
        [[maybe_unused]] auto bytes = cppTypeBytes;

        if(cppTypeIndex == typeid(std::string) and h5Size == 1)
            h5pp::print("{} = {}\n", message, readAttribute.template operator()<std::string>(attrName));
        if(cppTypeIndex == typeid(int) and h5Size == 1)
            h5pp::print("{} = {}\n", message, readAttribute.template operator()<int>(attrName));
        if(cppTypeIndex == typeid(int) and h5Size > 1)
            h5pp::print("{} = {}\n", message, readAttribute.template operator()<std::vector<int>>(attrName));
        if(cppTypeIndex == typeid(double) and h5Size == 1)
            h5pp::print("{} = {}\n", message, readAttribute.template operator()<double>(attrName));
        if(cppTypeIndex == typeid(double) and h5Size > 1)
            h5pp::print("{} = {}\n", message, readAttribute.template operator()<std::vector<double>>(attrName));

        // ... and so on. In practice, it is simpler to create templated helper functions to avoid code duplication here.
    }
}

void longForm(h5pp::File &file) {
    // Write a dummy integer to file.
    file.dataset("longForm/intGroup/myInt").write(42);

    // Write some attributes to the dataset.
    file.dataset("longForm/intGroup/myInt").attribute("myInt_stringAttribute").write(std::string("this is some info about my int"));
    file.dataset("longForm/intGroup/myInt").attribute("myInt_doubleAttribute").write(3.14);
    file.dataset("longForm/intGroup/myInt").attribute("myInt_vectorAttribute").write(std::vector<int>{1, 2, 3, 4});

    // Now we imagine wanting to iterate through all attributes of "longForm/intGroup/myInt" and print their contents.
    // The most pressing concern is that we may not know beforehand what type we should read data into.
    // This has no simple solution, since C++ is statically typed. We have to check a list of types during runtime.
    traverseAttributes(file, "longForm/intGroup/myInt", LongFormAttributeReader{file, "longForm/intGroup/myInt"});
}

void shortForm(h5pp::File &file) {
    // Write a dummy integer to file.
    file.writeDataset(42, "shortForm/intGroup/myInt");

    // Write some attributes to the dataset.
    file.writeAttribute("shortForm/intGroup/myInt", "myInt_stringAttribute", std::string("this is some info about my int"));
    file.writeAttribute("shortForm/intGroup/myInt", "myInt_doubleAttribute", 3.14);
    file.writeAttribute("shortForm/intGroup/myInt", "myInt_vectorAttribute", std::vector<int>{1, 2, 3, 4});

    traverseAttributes(file, "shortForm/intGroup/myInt", ShortFormAttributeReader{file, "shortForm/intGroup/myInt"});
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-03b-attributes-traverse.h5", h5pp::FileAccess::REPLACE);
    longForm(file);  // Show the canonical handle-based API
    shortForm(file); // Show the compatibility short form
    return 0;
}

/* Program output:

Attribute longForm/intGroup/myInt::myInt_stringAttribute [std::__cxx11::basic_string<char> | size 1] = this is some info about my int
Attribute longForm/intGroup/myInt::myInt_doubleAttribute [double | size 1] = 3.14
Attribute longForm/intGroup/myInt::myInt_vectorAttribute [int | size 4] = [1, 2, 3, 4]

Attribute shortForm/intGroup/myInt::myInt_stringAttribute [std::__cxx11::basic_string<char> | size 1] = this is some info about my int
Attribute shortForm/intGroup/myInt::myInt_doubleAttribute [double | size 1] = 3.14
Attribute shortForm/intGroup/myInt::myInt_vectorAttribute [int | size 4] = [1, 2, 3, 4]

*/
