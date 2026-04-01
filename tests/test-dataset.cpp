#include <catch2/catch_all.hpp>
#include <cstring>
#include <h5pp/details/h5ppFormatComplex.h>
#include <h5pp/h5pp.h>
#include <string>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }

    template<typename L, typename R>
    bool require_equal_value(const L &lhs, const R &rhs) {
#if defined(H5PP_USE_QUADMATH) || defined(H5PP_USE_FLOAT128)
        if constexpr((std::is_same_v<std::decay_t<L>, h5pp::fp128> && std::is_same_v<std::decay_t<R>, h5pp::fp128>) ||
                     (std::is_same_v<std::decay_t<L>, h5pp::cx128> && std::is_same_v<std::decay_t<R>, h5pp::cx128>) ) {
            return std::memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
        } else
#endif
        {
            return lhs == rhs;
        }
    }

    const std::vector<std::optional<H5D_layout_t>> layouts      = {std::nullopt, H5D_COMPACT, H5D_CONTIGUOUS, H5D_CHUNKED};
    const std::vector<std::string>                 layout_names = {"auto", "compact", "contiguous", "chunked"};

    void require_zero_vector(h5pp::File &file, std::string_view path) {
        REQUIRE(file.readDataset<std::vector<double>>(path) == std::vector<double>{0, 0, 0, 0});
    }
}

template<typename T>
void require_scalar_equal(const T &lhs, const T &rhs) {
    if constexpr(h5pp::type::sfinae::is_Scalar2_v<T>) {
        REQUIRE(require_equal_value(lhs.x, rhs.x));
        REQUIRE(require_equal_value(lhs.y, rhs.y));
    } else if constexpr(h5pp::type::sfinae::is_Scalar3_v<T>) {
        REQUIRE(require_equal_value(lhs.x, rhs.x));
        REQUIRE(require_equal_value(lhs.y, rhs.y));
        REQUIRE(require_equal_value(lhs.z, rhs.z));
    } else {
        REQUIRE(require_equal_value(lhs, rhs));
    }
}

template<typename WriteType, typename ReadType = WriteType>
void require_roundtrip(h5pp::File &file, const WriteType &writeData, std::string_view dsetPath) {
    using namespace h5pp::type::sfinae;
    auto dset     = file.dataset(dsetPath);
    dset.write(writeData);
    auto readData = dset.read<ReadType>();

    if constexpr(is_ScalarN_v<ReadType>) {
        require_scalar_equal(writeData, readData);
    } else if constexpr(has_ScalarN_v<ReadType>) {
        REQUIRE(writeData.size() == readData.size());
#ifdef H5PP_USE_EIGEN3
        if constexpr(is_eigen_matrix_v<ReadType>) {
            for(Eigen::Index col = 0; col < writeData.cols(); ++col)
                for(Eigen::Index row = 0; row < writeData.rows(); ++row) require_scalar_equal(writeData(row, col), readData(row, col));
        } else
#endif
            for(size_t idx = 0; idx < static_cast<size_t>(writeData.size()); ++idx) require_scalar_equal(writeData[idx], readData[idx]);
#ifdef H5PP_USE_EIGEN3
    } else if constexpr(is_eigen_tensor_v<WriteType> and is_eigen_tensor_v<ReadType>) {
        Eigen::Map<const Eigen::Matrix<typename WriteType::Scalar, Eigen::Dynamic, 1>> writeMap(writeData.data(), writeData.size());
        Eigen::Map<const Eigen::Matrix<typename ReadType::Scalar, Eigen::Dynamic, 1>>  readMap(readData.data(), readData.size());
        REQUIRE(writeMap == readMap);
#endif
    } else {
        REQUIRE(require_equal_value(writeData, readData));
    }
}

template<size_t Size, typename WriteType, typename ReadType = WriteType>
void require_roundtrip(h5pp::File &file, const WriteType *writeData, hsize_t dims, std::string_view dsetPath) {
    auto                      dset = file.dataset(dsetPath);
    h5pp::DatasetWriteOptions writeOptions;
    writeOptions.dims = std::vector<hsize_t>{dims};
    dset.write(writeData, writeOptions);

    auto readData = std::make_unique<ReadType[]>(Size);
    auto *readPtr = readData.get();
    h5pp::DatasetReadOptions readOptions;
    readOptions.dims = std::vector<hsize_t>{dims};
    dset.readInto(readPtr, readOptions);
    for(size_t idx = 0; idx < Size; ++idx) REQUIRE(require_equal_value(writeData[idx], readData[idx]));
}

