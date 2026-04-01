#include <algorithm>
#include <array>
#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/v1/h5pp.h>
#include <string>
#include <string_view>
#include <typeindex>
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
        std::vector<std::complex<double>> attribute_vector_complex_double = {
            {  2.0,  5.0},
            {  3.1, -2.3},
            {  3.0,  0.0},
            {-51.2,  5.0}
        };
        std::vector<double> attribute_vector_double = {1.0,  0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,
                                                       0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 0.0, 0.0, 0.0,
                                                       -1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0};
        std::string         attribute_string        = "This is a very long string that I am testing";
        char                attribute_char_array[]  = "This is a char array";

        h5pp::v1::File file(path, h5pp::FileAccess::REPLACE, 0);
        file.writeDataset(std::vector<double>(10, 5.0), "testGroup/vectorDouble");

        file.writeAttribute(attribute_int, "testGroup/vectorDouble", "AttributeInt");
        file.writeAttribute(attribute_double, "testGroup/vectorDouble", "AttributeDouble");
        file.writeAttribute(attribute_complex_int, "testGroup/vectorDouble", "AttributeComplexInt");
        file.writeAttribute(attribute_complex_double, "testGroup/vectorDouble", "AttributeComplexDouble");
        file.writeAttribute(attribute_array_long, "testGroup/vectorDouble", "AttributeArrayLong");
        file.writeAttribute(attribute_carray_float, "testGroup/vectorDouble", "AttributeCArrayFloat");
        file.writeAttribute(attribute_vector_double, "testGroup/vectorDouble", "AttributeVectorDouble");
        file.writeAttribute(attribute_vector_complex_double, "testGroup/vectorDouble", "AttributeVectorComplexDouble");
        file.writeAttribute(attribute_string, "testGroup/vectorDouble", "AttributeString");
        file.writeAttribute(attribute_char_array, "testGroup/vectorDouble", "AttributeCharArray");
    }
}

TEST_CASE("Type info exposes dataset metadata for stored objects", "[type-info]") {
    static_assert(
        h5pp::type::sfinae::has_data_v<std::vector<double>> &&
        "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    auto path = make_path("typeInfo");
    seed_type_info_file(path);
    h5pp::v1::File file(path, h5pp::FileAccess::READONLY, 0);

    auto info = file.getTypeInfoDataset("testGroup/vectorDouble");
    REQUIRE(info.cppTypeIndex);
    REQUIRE(info.cppTypeIndex.value() == std::type_index(typeid(double)));
    REQUIRE(info.cppTypeBytes);
    REQUIRE(info.cppTypeBytes.value() == sizeof(double));
    REQUIRE(info.h5Path);
    REQUIRE(info.h5Size);
    REQUIRE(info.h5Rank);
    REQUIRE(info.h5Dims);
    REQUIRE(info.h5Type);
    REQUIRE(info.h5Path.value().find("testGroup/vectorDouble") != std::string::npos);
    if(info.h5Name) REQUIRE(info.h5Name.value() == "vectorDouble");
    REQUIRE(info.h5Size.value() == 10);
    REQUIRE(info.h5Rank.value() == 1);
    REQUIRE(info.h5Dims.value() == std::vector<hsize_t>{10});
    REQUIRE(H5Tget_class(info.h5Type.value()) == H5T_FLOAT);
    REQUIRE_FALSE(info.string().empty());

    auto via_generic = file.getInfo<h5pp::TypeInfo>("testGroup/vectorDouble");
    REQUIRE(via_generic.h5Dims == info.h5Dims);
}

TEST_CASE("Type info exposes per-attribute metadata across scalar, array and string attributes", "[type-info]") {
    auto path = make_path("typeInfo-attrs");
    seed_type_info_file(path);
    h5pp::v1::File file(path, h5pp::FileAccess::READONLY, 0);

    auto infos = file.getTypeInfoAttributes("testGroup/vectorDouble");
    REQUIRE(infos.size() == 10);

    std::vector<std::string> names;
    names.reserve(infos.size());
    for(const auto &info : infos) {
        REQUIRE(info.h5Name);
        REQUIRE(info.h5Path);
        REQUIRE(info.h5Type);
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

    auto attr_int = file.getTypeInfoAttribute("testGroup/vectorDouble", "AttributeInt");
    REQUIRE(attr_int.cppTypeIndex);
    REQUIRE(attr_int.cppTypeIndex.value() == std::type_index(typeid(int)));
    REQUIRE(attr_int.h5Rank);
    REQUIRE(attr_int.h5Rank.value() == 0);
    REQUIRE(H5Tget_class(attr_int.h5Type.value()) == H5T_INTEGER);

    auto attr_vector = file.getTypeInfoAttribute("testGroup/vectorDouble", "AttributeVectorComplexDouble");
    REQUIRE(attr_vector.cppTypeIndex);
    REQUIRE(attr_vector.cppTypeIndex.value() == std::type_index(typeid(std::complex<double>)));
    REQUIRE(attr_vector.cppTypeBytes);
    REQUIRE(attr_vector.cppTypeBytes.value() == sizeof(std::complex<double>));
    REQUIRE(attr_vector.h5Dims);
    REQUIRE(attr_vector.h5Dims.value() == std::vector<hsize_t>{4});
    REQUIRE(attr_vector.h5Rank);
    REQUIRE(attr_vector.h5Rank.value() == 1);
    REQUIRE(H5Tget_class(attr_vector.h5Type.value()) == H5T_COMPOUND);

    auto attr_string = file.getInfo<h5pp::TypeInfo>("testGroup/vectorDouble", "AttributeString");
    REQUIRE(attr_string.h5Name);
    REQUIRE(attr_string.h5Name.value() == "AttributeString");
    REQUIRE(H5Tis_variable_str(attr_string.h5Type.value()) > 0);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    return session.run();
}
