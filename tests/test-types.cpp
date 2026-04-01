#include <array>
#include <catch2/catch_all.hpp>
#include <complex>
#include <cstddef>
#include <cstring>
#include <h5pp/h5pp.h>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {
    std::string make_path(std::string_view name) {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }

    template<typename T>
    struct Vec2 {
        T x;
        T y;
    };

    template<typename T>
    struct Vec3 {
        T x;
        T y;
        T z;
    };

    template<typename T>
    bool equal_value(const T &lhs, const T &rhs) {
#if defined(H5PP_USE_QUADMATH) || defined(H5PP_USE_FLOAT128)
        if constexpr((std::is_same_v<std::decay_t<T>, h5pp::fp128>) || (std::is_same_v<std::decay_t<T>, h5pp::cx128>)) {
            return std::memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
        } else
#endif
        {
            return lhs == rhs;
        }
    }

    template<typename T>
    void require_equal(const Vec2<T> &lhs, const Vec2<T> &rhs) {
        REQUIRE(equal_value(lhs.x, rhs.x));
        REQUIRE(equal_value(lhs.y, rhs.y));
    }

    template<typename T>
    void require_equal(const Vec3<T> &lhs, const Vec3<T> &rhs) {
        REQUIRE(equal_value(lhs.x, rhs.x));
        REQUIRE(equal_value(lhs.y, rhs.y));
        REQUIRE(equal_value(lhs.z, rhs.z));
    }

    template<typename T>
    void require_equal(const T &lhs, const T &rhs) {
        REQUIRE(equal_value(lhs, rhs));
    }

    template<typename T>
    T sample_scalar() {
        using D = std::decay_t<T>;
        if constexpr(std::is_same_v<D, bool>) return true;
        else if constexpr(std::is_same_v<D, std::byte>) return std::byte {0x5a};
        else if constexpr(std::is_floating_point_v<D>) return static_cast<D>(-355.0L / 113.0L);
        else if constexpr(std::is_signed_v<D>) return static_cast<D>(-42);
        else return static_cast<D>(42);
    }

    template<typename Scalar>
    std::complex<Scalar> sample_complex() {
        return {sample_scalar<Scalar>(), static_cast<Scalar>(sample_scalar<Scalar>() / static_cast<Scalar>(2))};
    }

    template<typename Scalar>
    void check_scalar_roundtrip(h5pp::File &file, std::string_view path) {
        auto value = sample_scalar<Scalar>();
        file.dataset(path).write(value);
        auto readback = file.dataset(path).read<Scalar>();
        require_equal(readback, value);

        auto info = file.dataset(path).getTypeInfo();
        REQUIRE(info.cppTypeBytes);
        REQUIRE(info.cppTypeBytes.value() == sizeof(Scalar));
    }

    template<typename Scalar>
    void check_complex_roundtrip(h5pp::File &file, std::string_view path) {
        auto value = sample_complex<Scalar>();
        file.dataset(path).write(value);
        auto readback = file.dataset(path).read<std::complex<Scalar>>();
        require_equal(readback, value);

        auto info = file.dataset(path).getTypeInfo();
        REQUIRE(info.cppTypeBytes);
        REQUIRE(info.cppTypeBytes.value() == sizeof(std::complex<Scalar>));
    }
}

static_assert(h5pp::type::sfinae::has_data_v<std::vector<double>>);
static_assert(h5pp::type::sfinae::has_size_v<std::vector<double>>);
static_assert(h5pp::type::sfinae::has_data_v<std::span<double>>);
static_assert(h5pp::type::sfinae::has_size_v<std::span<double>>);
static_assert(!h5pp::type::sfinae::has_data_v<std::vector<bool>>);

TEST_CASE("Datasets support the advertised scalar families and selected std::complex variants", "[types][dataset]") {
    h5pp::File file(make_path("types-scalars"), h5pp::FileAccess::REPLACE, 0);

    check_scalar_roundtrip<short>(file, "native/short");
    check_scalar_roundtrip<int>(file, "native/int");
    check_scalar_roundtrip<long>(file, "native/long");
    check_scalar_roundtrip<long long>(file, "native/longLong");
    check_scalar_roundtrip<unsigned short>(file, "native/unsignedShort");
    check_scalar_roundtrip<unsigned int>(file, "native/unsignedInt");
    check_scalar_roundtrip<unsigned long>(file, "native/unsignedLong");
    check_scalar_roundtrip<unsigned long long>(file, "native/unsignedLongLong");
    check_scalar_roundtrip<float>(file, "native/float");
    check_scalar_roundtrip<double>(file, "native/double");
    check_scalar_roundtrip<long double>(file, "native/longDouble");
    check_scalar_roundtrip<int8_t>(file, "fixed/int8");
    check_scalar_roundtrip<int16_t>(file, "fixed/int16");
    check_scalar_roundtrip<int32_t>(file, "fixed/int32");
    check_scalar_roundtrip<int64_t>(file, "fixed/int64");
    check_scalar_roundtrip<uint8_t>(file, "fixed/uint8");
    check_scalar_roundtrip<uint16_t>(file, "fixed/uint16");
    check_scalar_roundtrip<uint32_t>(file, "fixed/uint32");
    check_scalar_roundtrip<uint64_t>(file, "fixed/uint64");
    check_scalar_roundtrip<bool>(file, "native/bool");
    check_scalar_roundtrip<std::byte>(file, "native/byte");

    check_complex_roundtrip<int>(file, "complex/int");
    check_complex_roundtrip<float>(file, "complex/float");
    check_complex_roundtrip<double>(file, "complex/double");
    check_complex_roundtrip<long double>(file, "complex/longDouble");
}