TEST_CASE("Datasets round-trip representative containers and expose basic handle queries", "[dataset]") {
    using cplx = std::complex<double>;

    static_assert(
        h5pp::type::sfinae::has_data_v<std::vector<double>> and
        "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    struct Field2 {
        double x;
        double y;
    };
    struct Field3 {
        double x;
        double y;
        double z;
    };

    h5pp::File file(make_path("dataset"), H5F_ACC_TRUNC | H5F_ACC_RDWR, 0);

    std::vector<int>    emptyVector;
    std::string         stringDummy = "Dummy string with spaces";
    std::complex<float> cplxFloat(1, 1);
    std::vector<double> vectorDouble  = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,
                                         1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0};
    std::vector<cplx>   vectorComplex = {
        {-0.191154, 0.326211},
        {0.964728, -0.712335},
        {-0.0351791, -0.10264},
        {0.177544, 0.99999}
    };
    std::array<double, 10> cStyleDoubleArray{};
    for(size_t i = 0; i < cStyleDoubleArray.size(); ++i) cStyleDoubleArray[i] = static_cast<double>(i);

    Field2              field2{0.53, 0.45};
    Field3              field3{0.54, 0.56, 0.58};
    std::vector<Field2> field2vector(10);
    std::vector<Field3> field3vector(10);
    for(size_t i = 0; i < field2vector.size(); ++i) {
        auto d          = static_cast<double>(i);
        field2vector[i] = {2.3 * d, 20.5 * d};
        field3vector[i] = {2.3 * d, 20.5 * d, 200.9 * d};
    }

    h5pp::varr_t<double>              vlenDouble       = {1.0, 2.0, 3.0, 4.0};
    std::vector<h5pp::varr_t<double>> vectorVlenDouble = {{1.0}, {2.0, 3.0}, {4.0, 5.0, 6.0}, {7.0, 8.0, 9.0, 10.0}};

    require_roundtrip(file, emptyVector, "emptyVector");
    require_roundtrip(file, stringDummy, "stringDummy");
    require_roundtrip(file, cplxFloat, "cplxFloat");
    require_roundtrip(file, vectorDouble, "vectorDouble");
    require_roundtrip(file, vectorComplex, "vectorComplex");
    require_roundtrip<10, double>(file, cStyleDoubleArray.data(), 10, "cStyleDoubleArray");
    require_roundtrip(file, field2, "field2");
    require_roundtrip(file, field3, "field3");
    require_roundtrip(file, field2vector, "field2vector");
    require_roundtrip(file, field3vector, "field3vector");
    require_roundtrip(file, vlenDouble, "vlenDouble");
    require_roundtrip(file, vectorVlenDouble, "vectorVlenDouble");

    auto vectorReadBytes = file.readDataset<std::vector<std::byte>>("vectorDouble");
    REQUIRE(vectorReadBytes.size() == vectorDouble.size() * sizeof(double));

    auto dset = file.dataset("vectorDouble");
    REQUIRE(dset.exists());
    auto info = dset.getInfo();
    REQUIRE(info.dsetDims);
    REQUIRE(info.dsetDims.value() == std::vector<hsize_t>{vectorDouble.size()});

