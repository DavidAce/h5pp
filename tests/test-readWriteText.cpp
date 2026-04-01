#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }

    void require_null_free(const std::string &value) {
        INFO("value size: " << value.size());
        REQUIRE(value.find('\0') == std::string::npos);
    }
}

TEST_CASE("Attribute names and fixed-size strings are read without embedded nulls", "[text]") {
    h5pp::File file(make_path("readWriteText-fixed"), H5F_ACC_TRUNC | H5F_ACC_RDWR, 0);

    file.writeDataset("nulltest", "nullDset");
    file.writeAttribute("nullDset", "nullAttr1", std::string("this is a nulltest attribute"));
    file.writeAttribute("nullDset", "nullAttr2", std::string("this is a nulltest attribute"));

    auto attrNames = file.link("nullDset").getAttributeNames();
    REQUIRE(attrNames.size() == 2);
    for(const auto &name : attrNames) {
        require_null_free(name);
        REQUIRE(name.size() == 9);
    }

    std::string fixedString = "String with fixed size";
    h5pp::DatasetWriteOptions fixedStringOptions;
    fixedStringOptions.dims = std::vector<hsize_t>{23};
    file.writeDataset(fixedString, "stringDummy_fixedSize", fixedStringOptions);
    auto fixedRead = file.readDataset<std::string>("stringDummy_fixedSize");
    require_null_free(fixedRead);
    REQUIRE(fixedRead == fixedString);

    std::string attribute = "This is a dummy string attribute";
    h5pp::AttributeWriteOptions fixedAttrWriteOptions;
    fixedAttrWriteOptions.dims = std::vector<hsize_t>{attribute.size() + 1};
    file.writeAttribute("stringDummy_fixedSize", "stringAttribute_fixed", attribute, fixedAttrWriteOptions);

    auto attrReadDefault = file.readAttribute<std::string>("stringDummy_fixedSize", "stringAttribute_fixed");
    h5pp::AttributeReadOptions fixedAttrReadOptions;
    fixedAttrReadOptions.dims = std::vector<hsize_t>{33};
    auto attrReadExplicit = file.readAttribute<std::string>("stringDummy_fixedSize", "stringAttribute_fixed", fixedAttrReadOptions);
    require_null_free(attrReadDefault);
    require_null_free(attrReadExplicit);
    REQUIRE(attrReadDefault == attribute);
    REQUIRE(attrReadExplicit == attribute);
}

TEST_CASE("Vector-of-strings datasets and attributes preserve expected text semantics", "[text]") {
    h5pp::File file(make_path("readWriteText-vectors"), H5F_ACC_TRUNC | H5F_ACC_RDWR, 0);

    std::vector<std::string> vecString = {"this is a variable", "length array"};
    file.writeDataset(vecString, "vecString");

    auto vecStringAsText = file.readDataset<std::string>("vecString");
    REQUIRE(vecStringAsText == "this is a variable\nlength array");
    require_null_free(vecStringAsText);

    auto vecStringRead = file.readDataset<std::vector<std::string>>("vecString");
    REQUIRE(vecStringRead == vecString);
    for(const auto &value : vecStringRead) require_null_free(value);

    std::vector<std::string> stringVectorAttribute = {"This is a variable length", "dummy string attribute"};
    file.writeAttribute("vecString", "stringVectorAttribute", stringVectorAttribute);
    auto attrAsString = file.readAttribute<std::string>("vecString", "stringVectorAttribute");
    auto attrAsVector = file.readAttribute<std::vector<std::string>>("vecString", "stringVectorAttribute");
    require_null_free(attrAsString);
    REQUIRE(attrAsString == "This is a variable length\ndummy string attribute");
    REQUIRE(attrAsVector == stringVectorAttribute);
    for(const auto &value : attrAsVector) require_null_free(value);

    h5pp::hid::h5t customString = H5Tcopy(H5T_C_S1);
    H5Tset_size(customString, 5);
    H5Tset_strpad(customString, H5T_STR_NULLTERM);

    std::vector<std::string> fixedUpper    = {"this", "is", "a variable", "length", "vector", "with", "fixed", "upper size"};
    std::vector<std::string> fixedExpected = {"this", "is", "a va", "leng", "vect", "with", "fixe", "uppe"};
    h5pp::DatasetCreateOptions fixedCreateOptions;
    fixedCreateOptions.h5Type = customString;
    file.writeDataset(fixedUpper, "vecStringFixed", fixedCreateOptions);

    auto fixedRead = file.readDataset<std::vector<std::string>>("vecStringFixed");
    REQUIRE(fixedRead == fixedExpected);
    for(const auto &value : fixedRead) require_null_free(value);
}

