
#include <h5pp/details/h5ppFormatComplex.h>
#include <h5pp/h5pp.h>

template<typename T>
void compareScalar(const T &lhs, const T &rhs) {
    if constexpr(h5pp::type::sfinae::is_Scalar2_v<T>) {
        if(lhs.x != rhs.x) throw std::runtime_error("lhs.x != rhs.x");
        if(lhs.y != rhs.y) throw std::runtime_error("lhs.y != rhs.y");
    } else if constexpr(h5pp::type::sfinae::is_Scalar3_v<T>) {
        if(lhs.x != rhs.x) throw std::runtime_error("lhs.x != rhs.x");
        if(lhs.y != rhs.y) throw std::runtime_error("lhs.y != rhs.y");
        if(lhs.z != rhs.z) throw std::runtime_error("lhs.z != rhs.z");
    }
}

// Store some dummy data to an hdf5 file

template<typename WriteType, typename ReadType = WriteType>
void test_h5pp(h5pp::File &file, const WriteType &writeData, std::string_view dsetpath, std::string tag = "") {
    using namespace h5pp::type::sfinae;
    if(tag.empty()) tag = dsetpath;
    h5pp::logger::log->info("Writing {}", tag);
    file.writeDataset(writeData, dsetpath);
    h5pp::logger::log->debug("Reading {}", tag);
    auto readData = file.readDataset<ReadType>(dsetpath);
    if constexpr(h5pp::type::sfinae::is_ScalarN_v<ReadType>) {
        compareScalar(writeData, readData);
    } else if constexpr(h5pp::type::sfinae::has_ScalarN_v<ReadType>) {
        if(writeData.size() != readData.size()) throw std::runtime_error("Size mismatch in ScalarN container");
#ifdef H5PP_USE_EIGEN3
        if constexpr(h5pp::type::sfinae::is_eigen_matrix_v<ReadType>) {
            for(Eigen::Index j = 0; j < writeData.cols(); j++)
                for(Eigen::Index i = 0; i < writeData.rows(); i++) compareScalar(writeData(i, j), readData(i, j));
        } else
#endif
            for(size_t i = 0; i < static_cast<size_t>(writeData.size()); i++) compareScalar(writeData[i], readData[i]);
    }
#ifdef H5PP_USE_EIGEN3
    else if constexpr(h5pp::type::sfinae::is_eigen_tensor_v<WriteType> and h5pp::type::sfinae::is_eigen_tensor_v<ReadType>) {
        Eigen::Map<const Eigen::Matrix<typename WriteType::Scalar, Eigen::Dynamic, 1>> tensorMap(writeData.data(), writeData.size());
        Eigen::Map<const Eigen::Matrix<typename ReadType::Scalar, Eigen::Dynamic, 1>>  tensorMapRead(readData.data(), readData.size());
        if(tensorMap != tensorMapRead) {
            if constexpr(WriteType::NumIndices == 4) {
                for(int i = 0; i < writeData.dimension(0); i++) {
                    for(int j = 0; j < writeData.dimension(1); j++) {
                        for(int k = 0; k < writeData.dimension(2); k++) {
                            for(int l = 0; l < writeData.dimension(3); l++) {
                                auto w = writeData(i, j, k, l);
                                auto r = readData(i, j, k, l);
                                h5pp::print("[{} {} {} {}]: {} + i{} == {} + i{}",
                                            i,
                                            j,
                                            k,
                                            l,
                                            std::real(w),
                                            std::imag(w),
                                            std::real(r),
                                            std::imag(r));
                            }

                            h5pp::print("\n");
                        }
                    }
                }
            }
            if constexpr(WriteType::NumIndices == 3) {
                for(int i = 0; i < writeData.dimension(0); i++) {
                    for(int j = 0; j < writeData.dimension(1); j++) {
                        for(int k = 0; k < writeData.dimension(2); k++) {
                            h5pp::print("[{} {} {}]: {} == {}", i, j, k, writeData(i, j, k), readData(i, j, k));
                            h5pp::print("\n");
                        }
                    }
                }
            }
            throw std::runtime_error("tensor written != tensor read");
        }
    }
#endif
    else {
        if(writeData != readData) {
#if H5PP_USE_FMT
    #if defined(H5PP_USE_FLOAT128)
            if constexpr(std::is_same_v<ReadType, __float128>) {
                return;
            } else
    #endif
                if constexpr(not h5pp::type::sfinae::is_eigen_any_v<ReadType>) {
                if constexpr(h5pp::type::sfinae::is_std_complex_v<ReadType> or h5pp::type::sfinae::has_std_complex_v<ReadType>) {
    #if defined(FMT_USE_COMPLEX)
                    h5pp::print("Wrote: \n{}\n", writeData);
                    h5pp::print("Read: \n{}\n", readData);
    #endif
                } else {
                    h5pp::print("Wrote: \n{}\n", writeData);
                    h5pp::print("Read: \n{}\n", readData);
                }
            }
#endif
            throw std::runtime_error("Data mismatch: Write != Read");
        }
    }

    h5pp::logger::log->debug("Success");
}

