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

    file.dataset("simpleWriteGroup/String").write(text);
    file.dataset("largeWriteGroup/vectorComplexDouble").write(vectorComplex);

    REQUIRE(file.dataset("simpleWriteGroup/String").read<std::string>() == text);
    REQUIRE(file.dataset("largeWriteGroup/vectorComplexDouble").read<std::vector<std::complex<double>>>() == vectorComplex);

    auto info = file.dataset("largeWriteGroup/vectorComplexDouble").getInfo();
    REQUIRE(info.dsetDims.has_value());
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

    file.dataset("largeWriteGroup/matrixInt").write(matrixInt);
    file.dataset("largeWriteGroup/matrixDouble").write(matrixDouble);
    file.dataset("largeWriteGroup/matrixComplexDouble").write(matrixComplex);

    require_equal_matrix(matrixInt, file.dataset("largeWriteGroup/matrixInt").read<Eigen::MatrixXi>());
    require_equal_matrix(matrixDouble, file.dataset("largeWriteGroup/matrixDouble").read<Eigen::MatrixXd>());
    require_equal_matrix(matrixComplex, file.dataset("largeWriteGroup/matrixComplexDouble").read<Eigen::MatrixXcd>());
}
#endif

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
