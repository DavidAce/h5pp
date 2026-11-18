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

    file.dataset("nullDset").write("nulltest");
    file.attribute("nullDset", "nullAttr1").write(std::string("this is a nulltest attribute"));
    file.attribute("nullDset", "nullAttr2").write(std::string("this is a nulltest attribute"));

    auto attrNames = file.link("nullDset").getAttributeNames();
    REQUIRE(attrNames.size() == 2);
    for(const auto &name : attrNames) {
        require_null_free(name);
        REQUIRE(name.size() == 9);
    }

    std::string fixedString = "String with fixed size";
    h5pp::DatasetWriteOptions fixedStringOptions;
    fixedStringOptions.dims = std::vector<hsize_t>{23};
    file.dataset("stringDummy_fixedSize").write(fixedString, fixedStringOptions);
    auto fixedRead = file.dataset("stringDummy_fixedSize").read<std::string>();
    require_null_free(fixedRead);
    REQUIRE(fixedRead == fixedString);

    std::string attribute = "This is a dummy string attribute";
    h5pp::AttributeWriteOptions fixedAttrWriteOptions;
    fixedAttrWriteOptions.dims = std::vector<hsize_t>{attribute.size() + 1};
    file.attribute("stringDummy_fixedSize", "stringAttribute_fixed").write(attribute, fixedAttrWriteOptions);

    auto                       attrReadDefault = file.attribute("stringDummy_fixedSize", "stringAttribute_fixed").read<std::string>();
    h5pp::AttributeReadOptions fixedAttrReadOptions;
    fixedAttrReadOptions.dims = std::vector<hsize_t>{33};
    auto attrReadExplicit     = file.attribute("stringDummy_fixedSize", "stringAttribute_fixed").read<std::string>(fixedAttrReadOptions);
    require_null_free(attrReadDefault);
    require_null_free(attrReadExplicit);
    REQUIRE(attrReadDefault == attribute);
    REQUIRE(attrReadExplicit == attribute);
}

TEST_CASE("Vector-of-strings datasets and attributes preserve expected text semantics", "[text]") {
    h5pp::File file(make_path("readWriteText-vectors"), H5F_ACC_TRUNC | H5F_ACC_RDWR, 0);

    std::vector<std::string> vecString = {"this is a variable", "length array"};
    file.dataset("vecString").write(vecString);

    auto vecStringAsText = file.dataset("vecString").read<std::string>();
    REQUIRE(vecStringAsText == "this is a variable\nlength array");
    require_null_free(vecStringAsText);

    auto vecStringRead = file.dataset("vecString").read<std::vector<std::string>>();
    REQUIRE(vecStringRead == vecString);
    for(const auto &value : vecStringRead) require_null_free(value);

    std::vector<std::string> stringVectorAttribute = {"This is a variable length", "dummy string attribute"};
    file.attribute("vecString", "stringVectorAttribute").write(stringVectorAttribute);
    auto attrAsString = file.attribute("vecString", "stringVectorAttribute").read<std::string>();
    auto attrAsVector = file.attribute("vecString", "stringVectorAttribute").read<std::vector<std::string>>();
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
    file.dataset("vecStringFixed").create(fixedUpper, fixedCreateOptions).write(fixedUpper);

    auto fixedRead = file.dataset("vecStringFixed").read<std::vector<std::string>>();
    REQUIRE(fixedRead == fixedExpected);
    for(const auto &value : fixedRead) require_null_free(value);
}

