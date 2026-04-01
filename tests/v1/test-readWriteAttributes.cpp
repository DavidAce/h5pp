#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/v1/h5pp.h>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }
}

TEST_CASE("Attributes round-trip for scalar, array, vector and string types", "[attributes]") {
    int                               attribute_int              = 7;
    double                            attribute_double           = 47.4;
    std::complex<int>                 attribute_complex_int      = {47, -10};
    std::complex<double>              attribute_complex_double   = {47.2, -10.2445};
    std::array<long, 4>               attribute_array_long       = {1, 2, 3, 4};
    float                             attribute_c_array_float[4] = {1, 2, 3, 4};
    std::vector<std::complex<double>> attribute_vector_complex   = {
        {  2.0,  5.0},
        {  3.1, -2.3},
        3.0,
        {-51.2,  5.0}
    };
    std::vector<double> attribute_vector_double = {1.0,  0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,
                                                   0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 0.0, 0.0, 0.0,
                                                   -1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0};
    std::string         attribute_string        = "This is a very long string that I am testing";
    char                attribute_char_array[]  = "This is a char array";

    h5pp::v1::File file(make_path("readWriteAttributes"), h5pp::FileAccess::REPLACE, 0);
    file.writeDataset(std::vector<double>(10, 5), "testGroup/vectorDouble");

    file.writeAttribute(attribute_int, "testGroup/vectorDouble", "AttributeInt");
    file.writeAttribute(attribute_double, "testGroup/vectorDouble", "AttributeDouble");
    file.writeAttribute(attribute_complex_int, "testGroup/vectorDouble", "AttributeComplexInt");
    file.writeAttribute(attribute_complex_double, "testGroup/vectorDouble", "AttributeComplexDouble");
    file.writeAttribute(attribute_array_long, "testGroup/vectorDouble", "AttributeArrayLong");
    file.writeAttribute(attribute_c_array_float, "testGroup/vectorDouble", "AttributeCArrayFloat");
    file.writeAttribute(attribute_vector_double, "testGroup/vectorDouble", "AttributeVectorDouble");
    file.writeAttribute(attribute_vector_double, "testGroup/vectorDouble", "AttributeVectorDouble");
    file.writeAttribute(attribute_vector_complex, "testGroup/vectorDouble", "AttributeVectorComplexDouble");
    file.writeAttribute(attribute_string, "testGroup/vectorDouble", "AttributeString");
    file.writeAttribute(attribute_string, "testGroup/vectorDouble", "AttributeString");
    file.writeAttribute(attribute_char_array, "testGroup/vectorDouble", "AttributeCharArray");

    REQUIRE(file.readAttribute<int>("testGroup/vectorDouble", "AttributeInt") == attribute_int);
    REQUIRE(file.readAttribute<double>("testGroup/vectorDouble", "AttributeDouble") == attribute_double);
    REQUIRE(file.readAttribute<std::complex<int>>("testGroup/vectorDouble", "AttributeComplexInt") == attribute_complex_int);
    REQUIRE(file.readAttribute<std::complex<double>>("testGroup/vectorDouble", "AttributeComplexDouble") == attribute_complex_double);
    REQUIRE(file.readAttribute<std::array<long, 4>>("testGroup/vectorDouble", "AttributeArrayLong") == attribute_array_long);
    REQUIRE(file.readAttribute<std::vector<float>>("testGroup/vectorDouble", "AttributeCArrayFloat") == std::vector<float>{1, 2, 3, 4});
    REQUIRE(file.readAttribute<std::vector<double>>("testGroup/vectorDouble", "AttributeVectorDouble") == attribute_vector_double);
    REQUIRE(file.readAttribute<std::vector<std::complex<double>>>("testGroup/vectorDouble", "AttributeVectorComplexDouble") ==
            attribute_vector_complex);
    REQUIRE(file.readAttribute<std::string>("testGroup/vectorDouble", "AttributeString") == attribute_string);
    REQUIRE(file.readAttribute<std::string>("testGroup/vectorDouble", "AttributeCharArray") == attribute_char_array);

    auto opt_int     = file.readAttribute<std::optional<int>>("testGroup/vectorDouble", "AttributeInt");
    auto opt_vector  = file.readAttribute<std::optional<std::vector<double>>>("testGroup/vectorDouble", "AttributeVectorDouble");
    auto opt_missing = file.readAttribute<std::optional<std::string>>("testGroup/vectorDouble", "MissingAttribute");

    REQUIRE(opt_int.has_value());
    REQUIRE(opt_vector.has_value());
    REQUIRE_FALSE(opt_missing.has_value());
    REQUIRE(opt_int.value() == attribute_int);
    REQUIRE(opt_vector.value() == attribute_vector_double);

    auto all_attributes = file.getAttributeNames("testGroup/vectorDouble");
    REQUIRE(all_attributes.size() == 10);
    REQUIRE(std::find(all_attributes.begin(), all_attributes.end(), "AttributeString") != all_attributes.end());
    REQUIRE(std::find(all_attributes.begin(), all_attributes.end(), "AttributeVectorComplexDouble") != all_attributes.end());
}

#ifdef H5PP_USE_EIGEN3
TEST_CASE("Eigen-valued attributes round-trip", "[attributes][eigen]") {
    h5pp::v1::File file(make_path("readWriteAttributes-eigen"), h5pp::FileAccess::REPLACE, 0);
    file.writeDataset(std::vector<double>(4, 1.0), "testGroup/vectorDouble");

    Eigen::MatrixXd  matrix_double(6, 4);
    Eigen::MatrixXcd matrix_complex(5, 3);
    for(Eigen::Index row = 0; row < matrix_double.rows(); ++row)
        for(Eigen::Index col = 0; col < matrix_double.cols(); ++col) matrix_double(row, col) = static_cast<double>(row * 10 + col);
    for(Eigen::Index row = 0; row < matrix_complex.rows(); ++row)
        for(Eigen::Index col = 0; col < matrix_complex.cols(); ++col)
            matrix_complex(row, col) = {static_cast<double>(row + col), static_cast<double>(row - col)};

    file.writeAttribute(matrix_double, "testGroup/vectorDouble", "AttributeEigenMatrixDouble");
    file.writeAttribute(matrix_complex, "testGroup/vectorDouble", "AttributeEigenMatrixComplexDouble");

    REQUIRE(file.readAttribute<Eigen::MatrixXd>("testGroup/vectorDouble", "AttributeEigenMatrixDouble") == matrix_double);
    REQUIRE(file.readAttribute<Eigen::MatrixXcd>("testGroup/vectorDouble", "AttributeEigenMatrixComplexDouble") == matrix_complex);
}
#endif

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