template<auto size, typename WriteType, typename ReadType = WriteType, typename DimsType = int>
void test_h5pp(h5pp::File &file, const WriteType *writeData, const DimsType &dims, std::string_view dsetpath, std::string tag = "") {
    if(tag.empty()) tag = dsetpath;
    h5pp::logger::log->info("Writing {}", tag);
    file.writeDataset(writeData, dsetpath, dims);
    h5pp::logger::log->debug("Reading {}", tag);
    auto *readData = new ReadType[size];
    file.readDataset(readData, dsetpath, dims);
    for(size_t i = 0; i < size; i++) {
        if(writeData[i] != readData[i]) {
            for(size_t j = 0; j < size; j++) h5pp::print("Wrote [{}]: {} | Read [{}]: {}", j, writeData[j], j, readData[j]);
            throw std::runtime_error("Data mismatch: Write != Read");
        }
    }

    delete[] readData;
    h5pp::logger::log->debug("Success");
}

int main() {
    using cplx = std::complex<double>;

    static_assert(
        h5pp::type::sfinae::has_data_v<std::vector<double>> and
        "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    std::string outputFilename = "output/readWrite.h5";
    size_t      logLevel       = 2;
    h5pp::File  file(outputFilename, H5F_ACC_TRUNC | H5F_ACC_RDWR, logLevel);
    file.setLogLevel(0);
    // Generate dummy data
    std::vector<int>    emptyVector;
    std::string         stringDummy = "Dummy string with spaces";
    std::complex<float> cplxFloat(1, 1);
    std::vector<double> vectorDouble  = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,
                                         1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0};
    std::vector<cplx>   vectorComplex = {
        { -0.191154,  0.326211},
        {  0.964728, -0.712335},
        {-0.0351791,  -0.10264},
        {  0.177544,   0.99999}
    };
    auto *cStyleDoubleArray = new double[10];
    for(size_t i = 0; i < 10; i++) cStyleDoubleArray[i] = static_cast<double>(i);

    struct Field2 {
        double x;
        double y;
    };
    struct Field3 {
        double x;
        double y;
        double z;
    };
    Field2              field2{0.53, 0.45};
    Field3              field3{0.54, 0.56, 0.58};
    std::vector<Field2> field2vector(10);
    for(size_t i = 0; i < field2vector.size(); i++) {
        auto d            = static_cast<double>(i);
        field2vector[i].x = 2.3 * d;
        field2vector[i].y = 20.5 * d;
    }
    std::vector<Field3> field3vector(10);
    for(size_t i = 0; i < field3vector.size(); i++) {
        auto d            = static_cast<double>(i);
        field3vector[i].x = 2.3 * d;
        field3vector[i].y = 20.5 * d;
        field3vector[i].z = 200.9 * d;
    }

    h5pp::varr_t<double>              vlenDouble       = {1.0, 2.0, 3.0, 4.0};
    std::vector<h5pp::varr_t<double>> vectorVlenDouble = {
        {1.0},
        {2.0, 3.0},
        {4.0, 5.0, 6.0},
        {7.0, 8.0, 9.0, 10.0}
    };

#ifdef H5PP_USE_EIGEN3
    Eigen::MatrixXd                              matrixDouble        = Eigen::MatrixXd::Random(3, 2);
    Eigen::Matrix<size_t, 3, 2, Eigen::RowMajor> matrixSizeTRowMajor = Eigen::Matrix<size_t, 3, 2, Eigen::RowMajor>::Random(3, 2);
    Eigen::Tensor<cplx, 4>                       tensorComplex(2, 3, 2, 3);
    tensorComplex.setRandom();
    Eigen::Tensor<double, 3> tensorDoubleRowMajor(2, 3, 4);
    tensorDoubleRowMajor.setRandom();

    Eigen::Matrix<Field2, Eigen::Dynamic, Eigen::Dynamic> field2Matrix(10, 10);
    for(int row = 0; row < field2Matrix.rows(); row++)
        for(int col = 0; col < field2Matrix.cols(); col++) field2Matrix(row, col) = {static_cast<double>(row), static_cast<double>(col)};

    Eigen::Map<Eigen::VectorXd>                vectorMapDouble(vectorDouble.data(), static_cast<long>(vectorDouble.size()));
    Eigen::Map<Eigen::MatrixXd>                matrixMapDouble(matrixDouble.data(), matrixDouble.rows(), matrixDouble.cols());
    Eigen::TensorMap<Eigen::Tensor<double, 2>> tensorMapDouble(matrixDouble.data(), matrixDouble.rows(), matrixDouble.cols());
    Eigen::MatrixXd                            vectorMatrix = Eigen::MatrixXd::Random(10, 1);
#endif
    // Test reading and writing dummy data
    test_h5pp(file, emptyVector, "emptyVector");
    test_h5pp(file, stringDummy, "stringDummy");
    test_h5pp(file, cplxFloat, "cplxFloat");
    test_h5pp(file, vectorDouble, "vectorDouble");
    test_h5pp(file, vectorComplex, "vectorComplex");
    test_h5pp<10, double>(file, cStyleDoubleArray, 10, "cStyleDoubleArray");
    delete[] cStyleDoubleArray;
    test_h5pp(file, field2, "field2");
    test_h5pp(file, field3, "field3");
    test_h5pp(file, field2vector, "field2vector");
    test_h5pp(file, field3vector, "field3vector");

    test_h5pp(file, vlenDouble, "vlenDouble");
    test_h5pp(file, vectorVlenDouble, "vectorVlenDouble");

    // Read data as std::vector<std::byte>
    auto vectorReadBytes = file.readDataset<std::vector<std::byte>>("vectorDouble");

#ifdef H5PP_USE_EIGEN3
    test_h5pp(file, matrixDouble, "matrixDouble");
    test_h5pp(file, matrixSizeTRowMajor, "matrixSizeTRowMajor");
    test_h5pp(file, tensorComplex, "tensorComplex");
    test_h5pp(file, tensorDoubleRowMajor, "tensorDoubleRowMajor");
    test_h5pp(file, field2Matrix, "field2Matrix");
    test_h5pp<Eigen::Map<Eigen::VectorXd>, Eigen::VectorXd>(file, vectorMapDouble, "vectorMapDouble");
    test_h5pp<Eigen::Map<Eigen::MatrixXd>, Eigen::MatrixXd>(file, matrixMapDouble, "matrixMapDouble");
    test_h5pp<Eigen::TensorMap<Eigen::Tensor<double, 2>>, Eigen::Tensor<double, 2>>(file, tensorMapDouble, "tensorMapDouble");
    test_h5pp<Eigen::MatrixXd, Eigen::VectorXd>(file, vectorMatrix, "vectorMatrix");
#endif

#if defined(H5PP_USE_FLOAT128)
    __float128 f128 = 6.28318530717958623199592693708837032318115234375;
    test_h5pp(file, f128, "__float128");
#endif

    auto foundLinksInRoot = file.findDatasets();
    for(auto &link : foundLinksInRoot) h5pp::logger::log->info("Found Link: {}", link);

    return 0;
}