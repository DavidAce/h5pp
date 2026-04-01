#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/h5pp.h>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }

    template<typename MatrixType>
    void require_equal_matrix(const MatrixType &lhs, const MatrixType &rhs) {
        REQUIRE(lhs.rows() == rhs.rows());
        REQUIRE(lhs.cols() == rhs.cols());
        for(Eigen::Index row = 0; row < lhs.rows(); ++row)
            for(Eigen::Index col = 0; col < lhs.cols(); ++col) REQUIRE(lhs(row, col) == rhs(row, col));
    }
}

TEST_CASE("Large standard-library datasets round-trip with their dimensions intact", "[large-write]") {
    h5pp::File file(make_path("largeWrite"), h5pp::FileAccess::REPLACE, 0);

    std::string                       text = "teststring";
    std::vector<std::complex<double>> vectorComplex(10000);
    for(size_t idx = 0; idx < vectorComplex.size(); ++idx)
        vectorComplex[idx] = {static_cast<double>(idx), -static_cast<double>(idx % 17)};

    file.writeDataset(text, "simpleWriteGroup/String");
    file.writeDataset(vectorComplex, "largeWriteGroup/vectorComplexDouble");

    REQUIRE(file.readDataset<std::string>("simpleWriteGroup/String") == text);
    REQUIRE(file.readDataset<std::vector<std::complex<double>>>("largeWriteGroup/vectorComplexDouble") == vectorComplex);

    auto info = file.dataset("largeWriteGroup/vectorComplexDouble").getInfo();
    REQUIRE(info.dsetDims);
    REQUIRE(info.dsetDims.value() == std::vector<hsize_t>{vectorComplex.size()});
}

#ifdef H5PP_USE_EIGEN3
TEST_CASE("Large Eigen matrices of several scalar types round-trip", "[large-write][eigen]") {
    h5pp::File file(make_path("largeWrite-eigen"), h5pp::FileAccess::REPLACE, 0);

    Eigen::MatrixXi  matrixInt(160, 120);
    Eigen::MatrixXd  matrixDouble(160, 120);
    Eigen::MatrixXcd matrixComplex(80, 60);

    for(Eigen::Index row = 0; row < matrixInt.rows(); ++row) {
        for(Eigen::Index col = 0; col < matrixInt.cols(); ++col) {
            matrixInt(row, col)    = static_cast<int>(row * 1000 + col);
            matrixDouble(row, col) = static_cast<double>(row) + static_cast<double>(col) / 10.0;
        }
    }
    for(Eigen::Index row = 0; row < matrixComplex.rows(); ++row)
        for(Eigen::Index col = 0; col < matrixComplex.cols(); ++col)
            matrixComplex(row, col) = {static_cast<double>(row + col), static_cast<double>(row - col)};

    file.writeDataset(matrixInt, "largeWriteGroup/matrixInt");
    file.writeDataset(matrixDouble, "largeWriteGroup/matrixDouble");
    file.writeDataset(matrixComplex, "largeWriteGroup/matrixComplexDouble");

    require_equal_matrix(matrixInt, file.readDataset<Eigen::MatrixXi>("largeWriteGroup/matrixInt"));
    require_equal_matrix(matrixDouble, file.readDataset<Eigen::MatrixXd>("largeWriteGroup/matrixDouble"));
    require_equal_matrix(matrixComplex, file.readDataset<Eigen::MatrixXcd>("largeWriteGroup/matrixComplexDouble"));
}
#endif

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
