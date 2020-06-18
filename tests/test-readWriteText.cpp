#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <h5pp/h5pp.h>
#include <iostream>

/*! \brief Prints the content of a vector nicely */
template<typename T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
    if(!v.empty()) {
        out << "[ ";
        std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, " "));
        out << "]";
    }
    return out;
}

// Store some dummy data to an hdf5 file
TEST_CASE("Multiline string compared", "[text]") {
    std::string outputFilename2 = "output/readWriteText.h5";
    size_t      logLevel2       = 0;
    h5pp::File  file2(outputFilename2, H5F_ACC_TRUNC | H5F_ACC_RDWR, logLevel2);
    REQUIRE_THAT(file2.readDataset<std::string>("vecString"), Catch::Matchers::Equals("this is a variable\nlength array"));
}

int main() {
    std::string outputFilename = "output/readWriteText.h5";
    size_t      logLevel       = 0;
    h5pp::File  file(outputFilename, H5F_ACC_TRUNC | H5F_ACC_RDWR, logLevel);

    char charDummyTemp[100] = "Dummy char array";
    file.writeDataset(charDummyTemp, "charDummy");
    file.writeDataset(charDummyTemp, "charDummy_dims",{17});

    auto charDummy_dims_temp = file.readDataset<std::string>("charDummy_dims");
    if(strcmp(charDummyTemp, charDummy_dims_temp.c_str()) != 0) throw std::runtime_error(h5pp::format("Char dummy given dims failed: [{}] != [{}]", charDummyTemp, charDummy_dims_temp));

    std::vector<std::string> vecString;
    vecString.emplace_back("this is a variable");
    vecString.emplace_back("length array");
    file.writeDataset(vecString, "vecString");
    auto vecStringReadString = file.readDataset<std::string>("vecString");

    if(vecStringReadString != "this is a variable\nlength array") throw std::runtime_error(h5pp::format("String mismatch: [{}] != [{}]", vecString, vecStringReadString));
    auto vecstringReadVector = file.readDataset<std::vector<std::string>>("vecString");
    if(vecstringReadVector.size() != vecString.size())
        throw std::runtime_error(h5pp::format("Vecstring read size mismatch: [{}] != [{}]", vecString.size(), vecstringReadVector.size()));
    for(size_t i = 0; i < vecString.size(); i++)
        if(vecString[i] != vecstringReadVector[i]) throw std::runtime_error(h5pp::format("Vecstring read element mismatch: [{}] != [{}]", vecString[i], vecstringReadVector[i]));

    // Generate dummy data
    std::string stringDummy = "Dummy string";
    std::string hugeString;
    for(size_t num = 0; num < 100; num++) hugeString.append("This is a huge string line number: " + std::to_string(num) + "\n");
    char charDummy[100] = "Dummy char array";

    // Write text data in various ways
    file.writeDataset(stringDummy, "stringDummy");
    file.writeDataset(hugeString, "hugeString");

    // The following shouldn't work
//    try {
//        file.writeDataset(charDummy, {2}, "charDummy_asarray_dims",{2}, std::nullopt,{});
//    } catch(std::exception &err) { std::cout << "Expected error: " << err.what() << std::endl; }

    auto stringDummyRead = file.readDataset<std::string>("stringDummy");
    if(stringDummy != stringDummyRead) throw std::runtime_error(h5pp::format("String dummy failed: [{}] != [{}]", stringDummy, stringDummyRead));

    file.writeDataset(charDummy, "charDummy");
    auto charDummyRead = file.readDataset<std::string>("charDummy");
    if(strcmp(charDummy, charDummyRead.c_str()) != 0) throw std::runtime_error(h5pp::format("Char dummy failed: [{}] != [{}]", charDummy, charDummyRead));

    file.writeDataset(charDummy, "charDummy_dims", {17});
    auto charDummy_dims = file.readDataset<std::string>("charDummy_dims");
    if(strcmp(charDummy, charDummy_dims.c_str()) != 0) throw std::runtime_error(h5pp::format("Char dummy given dims failed: [{}] != [{}]", charDummy, charDummy_dims));

    file.writeDataset(charDummy, "charDummy_size",17);
    auto charDummy_size = file.readDataset<std::string>("charDummy_size");
    if(strcmp(charDummy, charDummy_size.c_str()) != 0) throw std::runtime_error(h5pp::format("Char dummy given size failed: [{}] != [{}]", charDummy, charDummy_size));

    file.writeDataset("Dummy string literal", "literalDummy");
    auto literalDummy = file.readDataset<std::string>("literalDummy");
    if("Dummy string literal" != literalDummy) throw std::runtime_error(h5pp::format("Literal dummy failed: [{}] != [{}]", "Dummy string literal", literalDummy));

    // Now try some outlier cases

    // Write text data in various ways
    file.writeDataset(stringDummy, "stringDummy_chunked", {stringDummy.size()}, H5D_CHUNKED);

    file.writeDataset(stringDummy, "stringDummy_extended");
    file.writeDataset("some other dummy text that makes it longer", "stringDummy_extended");
    auto stringDummy_extended = file.readDataset<std::string>("stringDummy_extended");
    if(stringDummy_extended != "some other dummy text that makes it longer") throw std::runtime_error(h5pp::format("Failed to extend string: [{}]", stringDummy_extended));

    file.writeDataset("String with fixed size", "stringDummy_fixedSize",23);
    auto stringDummy_fixedSize = file.readDataset<std::string>("stringDummy_fixedSize");
    if(stringDummy_fixedSize != "String with fixed size") throw std::runtime_error(h5pp::format("Failed to write fixed-size string: [{}]", stringDummy_fixedSize));

    // Now let's try some text attributes
    std::string stringAttribute = "This is a dummy string attribute";
    file.writeAttribute(stringAttribute, "stringAttribute", "vecString");
    auto stringAttributeRead = file.readAttribute<std::string>("stringAttribute", "vecString");
    std::cout << "\nstringAttribute read:\n" << stringAttributeRead << std::endl;
    if(stringAttribute != stringAttributeRead) throw std::runtime_error(h5pp::format("stringAttribute failed: [{}] != [{}]", stringAttribute, stringAttributeRead));

    std::vector<std::string> multiStringAttribute = {"This is another dummy string attribute", "With many elements"};
    file.writeAttribute(multiStringAttribute, "multiStringAttribute", "vecString");
    auto multiStringAttributeRead = file.readAttribute<std::vector<std::string>>("multiStringAttribute", "vecString");
    std::cout << "\nmultiStringAttribute read: " << std::endl;
    for(auto &elem : multiStringAttributeRead) std::cout << elem << std::endl;

    if(multiStringAttributeRead.size() != multiStringAttribute.size())
        throw std::runtime_error(h5pp::format("multiStringAttribute read size mismatch: [{}] != [{}]", multiStringAttribute.size(), multiStringAttributeRead.size()));
    for(size_t i = 0; i < multiStringAttribute.size(); i++)
        if(multiStringAttribute[i] != multiStringAttributeRead[i])
            throw std::runtime_error(h5pp::format("Vecstring read element mismatch: [{}] != [{}]", multiStringAttribute[i], multiStringAttributeRead[i]));

    // Test string views
    std::string_view stringView = "This is a string view";
    file.writeDataset(stringView, "stringView");
    auto stringViewRead = file.readDataset<std::string>("stringView");
    if(stringView != stringViewRead) throw std::runtime_error(h5pp::format("string view mismatch:  [{}] != [{}]", stringView, stringViewRead));

    return 0;
}