#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/h5pp.h>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }
}

TEST_CASE("Attributes round-trip for scalar, array, vector and string types", "[attributes]") {
    int                               attributeInt             = 7;
    double                            attributeDouble          = 47.4;
    std::complex<int>                 attributeComplexInt      = {47, -10};
    std::complex<double>              attributeComplexDouble   = {47.2, -10.2445};
    std::array<long, 4>               attributeArrayLong       = {1, 2, 3, 4};
    float                             attributeCArrayFloat[4]  = {1, 2, 3, 4};
    std::vector<std::complex<double>> attributeVectorComplex   = {{2.0, 5.0}, {3.1, -2.3}, {3.0, 0.0}, {-51.2, 5.0}};
    std::vector<double>               attributeVectorDouble    = {1.0,  0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,
                                                                  0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 0.0, 0.0, 0.0,
                                                                  -1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0};
    std::string                       attributeString          = "This is a very long string that I am testing";
    char                              attributeCharArray[]     = "This is a char array";

    h5pp::File file(make_path("readWriteAttributes"), h5pp::FileAccess::REPLACE, 0);
    file.writeDataset(std::vector<double>(10, 5), "testGroup/vectorDouble");

    file.writeAttribute("testGroup/vectorDouble", "AttributeInt", attributeInt);
    file.writeAttribute("testGroup/vectorDouble", "AttributeDouble", attributeDouble);
    file.writeAttribute("testGroup/vectorDouble", "AttributeComplexInt", attributeComplexInt);
    file.writeAttribute("testGroup/vectorDouble", "AttributeComplexDouble", attributeComplexDouble);
    file.writeAttribute("testGroup/vectorDouble", "AttributeArrayLong", attributeArrayLong);
    file.writeAttribute("testGroup/vectorDouble", "AttributeCArrayFloat", attributeCArrayFloat);
    file.writeAttribute("testGroup/vectorDouble", "AttributeVectorDouble", attributeVectorDouble);
    file.writeAttribute("testGroup/vectorDouble", "AttributeVectorDouble", attributeVectorDouble);
    file.writeAttribute("testGroup/vectorDouble", "AttributeVectorComplexDouble", attributeVectorComplex);
    file.writeAttribute("testGroup/vectorDouble", "AttributeString", attributeString);
    file.writeAttribute("testGroup/vectorDouble", "AttributeString", attributeString);
    file.writeAttribute("testGroup/vectorDouble", "AttributeCharArray", attributeCharArray);

    REQUIRE(file.readAttribute<int>("testGroup/vectorDouble", "AttributeInt") == attributeInt);
    REQUIRE(file.readAttribute<double>("testGroup/vectorDouble", "AttributeDouble") == attributeDouble);
    REQUIRE(file.readAttribute<std::complex<int>>("testGroup/vectorDouble", "AttributeComplexInt") == attributeComplexInt);
    REQUIRE(file.readAttribute<std::complex<double>>("testGroup/vectorDouble", "AttributeComplexDouble") == attributeComplexDouble);
    REQUIRE(file.readAttribute<std::array<long, 4>>("testGroup/vectorDouble", "AttributeArrayLong") == attributeArrayLong);
    REQUIRE(file.readAttribute<std::vector<float>>("testGroup/vectorDouble", "AttributeCArrayFloat") == std::vector<float>{1, 2, 3, 4});
    REQUIRE(file.readAttribute<std::vector<double>>("testGroup/vectorDouble", "AttributeVectorDouble") == attributeVectorDouble);
    REQUIRE(file.readAttribute<std::vector<std::complex<double>>>("testGroup/vectorDouble", "AttributeVectorComplexDouble") ==
            attributeVectorComplex);
    REQUIRE(file.readAttribute<std::string>("testGroup/vectorDouble", "AttributeString") == attributeString);
    REQUIRE(file.readAttribute<std::string>("testGroup/vectorDouble", "AttributeCharArray") == attributeCharArray);

    auto optInt     = file.readAttribute<std::optional<int>>("testGroup/vectorDouble", "AttributeInt");
    auto optVector  = file.readAttribute<std::optional<std::vector<double>>>("testGroup/vectorDouble", "AttributeVectorDouble");
    auto optMissing = file.readAttribute<std::optional<std::string>>("testGroup/vectorDouble", "MissingAttribute");

    REQUIRE(optInt.has_value());
    REQUIRE(optVector.has_value());
    REQUIRE_FALSE(optMissing.has_value());
    REQUIRE(optInt.value() == attributeInt);
    REQUIRE(optVector.value() == attributeVectorDouble);

    auto allAttributes = file.link("testGroup/vectorDouble").getAttributeNames();
    REQUIRE(allAttributes.size() == 10);
    REQUIRE(std::find(allAttributes.begin(), allAttributes.end(), "AttributeString") != allAttributes.end());
    REQUIRE(std::find(allAttributes.begin(), allAttributes.end(), "AttributeVectorComplexDouble") != allAttributes.end());

    h5pp::DatasetCreateOptions chunkedCreate;
    chunkedCreate.h5Layout  = H5D_CHUNKED;
    chunkedCreate.dimsChunk = {4};
    file.writeDataset(std::vector<double>{1.0, 2.0, 3.0, 4.0}, "testGroup/chunkedVector", chunkedCreate);
    file.attribute("testGroup/chunkedVector", "unit").write(std::string("arb"));
    REQUIRE(file.attribute("testGroup/chunkedVector", "unit").read<std::string>() == "arb");
}

#ifdef H5PP_USE_EIGEN3
TEST_CASE("Eigen-valued attributes round-trip", "[attributes][eigen]") {
    h5pp::File file(make_path("readWriteAttributes-eigen"), h5pp::FileAccess::REPLACE, 0);
    file.writeDataset(std::vector<double>(4, 1.0), "testGroup/vectorDouble");

    Eigen::MatrixXd  matrixDouble(6, 4);
    Eigen::MatrixXcd matrixComplex(5, 3);
    for(Eigen::Index row = 0; row < matrixDouble.rows(); ++row)
        for(Eigen::Index col = 0; col < matrixDouble.cols(); ++col) matrixDouble(row, col) = static_cast<double>(row * 10 + col);
    for(Eigen::Index row = 0; row < matrixComplex.rows(); ++row)
        for(Eigen::Index col = 0; col < matrixComplex.cols(); ++col)
            matrixComplex(row, col) = {static_cast<double>(row + col), static_cast<double>(row - col)};

    file.writeAttribute("testGroup/vectorDouble", "AttributeEigenMatrixDouble", matrixDouble);
    file.writeAttribute("testGroup/vectorDouble", "AttributeEigenMatrixComplexDouble", matrixComplex);

    REQUIRE(file.readAttribute<Eigen::MatrixXd>("testGroup/vectorDouble", "AttributeEigenMatrixDouble") == matrixDouble);
    REQUIRE(file.readAttribute<Eigen::MatrixXcd>("testGroup/vectorDouble", "AttributeEigenMatrixComplexDouble") == matrixComplex);
}
#endif

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