TEST_CASE("Datasets accept common contiguous buffer adapters and text-like views", "[types][buffer]") {
    h5pp::File file(make_path("types-buffers"), h5pp::FileAccess::REPLACE, 0);

    std::string      ownedText = "dataset text via string_view";
    std::string_view textView  = ownedText;
    file.dataset("buffer/textView").write(textView);
    REQUIRE(file.dataset("buffer/textView").read<std::string>() == ownedText);

    std::array<int, 5> intArray = {1, 2, 3, 4, 5};
    file.dataset("buffer/stdArray").write(intArray);
    REQUIRE(file.dataset("buffer/stdArray").read<std::vector<int>>() == std::vector<int>{1, 2, 3, 4, 5});

    std::vector<double> spanBacking = {1.0, -2.0, 3.5, 4.25};
    std::span<const double> spanView(spanBacking.data(), spanBacking.size());
    file.dataset("buffer/span").write(spanView);
    REQUIRE(file.dataset("buffer/span").read<std::vector<double>>() == spanBacking);

    std::vector<std::byte> byteBuffer = {std::byte {0x00}, std::byte {0x7f}, std::byte {0xaa}, std::byte {0xff}};
    file.dataset("buffer/bytes").write(byteBuffer);
    REQUIRE(file.dataset("buffer/bytes").read<std::vector<std::byte>>() == byteBuffer);
}

TEST_CASE("Datasets support CUDA-style x-y and x-y-z POD structs and vectors thereof", "[types][scalarn]") {
    h5pp::File file(make_path("types-scalarn"), h5pp::FileAccess::REPLACE, 0);

    Vec2<double> vec2 = {1.25, -2.5};
    Vec3<int>    vec3 = {3, -4, 5};
    std::vector<Vec2<float>> vec2Vector = {
        {1.0f, 2.0f},
        {-3.0f, 4.5f},
        {6.0f, -7.25f},
    };
    std::vector<Vec3<double>> vec3Vector = {
        {1.0, 2.0, 3.0},
        {-4.0, 5.5, -6.5},
    };

    file.dataset("scalarn/vec2").write(vec2);
    file.dataset("scalarn/vec3").write(vec3);
    file.dataset("scalarn/vec2Vector").write(vec2Vector);
    file.dataset("scalarn/vec3Vector").write(vec3Vector);

    require_equal(file.dataset("scalarn/vec2").read<Vec2<double>>(), vec2);
    require_equal(file.dataset("scalarn/vec3").read<Vec3<int>>(), vec3);

    auto vec2VectorRead = file.dataset("scalarn/vec2Vector").read<std::vector<Vec2<float>>>();
    REQUIRE(vec2VectorRead.size() == vec2Vector.size());
    for(size_t idx = 0; idx < vec2Vector.size(); ++idx) require_equal(vec2VectorRead[idx], vec2Vector[idx]);

    auto vec3VectorRead = file.dataset("scalarn/vec3Vector").read<std::vector<Vec3<double>>>();
    REQUIRE(vec3VectorRead.size() == vec3Vector.size());
    for(size_t idx = 0; idx < vec3Vector.size(); ++idx) require_equal(vec3VectorRead[idx], vec3Vector[idx]);
}

#ifdef H5PP_USE_EIGEN3
TEST_CASE("Datasets accept selected Eigen view and array adapters", "[types][eigen]") {
    h5pp::File file(make_path("types-eigen"), h5pp::FileAccess::REPLACE, 0);

    Eigen::MatrixXd matrix(3, 2);
    for(Eigen::Index row = 0; row < matrix.rows(); ++row)
        for(Eigen::Index col = 0; col < matrix.cols(); ++col) matrix(row, col) = static_cast<double>(row * 10 + col) / 7.0;

    Eigen::Ref<Eigen::MatrixXd> matrixRef(matrix);
    file.dataset("eigen/refMatrix").write(matrixRef);
    REQUIRE(file.dataset("eigen/refMatrix").read<Eigen::MatrixXd>() == matrix);

    Eigen::ArrayXXf array(2, 3);
    for(Eigen::Index row = 0; row < array.rows(); ++row)
        for(Eigen::Index col = 0; col < array.cols(); ++col) array(row, col) = static_cast<float>(row * 10 + col) / 5.0f;

    file.dataset("eigen/array").write(array);
    auto arrayRead = file.dataset("eigen/array").read<Eigen::ArrayXXf>();
    REQUIRE(arrayRead.rows() == array.rows());
    REQUIRE(arrayRead.cols() == array.cols());
    for(Eigen::Index row = 0; row < array.rows(); ++row)
        for(Eigen::Index col = 0; col < array.cols(); ++col) REQUIRE(arrayRead(row, col) == array(row, col));
}
#endif

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
