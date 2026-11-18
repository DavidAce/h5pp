#include <algorithm>
#include <array>
#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/h5pp.h>
#include <string>
#include <string_view>
#include <typeindex>
#include <utility>
#include <vector>

namespace {
    std::string make_path(std::string_view name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }

    void seed_type_info_file(const std::string &path) {
        int                               attribute_int                   = 7;
        double                            attribute_double                = 47.4;
        std::complex<int>                 attribute_complex_int           = {47, -10};
        std::complex<double>              attribute_complex_double        = {47.2, -10.2445};
        std::array<long, 4>               attribute_array_long            = {1, 2, 3, 4};
        float                             attribute_carray_float[4]       = {1, 2, 3, 4};
        std::vector<std::complex<double>> attribute_vector_complex_double = {{2.0, 5.0}, {3.1, -2.3}, {3.0, 0.0}, {-51.2, 5.0}};
        std::vector<double>               attribute_vector_double         = {1.0,  0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,
                                                                             0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 0.0, 0.0, 0.0,
                                                                             -1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0};
        std::string                       attribute_string               = "This is a very long string that I am testing";
        char                              attribute_char_array[]         = "This is a char array";

        h5pp::File file(path, h5pp::FileAccess::REPLACE, 0);
        file.writeDataset(std::vector<double>(10, 5.0), "testGroup/vectorDouble");

        file.writeAttribute("testGroup/vectorDouble", "AttributeInt", attribute_int);
        file.writeAttribute("testGroup/vectorDouble", "AttributeDouble", attribute_double);
        file.writeAttribute("testGroup/vectorDouble", "AttributeComplexInt", attribute_complex_int);
        file.writeAttribute("testGroup/vectorDouble", "AttributeComplexDouble", attribute_complex_double);
        file.writeAttribute("testGroup/vectorDouble", "AttributeArrayLong", attribute_array_long);
        file.writeAttribute("testGroup/vectorDouble", "AttributeCArrayFloat", attribute_carray_float);
        file.writeAttribute("testGroup/vectorDouble", "AttributeVectorDouble", attribute_vector_double);
        file.writeAttribute("testGroup/vectorDouble", "AttributeVectorComplexDouble", attribute_vector_complex_double);
        file.writeAttribute("testGroup/vectorDouble", "AttributeString", attribute_string);
        file.writeAttribute("testGroup/vectorDouble", "AttributeCharArray", attribute_char_array);
    }
}

