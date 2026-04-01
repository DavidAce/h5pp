#include <array>
#include <catch2/catch_all.hpp>
#include <complex>
#include <cstring>
#include <h5pp/v1/h5pp.h>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

namespace {
    struct Field2 {
        double x;
        double y;

        bool operator==(const Field2 &other) const { return x == other.x && y == other.y; }
    };

    struct Field3 {
        float x;
        float y;
        float z;

        bool operator==(const Field3 &other) const { return x == other.x && y == other.y && z == other.z; }
    };

    std::string make_path(std::string_view name) {
        h5pp::fs::create_directories("output");
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
}

TEST_CASE("Simple writes cover scalars, containers, pointers, vlen data and user structs", "[simple-write]") {
    auto       path = make_path("simpleWrite");
    h5pp::v1::File file(path, h5pp::FileAccess::REPLACE, 0);

    bool                 boolean_value   = true;
    std::string          string_value    = "This is a string";
    char                 char_array[100] = "This is a char array";
    double               double_value    = 2.0;
    std::complex<double> complex_value{3.0, 4.0};
    int                  array_int[5] = {1, 2, 3, 4, 5};

    std::vector<int>                  vector_int(10, 42);
    std::vector<long>                 vector_long(10, 42);
    std::vector<unsigned int>         vector_uint(10, 42);
    std::vector<unsigned long>        vector_ulong(10, 42);
    std::vector<float>                vector_float(10, 42.0f);
    std::vector<double>               vector_double(10, 42.0);
    std::vector<std::complex<int>>    vector_complex_int(10, std::complex<int>(42, 7));
    std::vector<std::complex<double>> vector_complex_double(10, std::complex<double>(10.0, 5.0));

    h5pp::varr_t<double>              vlen_double = std::vector<double>(10, 42.0);
    std::vector<h5pp::varr_t<double>> vector_vlen_double;
    for(size_t i = 0; i < 10; ++i) vector_vlen_double.emplace_back(std::vector<double>(i + 1, static_cast<double>(i)));

    Field2 field2{0.53, 0.45};
    Field3 field3{0.54f, 0.56f, 0.58f};

    std::vector<Field2> field2_array(10, {0.3, 0.8});
    std::vector<Field3> field3_array(10, {0.3f, 0.8f, 1.4f});
    for(size_t idx = 0; idx < field2_array.size(); ++idx) {
        field2_array[idx] = Field2{0.3 + static_cast<double>(idx), 0.8 - static_cast<double>(idx) / 10.0};
        field3_array[idx] = Field3{0.3f + static_cast<float>(idx), 0.8f - static_cast<float>(idx) / 10.0f, 1.4f + static_cast<float>(idx)};
    }

    file.writeDataset(boolean_value, "simpleWriteGroup/Boolean");
    file.writeDataset(false, "simpleWriteGroup/BooleanRval");
    file.writeDataset(string_value, "simpleWriteGroup/String");
    file.writeDataset(char_array, "simpleWriteGroup/Char");
    file.writeDataset(double_value, "simpleWriteGroup/Double");
    file.writeDataset(complex_value, "simpleWriteGroup/ComplexDouble");
    file.writeDataset(array_int, "simpleWriteGroup/arrayInt");
    file.writeDataset(vector_int, "simpleWriteGroup/vectorInt");
    file.writeDataset(vector_long, "simpleWriteGroup/vectorLong");
    file.writeDataset(vector_uint, "simpleWriteGroup/vectorUint");
    file.writeDataset(vector_ulong, "simpleWriteGroup/vectorUlong");
    file.writeDataset(vector_float, "simpleWriteGroup/vectorFloat");
    file.writeDataset(vector_double, "simpleWriteGroup/vectorDouble");
    file.writeDataset(vector_complex_int, "simpleWriteGroup/vectorComplexInt");
    file.writeDataset(vector_complex_double, "simpleWriteGroup/vectorComplexDouble");
    file.writeDataset(field2, "simpleWriteGroup/field2");
    file.writeDataset(field3, "simpleWriteGroup/field3");
    file.writeDataset(field2_array, "simpleWriteGroup/field2array");
    file.writeDataset(field3_array, "simpleWriteGroup/field3array");
    file.writeDataset(vlen_double, "simpleWriteGroup/vlenDouble");
    file.writeDataset(vector_vlen_double, "simpleWriteGroup/vectorVlenDouble");

    auto pointer_int_write = vector_int;
    std::iota(pointer_int_write.begin(), pointer_int_write.end(), -4);
    auto pointer_double_write = vector_double;
    for(size_t idx = 0; idx < pointer_double_write.size(); ++idx) pointer_double_write[idx] = static_cast<double>(idx) / 3.0;
    file.writeDataset(pointer_int_write.data(), "simpleWriteGroup/vectorInt", pointer_int_write.size());
    file.writeDataset(pointer_double_write.data(), "simpleWriteGroup/vectorDouble", pointer_double_write.size());

    REQUIRE(file.readDataset<bool>("simpleWriteGroup/Boolean"));
    REQUIRE_FALSE(file.readDataset<bool>("simpleWriteGroup/BooleanRval"));
    REQUIRE(file.readDataset<std::string>("simpleWriteGroup/String") == string_value);
    REQUIRE(file.readDataset<std::string>("simpleWriteGroup/Char") == "This is a char array");
    REQUIRE(file.readDataset<double>("simpleWriteGroup/Double") == double_value);
    REQUIRE(file.readDataset<std::complex<double>>("simpleWriteGroup/ComplexDouble") == complex_value);
    REQUIRE(file.readDataset<std::vector<int>>("simpleWriteGroup/arrayInt") == std::vector<int>{1, 2, 3, 4, 5});
    REQUIRE(file.readDataset<std::vector<int>>("simpleWriteGroup/vectorInt") == pointer_int_write);
    REQUIRE(file.readDataset<std::vector<long>>("simpleWriteGroup/vectorLong") == vector_long);
    REQUIRE(file.readDataset<std::vector<unsigned int>>("simpleWriteGroup/vectorUint") == vector_uint);
    REQUIRE(file.readDataset<std::vector<unsigned long>>("simpleWriteGroup/vectorUlong") == vector_ulong);
    REQUIRE(file.readDataset<std::vector<float>>("simpleWriteGroup/vectorFloat") == vector_float);
    REQUIRE(file.readDataset<std::vector<double>>("simpleWriteGroup/vectorDouble") == pointer_double_write);
    REQUIRE(file.readDataset<std::vector<std::complex<int>>>("simpleWriteGroup/vectorComplexInt") == vector_complex_int);
    REQUIRE(file.readDataset<std::vector<std::complex<double>>>("simpleWriteGroup/vectorComplexDouble") == vector_complex_double);
    REQUIRE(file.readDataset<Field2>("simpleWriteGroup/field2") == field2);
    REQUIRE(file.readDataset<Field3>("simpleWriteGroup/field3") == field3);
    REQUIRE(file.readDataset<std::vector<Field2>>("simpleWriteGroup/field2array") == field2_array);
    REQUIRE(file.readDataset<std::vector<Field3>>("simpleWriteGroup/field3array") == field3_array);
    REQUIRE(file.readDataset<h5pp::varr_t<double>>("simpleWriteGroup/vlenDouble") == vlen_double);
    auto vector_vlen_double_read = file.readDataset<std::vector<h5pp::varr_t<double>>>("simpleWriteGroup/vectorVlenDouble");
    REQUIRE(vector_vlen_double_read.size() == vector_vlen_double.size());
    for(size_t idx = 0; idx < vector_vlen_double.size(); idx++) {
        REQUIRE(std::vector<double>(vector_vlen_double_read[idx].begin(), vector_vlen_double_read[idx].end()) ==
                std::vector<double>(vector_vlen_double[idx].begin(), vector_vlen_double[idx].end()));
    }

    auto vector_int_info = file.getDatasetInfo("simpleWriteGroup/vectorInt");
    REQUIRE(vector_int_info.dsetDims);
    REQUIRE(vector_int_info.dsetDims.value() == std::vector<hsize_t>{pointer_int_write.size()});

    auto found_datasets = file.findDatasets();
    REQUIRE(std::find(found_datasets.begin(), found_datasets.end(), "simpleWriteGroup/vectorComplexDouble") != found_datasets.end());
    REQUIRE(std::find(found_datasets.begin(), found_datasets.end(), "simpleWriteGroup/vlenDouble") != found_datasets.end());

#ifdef H5PP_USE_EIGEN3
    Eigen::MatrixXi  matrix_int(2, 2);
    Eigen::MatrixXd  matrix_double(2, 2);
    Eigen::MatrixXcd matrix_complex_double(2, 2);
    matrix_int << 1, 2, 3, 4;
    matrix_double << 1.5, 2.5, 3.5, 4.5;
    for(Eigen::Index row = 0; row < matrix_complex_double.rows(); ++row)
        for(Eigen::Index col = 0; col < matrix_complex_double.cols(); ++col)
            matrix_complex_double(row, col) = std::complex<double>(static_cast<double>(row) + 1.0, static_cast<double>(-col) - 0.5);

    file.writeDataset(matrix_int, "simpleWriteGroup/matrixInt");
    file.writeDataset(matrix_double, "simpleWriteGroup/matrixDouble");
    file.writeDataset(matrix_complex_double, "simpleWriteGroup/matrixComplexDouble");

    REQUIRE(file.readDataset<Eigen::MatrixXi>("simpleWriteGroup/matrixInt") == matrix_int);
    REQUIRE(file.readDataset<Eigen::MatrixXd>("simpleWriteGroup/matrixDouble") == matrix_double);
    REQUIRE(file.readDataset<Eigen::MatrixXcd>("simpleWriteGroup/matrixComplexDouble") == matrix_complex_double);

    if(h5pp::hdf5::isCompressionAvaliable()) {
        file.setCompressionLevel(9);
        Eigen::Tensor<double, 4> big_tensor(12, 18, 8, 3);
        for(Eigen::Index i = 0; i < big_tensor.dimension(0); ++i) {
            for(Eigen::Index j = 0; j < big_tensor.dimension(1); ++j) {
                for(Eigen::Index k = 0; k < big_tensor.dimension(2); ++k)
                    for(Eigen::Index l = 0; l < big_tensor.dimension(3); ++l)
                        big_tensor(i, j, k, l) = static_cast<double>(i * 1000 + j * 100 + k * 10 + l);
            }
        }

        file.writeDataset(big_tensor, "compressedWriteGroup/bigTensor");
        auto tensor_info = file.getDatasetInfo("compressedWriteGroup/bigTensor");
        REQUIRE(tensor_info.dsetDims);
        REQUIRE(tensor_info.dsetDims.value() == std::vector<hsize_t>{12, 18, 8, 3});
        REQUIRE(tensor_info.compression);
        REQUIRE(tensor_info.h5Layout);
        REQUIRE(tensor_info.h5Layout.value() == H5D_CONTIGUOUS);
        REQUIRE(tensor_info.compression.value() == -1);
    }
#endif

#if defined(H5PP_USE_FLOAT128) || defined(H5PP_USE_QUADMATH)
    h5pp::fp128 twopi_real = 6.28318530717958623199592693708837032318115234375;
    h5pp::cx128 twopi_cplx = 6.28318530717958623199592693708837032318115234375;
    file.writeDataset(twopi_real, "simpleWriteGroup/twopi_real");
    file.writeDataset(twopi_cplx, "simpleWriteGroup/twopi_cplx");
    REQUIRE(require_equal_value(file.readDataset<h5pp::fp128>("simpleWriteGroup/twopi_real"), twopi_real));
    REQUIRE(require_equal_value(file.readDataset<h5pp::cx128>("simpleWriteGroup/twopi_cplx"), twopi_cplx));
#endif
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    return session.run();
}