TEST_CASE("Char arrays, string views and variable-length strings round-trip", "[text]") {
    h5pp::File file(make_path("readWriteText-misc"), H5F_ACC_TRUNC | H5F_ACC_RDWR, 0);

    char charDummy[100] = "Dummy char array";
    file.dataset("charDummy_variable").write(charDummy);

    h5pp::DatasetWriteOptions char15;
    char15.dims = std::vector<hsize_t>{15};
    file.dataset("charDummy_fixed_15").write(charDummy, char15);
    h5pp::DatasetWriteOptions char16;
    char16.dims = std::vector<hsize_t>{16};
    file.dataset("charDummy_fixed_16").write(charDummy, char16);
    h5pp::DatasetWriteOptions char17;
    char17.dims = std::vector<hsize_t>{17};
    file.dataset("charDummy_fixed_17").write(charDummy, char17);
    h5pp::DatasetWriteOptions char18;
    char18.dims = std::vector<hsize_t>{18};
    file.dataset("charDummy_fixed_18").write(charDummy, char18);

    REQUIRE(file.dataset("charDummy_variable").read<std::string>() == "Dummy char array");
    REQUIRE(file.dataset("charDummy_fixed_18").read<std::string>() == "Dummy char array");
    REQUIRE(file.dataset("charDummy_fixed_17").read<std::string>() == "Dummy char array");
    REQUIRE(file.dataset("charDummy_fixed_16").read<std::string>() == "Dummy char array");
    REQUIRE(file.dataset("charDummy_fixed_15").read<std::string>() == "Dummy char arra");

    std::string stringDummy = "Dummy string";
    std::string hugeString;
    for(size_t num = 0; num < 100; ++num) hugeString.append("This is a huge string line number: " + std::to_string(num) + "\n");

    file.dataset("stringDummy").write(stringDummy);
    file.dataset("hugeString").write(hugeString);
    file.dataset("charDummy").write(charDummy);
    file.dataset("charDummy_dims").write(charDummy, char17);
    file.dataset("charDummy_size").write(charDummy, char17);
    file.dataset("literalDummy").write("Dummy string literal");

    REQUIRE(file.dataset("stringDummy").read<std::string>() == stringDummy);
    REQUIRE(file.dataset("hugeString").read<std::string>() == hugeString);
    REQUIRE(file.dataset("charDummy").read<std::string>() == "Dummy char array");
    REQUIRE(file.dataset("charDummy_dims").read<std::string>() == "Dummy char array");
    REQUIRE(file.dataset("charDummy_size").read<std::string>() == "Dummy char array");
    REQUIRE(file.dataset("literalDummy").read<std::string>() == "Dummy string literal");

    file.dataset("stringDummy_extended").write(stringDummy);
    file.dataset("stringDummy_extended").write("some other dummy text that makes it longer");
    REQUIRE(file.dataset("stringDummy_extended").read<std::string>() == "some other dummy text that makes it longer");

    std::string              attribute      = "This is a dummy string attribute";
    std::vector<std::string> multiAttribute = {"This is another dummy string attribute", "With many elements"};
    file.attribute("stringDummy", "stringAttribute").write(attribute);
    h5pp::AttributeWriteOptions fixedStringAttributeOptions;
    fixedStringAttributeOptions.dims = std::vector<hsize_t>{attribute.size()};
    file.attribute("stringDummy", "stringAttribute_fixed").write(attribute, fixedStringAttributeOptions);
    file.attribute("stringDummy", "multiStringAttribute").write(multiAttribute);

    REQUIRE(file.attribute("stringDummy", "stringAttribute").read<std::string>() == attribute);
    REQUIRE(file.attribute("stringDummy", "stringAttribute_fixed").read<std::string>() == attribute);
    REQUIRE(file.attribute("stringDummy", "multiStringAttribute").read<std::vector<std::string>>() == multiAttribute);

    std::string_view stringView = "This is a string view";
    file.dataset("stringView").write(stringView);
    REQUIRE(file.dataset("stringView").read<std::string>() == stringView);

    h5pp::vstr_t vlenString = "This is a variable-length string";
    file.dataset("vlenString").write(vlenString);
    REQUIRE(file.dataset("vlenString").read<h5pp::vstr_t>() == vlenString);

    std::vector<h5pp::vstr_t> vectorVlenString = {{"This is "}, {"a variable-length string vector"}};
    file.dataset("vectorVlenString").write(vectorVlenString);
    REQUIRE(file.dataset("vectorVlenString").read<std::vector<h5pp::vstr_t>>() == vectorVlenString);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