TEST_CASE("Type info exposes dataset metadata for stored objects", "[type-info]") {
    static_assert(
        h5pp::type::sfinae::has_data_v<std::vector<double>> &&
        "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    auto path = make_path("typeInfo");
    seed_type_info_file(path);
    h5pp::File file(path, h5pp::FileAccess::READONLY, 0);

    auto info = file.dataset("testGroup/vectorDouble").getTypeInfo();
    REQUIRE(info.cppTypeIndex.has_value());
    REQUIRE(info.cppTypeIndex.value() == std::type_index(typeid(double)));
    REQUIRE(info.cppTypeBytes.has_value());
    REQUIRE(info.cppTypeBytes.value() == sizeof(double));
    REQUIRE(info.h5Path.has_value());
    REQUIRE(info.h5Size.has_value());
    REQUIRE(info.h5Rank.has_value());
    REQUIRE(info.h5Dims.has_value());
    REQUIRE(info.h5Type.has_value());
    REQUIRE(info.h5Path.value().find("testGroup/vectorDouble") != std::string::npos);
    if(info.h5Name.has_value()) REQUIRE(info.h5Name.value() == "vectorDouble");
    REQUIRE(info.h5Size.value() == 10);
    REQUIRE(info.h5Rank.value() == 1);
    REQUIRE(info.h5Dims.value() == std::vector<hsize_t>{10});
    REQUIRE(H5Tget_class(info.h5Type.value()) == H5T_FLOAT);
    REQUIRE_FALSE(info.string().empty());
}

TEST_CASE("Type info exposes per-attribute metadata across scalar, array and string attributes", "[type-info]") {
    auto path = make_path("typeInfo-attrs");
    seed_type_info_file(path);
    h5pp::File file(path, h5pp::FileAccess::READONLY, 0);

    auto infos = file.dataset("testGroup/vectorDouble").getTypeInfoAttributes();
    REQUIRE(infos.size() == 10);

    std::vector<std::string> names;
    names.reserve(infos.size());
    for(const auto &info : infos) {
        REQUIRE(info.h5Name.has_value());
        REQUIRE(info.h5Path.has_value());
        REQUIRE(info.h5Type.has_value());
        names.emplace_back(info.h5Name.value());
    }

    std::sort(names.begin(), names.end());
    REQUIRE(names == std::vector<std::string>{"AttributeArrayLong",
                                              "AttributeCArrayFloat",
                                              "AttributeCharArray",
                                              "AttributeComplexDouble",
                                              "AttributeComplexInt",
                                              "AttributeDouble",
                                              "AttributeInt",
                                              "AttributeString",
                                              "AttributeVectorComplexDouble",
                                              "AttributeVectorDouble"});

    auto attr_int = file.attribute("testGroup/vectorDouble", "AttributeInt").getTypeInfo();
    REQUIRE(attr_int.cppTypeIndex.has_value());
    REQUIRE(attr_int.cppTypeIndex.value() == std::type_index(typeid(int)));
    REQUIRE(attr_int.h5Rank.has_value());
    REQUIRE(attr_int.h5Rank.value() == 0);
    REQUIRE(H5Tget_class(attr_int.h5Type.value()) == H5T_INTEGER);

    auto attr_vector = file.attribute("testGroup/vectorDouble", "AttributeVectorComplexDouble").getTypeInfo();
    REQUIRE(attr_vector.cppTypeIndex.has_value());
    REQUIRE(attr_vector.cppTypeIndex.value() == std::type_index(typeid(std::complex<double>)));
    REQUIRE(attr_vector.cppTypeBytes.has_value());
    REQUIRE(attr_vector.cppTypeBytes.value() == sizeof(std::complex<double>));
    REQUIRE(attr_vector.h5Dims.has_value());
    REQUIRE(attr_vector.h5Dims.value() == std::vector<hsize_t>{4});
    REQUIRE(attr_vector.h5Rank.has_value());
    REQUIRE(attr_vector.h5Rank.value() == 1);
    REQUIRE(H5Tget_class(attr_vector.h5Type.value()) == H5T_COMPOUND);

    auto attr_string = file.attribute("testGroup/vectorDouble", "AttributeString").getTypeInfo();
    REQUIRE(attr_string.h5Name.has_value());
    REQUIRE(attr_string.h5Name.value() == "AttributeString");
    REQUIRE(H5Tis_variable_str(attr_string.h5Type.value()) > 0);
}

TEST_CASE("Datatype class names describe the standard HDF5 classes", "[type-info]") {
    const std::pair<H5T_class_t, const char *> classes[] = {
        {  H5T_INTEGER,       "H5T_INTEGER"},
        {    H5T_FLOAT,         "H5T_FLOAT"},
        {     H5T_TIME,          "H5T_TIME"},
        {   H5T_STRING,        "H5T_STRING"},
        { H5T_BITFIELD,      "H5T_BITFIELD"},
        {   H5T_OPAQUE,        "H5T_OPAQUE"},
        { H5T_COMPOUND,      "H5T_COMPOUND"},
        {H5T_REFERENCE,     "H5T_REFERENCE"},
        {     H5T_ENUM,          "H5T_ENUM"},
        {     H5T_VLEN,          "H5T_VLEN"},
        {    H5T_ARRAY,         "H5T_ARRAY"},
        { H5T_NO_CLASS, "H5T_UNKNOWN_CLASS"}
    };
    for(const auto &[typeClass, expected] : classes) {
        CAPTURE(expected);
        CHECK(h5pp::type::getH5ClassName(typeClass) == expected);
    }
}

TEST_CASE("Big-endian integer descriptions retain their stored representation", "[type-info]") {
    const std::pair<hid_t, const char *> types[] = {
        {H5T_STD_I16BE, "H5T_STD_I16BE"},
        {H5T_STD_I32BE, "H5T_STD_I32BE"},
        {H5T_STD_I64BE, "H5T_STD_I64BE"},
        {H5T_STD_U16BE, "H5T_STD_U16BE"},
        {H5T_STD_U32BE, "H5T_STD_U32BE"},
        {H5T_STD_U64BE, "H5T_STD_U64BE"}
    };
    for(const auto &[rawType, expected] : types) {
        CAPTURE(expected);
        h5pp::hid::h5t type   = H5Tcopy(rawType);
        h5pp::hid::h5t native = H5Tget_native_type(type, H5T_DIR_DEFAULT);
        CHECK(h5pp::type::getH5ClassName(type) == "H5T_INTEGER");
        // On a big-endian host the stored type may also be a native type.
        if(H5Tequal(type, native) > 0) CHECK_THAT(h5pp::type::getH5TypeName(type), Catch::Matchers::StartsWith("H5T_NATIVE_"));
        else CHECK(h5pp::type::getH5TypeName(type) == expected);
    }
}

TEST_CASE("Committed datatype descriptions report the name in the file", "[type-info]") {
    h5pp::File     file(make_path("namedTypeInfo"), h5pp::FileAccess::REPLACE, 0);
    auto           fileHandle = file.advanced().openFileHandle();
    h5pp::hid::h5t type       = H5Tcreate(H5T_OPAQUE, 7);
    REQUIRE(H5Tset_tag(type, "payload") >= 0);
    REQUIRE(H5Tcommit2(fileHandle, "payload", type, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) >= 0);
    CHECK(h5pp::type::getH5TypeName(type) == "/payload");
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    return session.run();
}
