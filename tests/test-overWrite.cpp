#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/h5pp.h>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }

    std::vector<std::complex<double>> make_complex_series(size_t size, double scale = 1.0) {
        std::vector<std::complex<double>> data(size);
        for(size_t idx = 0; idx < size; ++idx) data[idx] = {scale * static_cast<double>(idx + 1), -scale * static_cast<double>(idx % 9)};
        return data;
    }
}

TEST_CASE("Contiguous datasets support slab updates and same-sized overwrites", "[overwrite][contiguous]") {
    h5pp::File file(make_path("overWrite-contiguous"), h5pp::FileAccess::REPLACE, 0);

    auto original = make_complex_series(10, 1.0);
    auto partial  = make_complex_series(5, 10.0);

    h5pp::DatasetCreateOptions contiguousCreate;
    contiguousCreate.h5Layout = H5D_CONTIGUOUS;
    file.dataset("overWriteGroup_contiguous/vectorComplexDouble").create(original, contiguousCreate).write(original);

    h5pp::DatasetWriteOptions partialWrite;
    partialWrite.dims = std::vector<hsize_t>{partial.size()};
    file.dataset("overWriteGroup_contiguous/vectorComplexDouble").select({0}, {partial.size()}).write(partial, partialWrite);

    auto expected = original;
    std::copy(partial.begin(), partial.end(), expected.begin());
    REQUIRE(file.dataset("overWriteGroup_contiguous/vectorComplexDouble").read<std::vector<std::complex<double>>>() == expected);

    auto rewrite = make_complex_series(10, 100.0);
    REQUIRE_NOTHROW(file.dataset("overWriteGroup_contiguous/vectorComplexDouble").write(rewrite));
    REQUIRE(file.dataset("overWriteGroup_contiguous/vectorComplexDouble").read<std::vector<std::complex<double>>>() == rewrite);

    REQUIRE_THROWS_AS(file.dataset("overWriteGroup_contiguous/vectorComplexDouble").write(make_complex_series(5, 2.0)), std::runtime_error);
    REQUIRE_THROWS_AS(file.dataset("overWriteGroup_contiguous/vectorComplexDouble").write(make_complex_series(15, 2.0)),
                      std::runtime_error);
}

TEST_CASE("Chunked datasets overwrite, grow and shrink cleanly", "[overwrite][chunked]") {
    h5pp::File file(make_path("overWrite-chunked"), h5pp::FileAccess::REPLACE, 0);

    auto initial = make_complex_series(5, 1.0);
    h5pp::DatasetCreateOptions chunkedCreate;
    chunkedCreate.h5Layout = H5D_CHUNKED;
    file.dataset("overWriteGroup_chunked/vectorComplexDouble").create(initial, chunkedCreate).write(initial);
    REQUIRE(file.dataset("overWriteGroup_chunked/vectorComplexDouble").read<std::vector<std::complex<double>>>() == initial);

    auto grown = make_complex_series(150, 2.0);
    REQUIRE_NOTHROW(file.dataset("overWriteGroup_chunked/vectorComplexDouble").write(grown));
    REQUIRE(file.dataset("overWriteGroup_chunked/vectorComplexDouble").read<std::vector<std::complex<double>>>() == grown);

    file.dataset("overWriteGroup_chunked/somestring")
        .create(std::string("this is a teststring"), chunkedCreate)
        .write(std::string("this is a teststring"));
    file.dataset("overWriteGroup_chunked/somestring").write(std::string("this is a slightly longer string"));
    REQUIRE(file.dataset("overWriteGroup_chunked/somestring").read<std::string>() == "this is a slightly longer string");

    auto shrunk = make_complex_series(15, 3.0);
    file.dataset("overWriteGroup_chunked/vectorComplexDouble").resize(shrunk.size(), h5pp::ResizePolicy::FIT);
    REQUIRE_NOTHROW(file.dataset("overWriteGroup_chunked/vectorComplexDouble").write(shrunk));
    REQUIRE(file.dataset("overWriteGroup_chunked/vectorComplexDouble").read<std::vector<std::complex<double>>>() == shrunk);

    file.dataset("overWriteGroup_chunked/somestring").write(std::string("short string"));
    REQUIRE(file.dataset("overWriteGroup_chunked/somestring").read<std::string>() == "short string");
}