#ifdef H5PP_USE_EIGEN3
    Eigen::MatrixXd                                       matrixDouble(3, 2);
    Eigen::Matrix<size_t, 3, 2, Eigen::RowMajor>          matrixSizeTRowMajor;
    Eigen::Tensor<cplx, 4>                                tensorComplex(2, 3, 2, 3);
    Eigen::Tensor<double, 3>                              tensorDoubleRowMajor(2, 3, 4);
    Eigen::Matrix<Field2, Eigen::Dynamic, Eigen::Dynamic> field2Matrix(10, 10);

    for(Eigen::Index row = 0; row < matrixDouble.rows(); ++row)
        for(Eigen::Index col = 0; col < matrixDouble.cols(); ++col) matrixDouble(row, col) = static_cast<double>(row * 10 + col) / 3.0;
    for(Eigen::Index row = 0; row < matrixSizeTRowMajor.rows(); ++row)
        for(Eigen::Index col = 0; col < matrixSizeTRowMajor.cols(); ++col)
            matrixSizeTRowMajor(row, col) = static_cast<size_t>(row * 10 + col);
    for(int i = 0; i < tensorComplex.dimension(0); ++i) {
        for(int j = 0; j < tensorComplex.dimension(1); ++j) {
            for(int k = 0; k < tensorComplex.dimension(2); ++k)
                for(int l = 0; l < tensorComplex.dimension(3); ++l) tensorComplex(i, j, k, l) = cplx(i + j + k + l, i - j + k - l);
        }
    }
    for(int i = 0; i < tensorDoubleRowMajor.dimension(0); ++i) {
        for(int j = 0; j < tensorDoubleRowMajor.dimension(1); ++j)
            for(int k = 0; k < tensorDoubleRowMajor.dimension(2); ++k)
                tensorDoubleRowMajor(i, j, k) = static_cast<double>(i * 100 + j * 10 + k);
    }
    for(int row = 0; row < field2Matrix.rows(); ++row)
        for(int col = 0; col < field2Matrix.cols(); ++col) field2Matrix(row, col) = {static_cast<double>(row), static_cast<double>(col)};

    Eigen::Map<Eigen::VectorXd>                vectorMapDouble(vectorDouble.data(), static_cast<long>(vectorDouble.size()));
    Eigen::Map<Eigen::MatrixXd>                matrixMapDouble(matrixDouble.data(), matrixDouble.rows(), matrixDouble.cols());
    Eigen::TensorMap<Eigen::Tensor<double, 2>> tensorMapDouble(matrixDouble.data(), matrixDouble.rows(), matrixDouble.cols());
    Eigen::MatrixXd                            vectorMatrix(10, 1);
    for(Eigen::Index row = 0; row < vectorMatrix.rows(); ++row) vectorMatrix(row, 0) = static_cast<double>(row + 1);

    require_roundtrip(file, matrixDouble, "matrixDouble");
    require_roundtrip(file, matrixSizeTRowMajor, "matrixSizeTRowMajor");
    require_roundtrip(file, tensorComplex, "tensorComplex");
    require_roundtrip(file, tensorDoubleRowMajor, "tensorDoubleRowMajor");
    require_roundtrip(file, field2Matrix, "field2Matrix");
    require_roundtrip<Eigen::Map<Eigen::VectorXd>, Eigen::VectorXd>(file, vectorMapDouble, "vectorMapDouble");
    require_roundtrip<Eigen::Map<Eigen::MatrixXd>, Eigen::MatrixXd>(file, matrixMapDouble, "matrixMapDouble");
    require_roundtrip<Eigen::TensorMap<Eigen::Tensor<double, 2>>, Eigen::Tensor<double, 2>>(file, tensorMapDouble, "tensorMapDouble");
    require_roundtrip<Eigen::MatrixXd, Eigen::VectorXd>(file, vectorMatrix, "vectorMatrix");
#endif
}

TEST_CASE("Dataset API covers canonical create and write flows across layouts", "[dataset][api]") {
    auto file = h5pp::File(make_path("datasetApi"), h5pp::FileAccess::REPLACE, 0);

    for(size_t idx = 0; idx < layouts.size(); ++idx) {
        SECTION(h5pp::format("handle create: {}", layout_names[idx])) {
            h5pp::DatasetCreateOptions create;
            create.h5Type   = H5Tcopy(H5T_NATIVE_DOUBLE);
            create.dims     = {4};
            create.h5Layout = layouts[idx];

            auto path         = h5pp::format("createGroup/vectorDouble_{}", layout_names[idx]);
            auto created_info = file.dataset(path).create(create);
            REQUIRE(created_info.dsetPath.value() == path);
            require_zero_vector(file, path);
        }
    }

    std::vector<double> data = {1, 2, 3, 4};

    h5pp::DatasetCreateOptions chunked_create;
    chunked_create.h5Type    = H5Tcopy(H5T_NATIVE_DOUBLE);
    chunked_create.dims      = {4};
    chunked_create.h5Layout  = H5D_CHUNKED;
    chunked_create.dimsChunk = {2};
    chunked_create.dimsMax   = {H5S_UNLIMITED};

    file.writeDataset(data, "manual/writeShort", chunked_create);
    auto short_info = file.dataset("manual/writeShort").getInfo();
    REQUIRE(short_info.h5Layout.value() == H5D_CHUNKED);
    REQUIRE(short_info.dsetChunk.value() == std::vector<hsize_t>{2});
    REQUIRE(short_info.dsetDimsMax.value() == std::vector<hsize_t>{H5S_UNLIMITED});
    REQUIRE(file.readDataset<std::vector<double>>("manual/writeShort") == data);

    h5pp::DatasetCreateOptions typed_create;
    typed_create.h5Type   = H5Tcopy(H5T_NATIVE_DOUBLE);
    typed_create.dims     = {4};
    typed_create.h5Layout = H5D_CONTIGUOUS;
    file.dataset("manual/writeLong").ensure(typed_create).write(data);
    REQUIRE(file.dataset("manual/writeLong").getInfo().h5Layout.value() == H5D_CONTIGUOUS);
    REQUIRE(file.readDataset<std::vector<double>>("manual/writeLong") == data);

    h5pp::DatasetWriteOptions write;
    write.resizePolicy = h5pp::ResizePolicy::FIT;
    file.writeDataset(std::vector<double>{5, 6}, "manual/writeShort", write);
    REQUIRE(file.readDataset<std::vector<double>>("manual/writeShort") == std::vector<double>{5, 6});
    REQUIRE(file.dataset("manual/writeShort").getInfo().dsetDims.value() == std::vector<hsize_t>{2});
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
