#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <h5pp/h5pp.h>

void assert_nullfree(const std::string &s) {
    std::string s_esc;
    bool        found_null = false;
    for(size_t i = 0; i < s.size(); i++) {
        char ch = s[i];
        switch(ch) {
            case '\0':
                s_esc.append("\\0");
                found_null = true;
                break;
            case '\'': s_esc.append("\\'"); break;
            case '\"': s_esc.append("\\\""); break;
            case '\?': s_esc.append("\\?"); break;
            case '\\': s_esc.append("\\\\"); break;
            case '\a': s_esc.append("\\a"); break;
            case '\b': s_esc.append("\\b"); break;
            case '\f': s_esc.append("\\f"); break;
            case '\n': s_esc.append("\\n"); break;
            case '\r': s_esc.append("\\r"); break;
            case '\t': s_esc.append("\\t"); break;
            case '\v': s_esc.append("\\v"); break;
            default: s_esc += ch;
        }
    }
    if(found_null) {
        h5pp::print("print before esc: {} | size {}", s.c_str(), s.size());
        h5pp::print("print after  esc: {} | size {}", s_esc.c_str(), s_esc.size());
        throw std::runtime_error("Found null character");
    }
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

    /* New test
     * file.getAttributeNames was returning strings where the null terminator character was included
     * in the size of the string. For instance the std::string "xDMRG/state_1" should have size 13
     * because std::string::size does not include the hidden null terminator and rightly so.
     * However getAttributeNames returns an std::vector with strings where the size is 1 larger,
     * which causes the strings to include the null terminator on concatenations later.
     *
     * This test is designed to make sure that this problem is fixed.
     */

    file.writeDataset("nulltest", "nullDset");
    file.writeAttribute("this is a nulltest attribute", "nullDset", "nullAttr1");
    file.writeAttribute("this is a nulltest attribute", "nullDset", "nullAttr2");

    auto attrNames = file.getAttributeNames("nullDset");
    for(const auto &str : attrNames) {
        assert_nullfree(str);
        if(str.size() != 9) throw std::runtime_error("Attribute name has the wrong size");
    }

    std::string stringDummy_fixedSize_t = "String with fixed size";

    file.writeDataset(stringDummy_fixedSize_t, "stringDummy_fixedSize", 23);
    auto stringDummy_fixedSize_t_read = file.readDataset<std::string>("stringDummy_fixedSize");
    assert_nullfree(stringDummy_fixedSize_t_read);
    if(stringDummy_fixedSize_t_read != stringDummy_fixedSize_t)
        throw std::runtime_error(
            h5pp::format("String with fixed size 23 failed: [{}] != [{}]", stringDummy_fixedSize_t, stringDummy_fixedSize_t_read));

    std::string stringAttribute_t = "This is a dummy string attribute";
    file.writeAttribute(stringAttribute_t, "stringDummy_fixedSize", "stringAttribute_fixed", stringAttribute_t.size() + 1);
    auto stringAttributeRead_fixed_t = file.readAttribute<std::string>("stringDummy_fixedSize", "stringAttribute_fixed");
    assert_nullfree(stringAttributeRead_fixed_t);
    stringAttributeRead_fixed_t = file.readAttribute<std::string>("stringDummy_fixedSize", "stringAttribute_fixed", 33);
    assert_nullfree(stringAttributeRead_fixed_t);
    h5pp::print("stringAttributeRead_fixed_t: {}\n", stringAttributeRead_fixed_t);
    if(stringAttribute_t != stringAttributeRead_fixed_t)
        throw std::runtime_error(
            h5pp::format("stringAttributeRead_fixed failed: [{}] != [{}]", stringAttribute_t, stringAttributeRead_fixed_t));

    std::vector<std::string> stringVectorAttribute_t = {"This is a variable length", "dummy string attribute"};
    file.writeAttribute(stringVectorAttribute_t, "stringDummy_fixedSize", "stringVectorAttribute_t");

    auto stringVectorAttributeRead_string = file.readAttribute<std::string>("stringDummy_fixedSize", "stringVectorAttribute_t");
    assert_nullfree(stringVectorAttributeRead_string);
    h5pp::print("stringVectorAttributeRead_string: {}\n", stringVectorAttributeRead_string);

    auto stringVectorAttributeRead_vector =
        file.readAttribute<std::vector<std::string>>("stringDummy_fixedSize", "stringVectorAttribute_t");
    for(const auto &str : stringVectorAttributeRead_vector) {
        assert_nullfree(str);
        h5pp::print("str: {}\n", str);

    }

    h5pp::hid::h5t custom_string = H5Tcopy(H5T_C_S1);
    H5Tset_size(custom_string, 5);
    H5Tset_strpad(custom_string, H5T_STR_NULLTERM);
    std::vector<std::string> vlenvec  = {"this", "is", "a variable", "length", "vector", "with", "fixed", "upper size"};
    std::vector<std::string> vlenvec5 = {"this", "is", "a va", "leng", "vect", "with", "fixe", "uppe"};
    file.writeDataset(vlenvec, "vecStringFixed", custom_string);
    auto vlenvecRead = file.readDataset<std::vector<std::string>>("vecStringFixed");
    if(vlenvecRead != vlenvec5) throw std::runtime_error(h5pp::format("vlenvec failed: {} != {}", vlenvec5, vlenvecRead));
    for(const auto &str : vlenvecRead) assert_nullfree(str);

    char charDummyTemp[100] = "Dummy char array";
    file.writeDataset(charDummyTemp, "charDummy_variable");
    file.writeDataset(charDummyTemp, "charDummy_fixed_15", {15});
    file.writeDataset(charDummyTemp, "charDummy_fixed_16", {16});
    file.writeDataset(charDummyTemp, "charDummy_fixed_17", {17});
    file.writeDataset(charDummyTemp, "charDummy_fixed_18", {18});
    auto charDummy_variable = file.readDataset<std::string>("charDummy_variable");
    auto charDummy_fixed_18 = file.readDataset<std::string>("charDummy_fixed_18");
    auto charDummy_fixed_17 = file.readDataset<std::string>("charDummy_fixed_17");
    auto charDummy_fixed_16 = file.readDataset<std::string>("charDummy_fixed_16");
    auto charDummy_fixed_15 = file.readDataset<std::string>("charDummy_fixed_15");

    char charDummyTe16[17] = "Dummy char array";
    char charDummyTe15[16] = "Dummy char arra";
    if(strncmp(charDummyTemp, charDummy_variable.c_str(), 17) != 0)
        throw std::runtime_error(h5pp::format("Char dummy with variable length failed: [{}] != [{}]", charDummyTemp, charDummy_variable));
    if(strncmp(charDummyTemp, charDummy_fixed_18.c_str(), 18) != 0)
        throw std::runtime_error(h5pp::format("Char dummy with fixed size 18 failed: [{}] != [{}]", charDummyTemp, charDummy_fixed_18));
    if(strncmp(charDummyTemp, charDummy_fixed_17.c_str(), 17) != 0)
        throw std::runtime_error(h5pp::format("Char dummy with fixed size 17 failed: [{}] != [{}]", charDummyTemp, charDummy_fixed_17));
    if(strncmp(charDummyTe16, charDummy_fixed_16.c_str(), 16) != 0)
        throw std::runtime_error(h5pp::format("Char dummy with fixed size 16 failed: [{}] != [{}]", charDummyTe16, charDummy_fixed_16));
    if(strncmp(charDummyTe15, charDummy_fixed_15.c_str(), 15) != 0)
        throw std::runtime_error(h5pp::format("Char dummy with fixed size 15 failed: [{}] != [{}]", charDummyTe15, charDummy_fixed_15));

    std::vector<std::string> vecString;
    vecString.emplace_back("this is a variable");
    vecString.emplace_back("length array");
    file.writeDataset(vecString, "vecString");
    auto vecStringReadString = file.readDataset<std::string>("vecString");

    if(vecStringReadString != "this is a variable\nlength array")
        throw std::runtime_error(h5pp::format("String mismatch: [{}] != [{}]", vecString, vecStringReadString));
    auto vecstringReadVector = file.readDataset<std::vector<std::string>>("vecString");
    if(vecstringReadVector.size() != vecString.size())
        throw std::runtime_error(h5pp::format("Vecstring read size mismatch: [{}] != [{}]", vecString.size(), vecstringReadVector.size()));
    for(size_t i = 0; i < vecString.size(); i++)
        if(vecString[i] != vecstringReadVector[i])
            throw std::runtime_error(h5pp::format("Vecstring read element mismatch: [{}] != [{}]", vecString[i], vecstringReadVector[i]));

    // Generate dummy data
    std::string stringDummy = "Dummy string";
    std::string hugeString;
    for(size_t num = 0; num < 100; num++) hugeString.append("This is a huge string line number: " + std::to_string(num) + "\n");
    char charDummy[100] = "Dummy char array";

    // Write text data in various ways
    file.writeDataset(stringDummy, "stringDummy");
    file.writeDataset(hugeString, "hugeString");

    auto stringDummyRead = file.readDataset<std::string>("stringDummy");
    if(stringDummy != stringDummyRead)
        throw std::runtime_error(h5pp::format("String dummy failed: [{}] != [{}]", stringDummy, stringDummyRead));

    file.writeDataset(charDummy, "charDummy");
    auto charDummyRead = file.readDataset<std::string>("charDummy");
    if(strcmp(charDummy, charDummyRead.c_str()) != 0)
        throw std::runtime_error(h5pp::format("Char dummy failed: [{}] != [{}]", charDummy, charDummyRead));

    file.writeDataset(charDummy, "charDummy_dims", {17});
    auto charDummy_dims = file.readDataset<std::string>("charDummy_dims");
    if(strcmp(charDummy, charDummy_dims.c_str()) != 0)
        throw std::runtime_error(h5pp::format("Char dummy given dims failed: [{}] != [{}]", charDummy, charDummy_dims));

    file.writeDataset(charDummy, "charDummy_size", 17);
    auto charDummy_size = file.readDataset<std::string>("charDummy_size");
    if(strcmp(charDummy, charDummy_size.c_str()) != 0)
        throw std::runtime_error(h5pp::format("Char dummy given size failed: [{}] != [{}]", charDummy, charDummy_size));

    file.writeDataset("Dummy string literal", "literalDummy");
    auto literalDummy = file.readDataset<std::string>("literalDummy");
    if("Dummy string literal" != literalDummy)
        throw std::runtime_error(h5pp::format("Literal dummy failed: [{}] != [{}]", "Dummy string literal", literalDummy));

    // Now try some outlier cases

    // Write text data in various ways
    file.writeDataset(stringDummy, "stringDummy_chunked", {stringDummy.size()}, H5D_CHUNKED);

    file.writeDataset(stringDummy, "stringDummy_extended");
    file.writeDataset("some other dummy text that makes it longer", "stringDummy_extended");
    auto stringDummy_extended = file.readDataset<std::string>("stringDummy_extended");
    if(stringDummy_extended != "some other dummy text that makes it longer")
        throw std::runtime_error(h5pp::format("Failed to extend string: [{}]", stringDummy_extended));

    std::string stringDummy_fixedSize = "String with fixed size";
    file.writeDataset(stringDummy_fixedSize, "stringDummy_fixedSize", 23);
    auto stringDummy_fixedSize_read = file.readDataset<std::string>("stringDummy_fixedSize");
    if(stringDummy_fixedSize_read != stringDummy_fixedSize)
        throw std::runtime_error(
            h5pp::format("String with fixed size 23 failed: [{}] != [{}]", stringDummy_fixedSize, stringDummy_fixedSize_read));

    // Now let's try some text attributes
    std::string stringAttribute = "This is a dummy string attribute";
    file.writeAttribute(stringAttribute, "vecString", "stringAttribute");
    auto stringAttributeRead = file.readAttribute<std::string>("vecString", "stringAttribute");
    h5pp::print("\nstringAttributeRead: {}\n", stringAttributeRead);

    if(stringAttribute != stringAttributeRead)
        throw std::runtime_error(h5pp::format("stringAttribute failed: [{}] != [{}]", stringAttribute, stringAttributeRead));

    std::vector<std::string> multiStringAttribute = {"This is another dummy string attribute", "With many elements"};
    file.writeAttribute(multiStringAttribute, "vecString", "multiStringAttribute");
    auto multiStringAttributeRead = file.readAttribute<std::vector<std::string>>("vecString", "multiStringAttribute");
    h5pp::print("multiStringAttributeRead: {}\n", multiStringAttributeRead);

    if(multiStringAttributeRead.size() != multiStringAttribute.size())
        throw std::runtime_error(h5pp::format("multiStringAttribute read size mismatch: [{}] != [{}]",
                                              multiStringAttribute.size(),
                                              multiStringAttributeRead.size()));
    for(size_t i = 0; i < multiStringAttribute.size(); i++)
        if(multiStringAttribute[i] != multiStringAttributeRead[i])
            throw std::runtime_error(
                h5pp::format("Vecstring read element mismatch: [{}] != [{}]", multiStringAttribute[i], multiStringAttributeRead[i]));

    file.writeAttribute(stringAttribute, "vecString", "stringAttribute_fixed", stringAttribute.size());
    auto stringAttributeRead_fixed = file.readAttribute<std::string>("vecString", "stringAttribute_fixed");
    if(stringAttribute != stringAttributeRead_fixed)
        throw std::runtime_error(
            h5pp::format("stringAttributeRead_fixed failed: [{}] != [{}]", stringAttribute, stringAttributeRead_fixed));

    // Test string views
    std::string_view stringView = "This is a string view";
    file.writeDataset(stringView, "stringView");
    auto stringViewRead = file.readDataset<std::string>("stringView");
    if(stringView != stringViewRead)
        throw std::runtime_error(h5pp::format("string view mismatch:  [{}] != [{}]", stringView, stringViewRead));


    // Test vstr_t for variable-length text
    h5pp::vstr_t vlenString = "This is a variable-length string";
    file.writeDataset(vlenString, "vlenString");
    auto vlenStringRead = file.readDataset<h5pp::vstr_t>("vlenString");
    if(vlenString != vlenStringRead)
        throw std::runtime_error(h5pp::format("vlen string mismatch:  [{}] != [{}]", vlenString, vlenStringRead));

    std::vector<h5pp::vstr_t> vectorVlenString = {{"This is "},{"a variable-length string vector"}};
    file.writeDataset(vectorVlenString, "vectorVlenString");
    auto vectorVlenStringRead = file.readDataset<std::vector<h5pp::vstr_t>>("vectorVlenString");
    if(vectorVlenString != vectorVlenStringRead)
        throw std::runtime_error(h5pp::format("vector vlen string mismatch:  [{}] != [{}]", vectorVlenString, vectorVlenStringRead));

    return 0;
}