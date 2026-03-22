#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format("output/{}.h5", name);
    }

    void require_null_free(const std::string &value) {
        INFO("value size: " << value.size());
        REQUIRE(value.find('\0') == std::string::npos);
    }
}

TEST_CASE("Attribute names and fixed-size strings are read without embedded nulls", "[text]") {
    h5pp::File file(make_path("readWriteText-fixed"), H5F_ACC_TRUNC | H5F_ACC_RDWR, 0);

    file.writeDataset("nulltest", "nullDset");
    file.writeAttribute("this is a nulltest attribute", "nullDset", "nullAttr1");
    file.writeAttribute("this is a nulltest attribute", "nullDset", "nullAttr2");

    auto attr_names = file.getAttributeNames("nullDset");
    REQUIRE(attr_names.size() == 2);
    for(const auto &name : attr_names) {
        require_null_free(name);
        REQUIRE(name.size() == 9);
    }

    std::string fixed_string = "String with fixed size";
    file.writeDataset(fixed_string, "stringDummy_fixedSize", 23);
    auto fixed_read = file.readDataset<std::string>("stringDummy_fixedSize");
    require_null_free(fixed_read);
    REQUIRE(fixed_read == fixed_string);

    std::string fixed_attribute = "This is a dummy string attribute";
    file.writeAttribute(fixed_attribute, "stringDummy_fixedSize", "stringAttribute_fixed", fixed_attribute.size() + 1);
    auto attr_read_default  = file.readAttribute<std::string>("stringDummy_fixedSize", "stringAttribute_fixed");
    auto attr_read_explicit = file.readAttribute<std::string>("stringDummy_fixedSize", "stringAttribute_fixed", 33);
    require_null_free(attr_read_default);
    require_null_free(attr_read_explicit);
    REQUIRE(attr_read_default == fixed_attribute);
    REQUIRE(attr_read_explicit == fixed_attribute);
}

TEST_CASE("Vector-of-strings datasets and attributes preserve expected text semantics", "[text]") {
    h5pp::File file(make_path("readWriteText-vectors"), H5F_ACC_TRUNC | H5F_ACC_RDWR, 0);

    std::vector<std::string> vec_string = {"this is a variable", "length array"};
    file.writeDataset(vec_string, "vecString");

    auto vec_string_as_text = file.readDataset<std::string>("vecString");
    REQUIRE(vec_string_as_text == "this is a variable\nlength array");
    require_null_free(vec_string_as_text);

    auto vec_string_read = file.readDataset<std::vector<std::string>>("vecString");
    REQUIRE(vec_string_read == vec_string);
    for(const auto &value : vec_string_read) require_null_free(value);

    std::vector<std::string> string_vector_attribute = {"This is a variable length", "dummy string attribute"};
    file.writeAttribute(string_vector_attribute, "vecString", "stringVectorAttribute");
    auto attr_as_string = file.readAttribute<std::string>("vecString", "stringVectorAttribute");
    auto attr_as_vector = file.readAttribute<std::vector<std::string>>("vecString", "stringVectorAttribute");
    require_null_free(attr_as_string);
    REQUIRE(attr_as_string == "This is a variable length\ndummy string attribute");
    REQUIRE(attr_as_vector == string_vector_attribute);
    for(const auto &value : attr_as_vector) require_null_free(value);

    h5pp::hid::h5t custom_string = H5Tcopy(H5T_C_S1);
    H5Tset_size(custom_string, 5);
    H5Tset_strpad(custom_string, H5T_STR_NULLTERM);

    std::vector<std::string> fixed_upper    = {"this", "is", "a variable", "length", "vector", "with", "fixed", "upper size"};
    std::vector<std::string> fixed_expected = {"this", "is", "a va", "leng", "vect", "with", "fixe", "uppe"};
    file.writeDataset(fixed_upper, "vecStringFixed", custom_string);

    auto fixed_read = file.readDataset<std::vector<std::string>>("vecStringFixed");
    REQUIRE(fixed_read == fixed_expected);
    for(const auto &value : fixed_read) require_null_free(value);
}

TEST_CASE("Char arrays, string views and variable-length strings round-trip", "[text]") {
    h5pp::File file(make_path("readWriteText-misc"), H5F_ACC_TRUNC | H5F_ACC_RDWR, 0);

    char char_dummy[100] = "Dummy char array";
    file.writeDataset(char_dummy, "charDummy_variable");
    file.writeDataset(char_dummy, "charDummy_fixed_15", {15});
    file.writeDataset(char_dummy, "charDummy_fixed_16", {16});
    file.writeDataset(char_dummy, "charDummy_fixed_17", {17});
    file.writeDataset(char_dummy, "charDummy_fixed_18", {18});

    REQUIRE(file.readDataset<std::string>("charDummy_variable") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("charDummy_fixed_18") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("charDummy_fixed_17") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("charDummy_fixed_16") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("charDummy_fixed_15") == "Dummy char arra");

    std::string string_dummy = "Dummy string";
    std::string huge_string;
    for(size_t num = 0; num < 100; ++num) huge_string.append("This is a huge string line number: " + std::to_string(num) + "\n");

    file.writeDataset(string_dummy, "stringDummy");
    file.writeDataset(huge_string, "hugeString");
    file.writeDataset(char_dummy, "charDummy");
    file.writeDataset(char_dummy, "charDummy_dims", {17});
    file.writeDataset(char_dummy, "charDummy_size", 17);
    file.writeDataset("Dummy string literal", "literalDummy");

    REQUIRE(file.readDataset<std::string>("stringDummy") == string_dummy);
    REQUIRE(file.readDataset<std::string>("hugeString") == huge_string);
    REQUIRE(file.readDataset<std::string>("charDummy") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("charDummy_dims") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("charDummy_size") == "Dummy char array");
    REQUIRE(file.readDataset<std::string>("literalDummy") == "Dummy string literal");

    file.writeDataset(string_dummy, "stringDummy_extended");
    file.writeDataset("some other dummy text that makes it longer", "stringDummy_extended");
    REQUIRE(file.readDataset<std::string>("stringDummy_extended") == "some other dummy text that makes it longer");

    std::string              attribute       = "This is a dummy string attribute";
    std::vector<std::string> multi_attribute = {"This is another dummy string attribute", "With many elements"};
    file.writeAttribute(attribute, "stringDummy", "stringAttribute");
    file.writeAttribute(attribute, "stringDummy", "stringAttribute_fixed", attribute.size());
    file.writeAttribute(multi_attribute, "stringDummy", "multiStringAttribute");

    REQUIRE(file.readAttribute<std::string>("stringDummy", "stringAttribute") == attribute);
    REQUIRE(file.readAttribute<std::string>("stringDummy", "stringAttribute_fixed") == attribute);
    REQUIRE(file.readAttribute<std::vector<std::string>>("stringDummy", "multiStringAttribute") == multi_attribute);

    std::string_view string_view = "This is a string view";
    file.writeDataset(string_view, "stringView");
    REQUIRE(file.readDataset<std::string>("stringView") == string_view);

    h5pp::vstr_t vlen_string = "This is a variable-length string";
    file.writeDataset(vlen_string, "vlenString");
    REQUIRE(file.readDataset<h5pp::vstr_t>("vlenString") == vlen_string);

    std::vector<h5pp::vstr_t> vector_vlen_string = {{"This is "}, {"a variable-length string vector"}};
    file.writeDataset(vector_vlen_string, "vectorVlenString");
    REQUIRE(file.readDataset<std::vector<h5pp::vstr_t>>("vectorVlenString") == vector_vlen_string);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
