#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/h5pp.h>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format("output/{}.h5", name);
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
    std::vector<std::complex<double>> vector_complex(10000);
    for(size_t idx = 0; idx < vector_complex.size(); ++idx)
        vector_complex[idx] = {static_cast<double>(idx), -static_cast<double>(idx % 17)};

    file.writeDataset(text, "simpleWriteGroup/String");
    file.writeDataset(vector_complex, "largeWriteGroup/vectorComplexDouble");

    REQUIRE(file.readDataset<std::string>("simpleWriteGroup/String") == text);
    REQUIRE(file.readDataset<std::vector<std::complex<double>>>("largeWriteGroup/vectorComplexDouble") == vector_complex);

    auto info = file.getDatasetInfo("largeWriteGroup/vectorComplexDouble");
    REQUIRE(info.dsetDims);
    REQUIRE(info.dsetDims.value() == std::vector<hsize_t>{vector_complex.size()});
}

#ifdef H5PP_USE_EIGEN3
TEST_CASE("Large Eigen matrices of several scalar types round-trip", "[large-write][eigen]") {
    h5pp::File file(make_path("largeWrite-eigen"), h5pp::FileAccess::REPLACE, 0);

    Eigen::MatrixXi  matrix_int(160, 120);
    Eigen::MatrixXd  matrix_double(160, 120);
    Eigen::MatrixXcd matrix_complex(80, 60);

    for(Eigen::Index row = 0; row < matrix_int.rows(); ++row) {
        for(Eigen::Index col = 0; col < matrix_int.cols(); ++col) {
            matrix_int(row, col)    = static_cast<int>(row * 1000 + col);
            matrix_double(row, col) = static_cast<double>(row) + static_cast<double>(col) / 10.0;
        }
    }
    for(Eigen::Index row = 0; row < matrix_complex.rows(); ++row)
        for(Eigen::Index col = 0; col < matrix_complex.cols(); ++col)
            matrix_complex(row, col) = {static_cast<double>(row + col), static_cast<double>(row - col)};

    file.writeDataset(matrix_int, "largeWriteGroup/matrixInt");
    file.writeDataset(matrix_double, "largeWriteGroup/matrixDouble");
    file.writeDataset(matrix_complex, "largeWriteGroup/matrixComplexDouble");

    require_equal_matrix(matrix_int, file.readDataset<Eigen::MatrixXi>("largeWriteGroup/matrixInt"));
    require_equal_matrix(matrix_double, file.readDataset<Eigen::MatrixXd>("largeWriteGroup/matrixDouble"));
    require_equal_matrix(matrix_complex, file.readDataset<Eigen::MatrixXcd>("largeWriteGroup/matrixComplexDouble"));
}
#endif

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