TEST_CASE("Large auto-written datasets stay writable after growing beyond the initial size", "[overwrite][auto-layout]") {
    h5pp::File file(make_path("overWrite-auto-layout"), h5pp::FileAccess::REPLACE, 0);

    auto initial = make_complex_series(32 * 1024, 1.0);
    auto grown   = make_complex_series(128 * 1024, 1.0);

    file.dataset("overWriteGroup_auto/vectorComplexDouble").write(initial);
    REQUIRE_NOTHROW(file.dataset("overWriteGroup_auto/vectorComplexDouble").write(grown));
    REQUIRE(file.dataset("overWriteGroup_auto/vectorComplexDouble").read<std::vector<std::complex<double>>>() == grown);

    auto info = file.dataset("overWriteGroup_auto/vectorComplexDouble").getInfo();
    REQUIRE(info.h5Layout.has_value());
    REQUIRE(info.dsetDims.has_value());
    REQUIRE(info.dsetDims.value() == std::vector<hsize_t>{grown.size()});
}

#ifdef H5PP_USE_EIGEN3
TEST_CASE("Chunked Eigen datasets can be overwritten across shape changes", "[overwrite][eigen]") {
    h5pp::File file(make_path("overWrite-eigen"), h5pp::FileAccess::REPLACE, 0);

    Eigen::MatrixXi  matrixInt(40, 40);
    Eigen::MatrixXd  matrixDouble(40, 40);
    Eigen::MatrixXcd matrixComplex(40, 40);

    for(Eigen::Index row = 0; row < matrixInt.rows(); ++row) {
        for(Eigen::Index col = 0; col < matrixInt.cols(); ++col) {
            matrixInt(row, col)    = static_cast<int>(row * 100 + col);
            matrixDouble(row, col) = static_cast<double>(row) + static_cast<double>(col) / 100.0;
            matrixComplex(row, col) = {static_cast<double>(row + col), static_cast<double>(row - col)};
        }
    }

    h5pp::DatasetCreateOptions chunkedCreate;
    chunkedCreate.h5Layout = H5D_CHUNKED;
    file.dataset("overWriteGroup_chunked/matrixInt").create(matrixInt, chunkedCreate).write(matrixInt);
    file.dataset("overWriteGroup_chunked/matrixDouble").create(matrixDouble, chunkedCreate).write(matrixDouble);
    file.dataset("overWriteGroup_chunked/matrixComplexDouble").create(matrixComplex, chunkedCreate).write(matrixComplex);

    matrixInt.resize(60, 60);
    matrixDouble.resize(60, 60);
    matrixComplex.resize(60, 60);
    for(Eigen::Index row = 0; row < matrixInt.rows(); ++row) {
        for(Eigen::Index col = 0; col < matrixInt.cols(); ++col) {
            matrixInt(row, col)    = static_cast<int>(row * 1000 + col);
            matrixDouble(row, col) = static_cast<double>(row * col) / 10.0;
            matrixComplex(row, col) = {static_cast<double>(row), static_cast<double>(-col)};
        }
    }

    file.dataset("overWriteGroup_chunked/matrixInt").write(matrixInt);
    file.dataset("overWriteGroup_chunked/matrixDouble").write(matrixDouble);
    file.dataset("overWriteGroup_chunked/matrixComplexDouble").write(matrixComplex);

    REQUIRE(file.dataset("overWriteGroup_chunked/matrixInt").read<Eigen::MatrixXi>() == matrixInt);
    REQUIRE(file.dataset("overWriteGroup_chunked/matrixDouble").read<Eigen::MatrixXd>() == matrixDouble);
    REQUIRE(file.dataset("overWriteGroup_chunked/matrixComplexDouble").read<Eigen::MatrixXcd>() == matrixComplex);
}
#endif

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    session.configData().shouldDebugBreak = true;
    return session.run();
}