TEST_CASE("Char arrays, string views and variable-length strings round-trip", "[text]") {
    h5pp::File file(make_path("readWriteText-misc"), H5F_ACC_TRUNC | H5F_ACC_RDWR, 0);

    char charDummy[100] = "Dummy char array";
    file.writeDataset(charDummy, "charDummy_variable");

    h5pp::DatasetWriteOptions char15;
    char15.dims = std::vector<hsize_t>{15};
    file.writeDataset(charDummy, "charDummy_fixed_15", char15);
    h5pp::DatasetWriteOptions char16;
    char16.dims = std::vector<hsize_t>{16};
    file.writeDataset(charDummy, "charDummy_fixed_16", char16);
    h5pp::DatasetWriteOptions char17;
    char17.dims = std::vector<hsize_t>{17};
    file.writeDataset(charDummy, "charDummy_fixed_17", char17);
    h5pp::DatasetWriteOptions char18;
    char18.dims = std::vector<hsize_t>{18};
    file.writeDataset(charDummy, "charDummy_fixed_18", char18);

    REQUIRE(file.readDataset<std::string>("charDummy_variable") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("charDummy_fixed_18") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("charDummy_fixed_17") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("charDummy_fixed_16") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("charDummy_fixed_15") == "Dummy char arra");

    std::string stringDummy = "Dummy string";
    std::string hugeString;
    for(size_t num = 0; num < 100; ++num) hugeString.append("This is a huge string line number: " + std::to_string(num) + "\n");

    file.writeDataset(stringDummy, "stringDummy");
    file.writeDataset(hugeString, "hugeString");
    file.writeDataset(charDummy, "charDummy");
    file.writeDataset(charDummy, "charDummy_dims", char17);
    file.writeDataset(charDummy, "charDummy_size", char17);
    file.writeDataset("Dummy string literal", "literalDummy");

    REQUIRE(file.readDataset<std::string>("stringDummy") == stringDummy);
    REQUIRE(file.readDataset<std::string>("hugeString") == hugeString);
    REQUIRE(file.readDataset<std::string>("charDummy") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("charDummy_dims") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("charDummy_size") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("literalDummy") == "Dummy string literal");

    file.writeDataset(stringDummy, "stringDummy_extended");
    file.writeDataset("some other dummy text that makes it longer", "stringDummy_extended");
    REQUIRE(file.readDataset<std::string>("stringDummy_extended") == "some other dummy text that makes it longer");

    std::string              attribute      = "This is a dummy string attribute";
    std::vector<std::string> multiAttribute = {"This is another dummy string attribute", "With many elements"};
    file.writeAttribute("stringDummy", "stringAttribute", attribute);
    h5pp::AttributeWriteOptions fixedStringAttributeOptions;
    fixedStringAttributeOptions.dims = std::vector<hsize_t>{attribute.size()};
    file.writeAttribute("stringDummy", "stringAttribute_fixed", attribute, fixedStringAttributeOptions);
    file.writeAttribute("stringDummy", "multiStringAttribute", multiAttribute);

    REQUIRE(file.readAttribute<std::string>("stringDummy", "stringAttribute") == attribute);
    REQUIRE(file.readAttribute<std::string>("stringDummy", "stringAttribute_fixed") == attribute);
    REQUIRE(file.readAttribute<std::vector<std::string>>("stringDummy", "multiStringAttribute") == multiAttribute);

    std::string_view stringView = "This is a string view";
    file.writeDataset(stringView, "stringView");
    REQUIRE(file.readDataset<std::string>("stringView") == stringView);

    h5pp::vstr_t vlenString = "This is a variable-length string";
    file.writeDataset(vlenString, "vlenString");
    REQUIRE(file.readDataset<h5pp::vstr_t>("vlenString") == vlenString);

    std::vector<h5pp::vstr_t> vectorVlenString = {{"This is "}, {"a variable-length string vector"}};
    file.writeDataset(vectorVlenString, "vectorVlenString");
    REQUIRE(file.readDataset<std::vector<h5pp::vstr_t>>("vectorVlenString") == vectorVlenString);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
