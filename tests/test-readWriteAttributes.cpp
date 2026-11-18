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
    file.dataset("testGroup/vectorDouble").write(std::vector<double>(10, 5));

    file.attribute("testGroup/vectorDouble", "AttributeInt").write(attributeInt);
    file.attribute("testGroup/vectorDouble", "AttributeDouble").write(attributeDouble);
    file.attribute("testGroup/vectorDouble", "AttributeComplexInt").write(attributeComplexInt);
    file.attribute("testGroup/vectorDouble", "AttributeComplexDouble").write(attributeComplexDouble);
    file.attribute("testGroup/vectorDouble", "AttributeArrayLong").write(attributeArrayLong);
    file.attribute("testGroup/vectorDouble", "AttributeCArrayFloat").write(attributeCArrayFloat);
    file.attribute("testGroup/vectorDouble", "AttributeVectorDouble").write(attributeVectorDouble);
    file.attribute("testGroup/vectorDouble", "AttributeVectorDouble").write(attributeVectorDouble);
    file.attribute("testGroup/vectorDouble", "AttributeVectorComplexDouble").write(attributeVectorComplex);
    file.attribute("testGroup/vectorDouble", "AttributeString").write(attributeString);
    file.attribute("testGroup/vectorDouble", "AttributeString").write(attributeString);
    file.attribute("testGroup/vectorDouble", "AttributeCharArray").write(attributeCharArray);

    REQUIRE(file.attribute("testGroup/vectorDouble", "AttributeInt").read<int>() == attributeInt);
    REQUIRE(file.attribute("testGroup/vectorDouble", "AttributeDouble").read<double>() == attributeDouble);
    REQUIRE(file.attribute("testGroup/vectorDouble", "AttributeComplexInt").read<std::complex<int>>() == attributeComplexInt);
    REQUIRE(file.attribute("testGroup/vectorDouble", "AttributeComplexDouble").read<std::complex<double>>() == attributeComplexDouble);
    REQUIRE(file.attribute("testGroup/vectorDouble", "AttributeArrayLong").read<std::array<long, 4>>() == attributeArrayLong);
    REQUIRE(file.attribute("testGroup/vectorDouble", "AttributeCArrayFloat").read<std::vector<float>>() == std::vector<float>{1, 2, 3, 4});
    REQUIRE(file.attribute("testGroup/vectorDouble", "AttributeVectorDouble").read<std::vector<double>>() == attributeVectorDouble);
    REQUIRE(file.attribute("testGroup/vectorDouble", "AttributeVectorComplexDouble").read<std::vector<std::complex<double>>>() ==
            attributeVectorComplex);
    REQUIRE(file.attribute("testGroup/vectorDouble", "AttributeString").read<std::string>() == attributeString);
    REQUIRE(file.attribute("testGroup/vectorDouble", "AttributeCharArray").read<std::string>() == attributeCharArray);

    auto optInt     = file.attribute("testGroup/vectorDouble", "AttributeInt").read<std::optional<int>>();
    auto optVector  = file.attribute("testGroup/vectorDouble", "AttributeVectorDouble").read<std::optional<std::vector<double>>>();
    auto optMissing = file.attribute("testGroup/vectorDouble", "MissingAttribute").read<std::optional<std::string>>();

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
    file.dataset("testGroup/chunkedVector").ensure(chunkedCreate).write(std::vector<double>{1.0, 2.0, 3.0, 4.0});
    file.attribute("testGroup/chunkedVector", "unit").write(std::string("arb"));
    REQUIRE(file.attribute("testGroup/chunkedVector", "unit").read<std::string>() == "arb");
}

#ifdef H5PP_USE_EIGEN3
TEST_CASE("Eigen-valued attributes round-trip", "[attributes][eigen]") {
    h5pp::File file(make_path("readWriteAttributes-eigen"), h5pp::FileAccess::REPLACE, 0);
    file.dataset("testGroup/vectorDouble").write(std::vector<double>(4, 1.0));

    Eigen::MatrixXd  matrixDouble(6, 4);
    Eigen::MatrixXcd matrixComplex(5, 3);
    for(Eigen::Index row = 0; row < matrixDouble.rows(); ++row)
        for(Eigen::Index col = 0; col < matrixDouble.cols(); ++col) matrixDouble(row, col) = static_cast<double>(row * 10 + col);
    for(Eigen::Index row = 0; row < matrixComplex.rows(); ++row)
        for(Eigen::Index col = 0; col < matrixComplex.cols(); ++col)
            matrixComplex(row, col) = {static_cast<double>(row + col), static_cast<double>(row - col)};

    file.attribute("testGroup/vectorDouble", "AttributeEigenMatrixDouble").write(matrixDouble);
    file.attribute("testGroup/vectorDouble", "AttributeEigenMatrixComplexDouble").write(matrixComplex);

    REQUIRE(file.attribute("testGroup/vectorDouble", "AttributeEigenMatrixDouble").read<Eigen::MatrixXd>() == matrixDouble);
    REQUIRE(file.attribute("testGroup/vectorDouble", "AttributeEigenMatrixComplexDouble").read<Eigen::MatrixXcd>() == matrixComplex);
}
#endif

TEST_CASE("Read-only files reject attribute creation and replacement", "[attributes][file-access]") {
    auto path = make_path("attributesReadOnly");
    {
        h5pp::File file(path, h5pp::FileAccess::REPLACE, 0);
        file.dataset("values").write(42);
        file.dataset("values").attribute("existing").write(7);
    }
    h5pp::File file(path, h5pp::FileAccess::READONLY, 0);
    SECTION("creation") {
        REQUIRE_THROWS_WITH(file.dataset("values").attribute("new").write(9), Catch::Matchers::ContainsSubstring("read-only"));
        CHECK_FALSE(file.dataset("values").attribute("new").exists());
    }
    SECTION("replacement") {
        REQUIRE_THROWS_WITH(file.dataset("values").attribute("existing").write(9), Catch::Matchers::ContainsSubstring("read-only"));
    }
    CHECK(file.dataset("values").attribute("existing").read<int>() == 7);
    CHECK(file.dataset("values").read<int>() == 42);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
