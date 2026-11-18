#include <catch2/catch_all.hpp>
#include <cstring>
#include <h5pp/h5pp.h>
#include <string_view>
#include <typeindex>
#include <vector>

namespace {
    std::string make_path(std::string_view name) {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }
}

#if defined(H5PP_USE_QUADMATH) || defined(H5PP_USE_FLOAT128)
namespace {
    struct QuadRecord {
        h5pp::fp128 scalar;
        h5pp::cx128 complex;
    };

    [[nodiscard]] bool binary_equal(const h5pp::fp128 &lhs, const h5pp::fp128 &rhs) { return std::memcmp(&lhs, &rhs, sizeof(lhs)) == 0; }
    [[nodiscard]] bool binary_equal(const h5pp::cx128 &lhs, const h5pp::cx128 &rhs) { return std::memcmp(&lhs, &rhs, sizeof(lhs)) == 0; }

    void require_equal(const h5pp::fp128 &lhs, const h5pp::fp128 &rhs) { REQUIRE(binary_equal(lhs, rhs)); }
    void require_equal(const h5pp::cx128 &lhs, const h5pp::cx128 &rhs) { REQUIRE(binary_equal(lhs, rhs)); }

    void require_equal(const QuadRecord &lhs, const QuadRecord &rhs) {
        require_equal(lhs.scalar, rhs.scalar);
        require_equal(lhs.complex, rhs.complex);
    }

    template<typename T>
    void require_vector_equal(const std::vector<T> &lhs, const std::vector<T> &rhs) {
        REQUIRE(lhs.size() == rhs.size());
        for(size_t idx = 0; idx < lhs.size(); ++idx) require_equal(lhs[idx], rhs[idx]);
    }

    [[nodiscard]] h5pp::fp128 make_value(long double numerator, long double denominator = 1.0L) {
        return static_cast<h5pp::fp128>(numerator) / static_cast<h5pp::fp128>(denominator);
    }

    [[nodiscard]] h5pp::cx128 make_complex(const h5pp::fp128 &real, const h5pp::fp128 &imag) {
#if defined(H5PP_USE_QUADMATH)
        h5pp::cx128 value {};
        __real__ value = real;
        __imag__ value = imag;
        return value;
#else
        return h5pp::cx128 {real, imag};
#endif
    }

    [[nodiscard]] h5pp::hid::h5t make_record_type() {
        h5pp::hid::h5t record_type = H5Tcreate(H5T_COMPOUND, sizeof(QuadRecord));
        H5Tinsert(record_type, "scalar", HOFFSET(QuadRecord, scalar), h5pp::type::getH5Type<h5pp::fp128>());
        H5Tinsert(record_type, "complex", HOFFSET(QuadRecord, complex), h5pp::type::getH5Type<h5pp::cx128>());
        return record_type;
    }
}

TEST_CASE("float128 scalars and complex values round-trip as datasets and attributes", "[float128][scalar]") {
    h5pp::File file(make_path("float128-scalar"), h5pp::FileAccess::REPLACE, 0);

    auto scalarValue  = make_value(-355.0L, 113.0L);
    auto complexValue = make_complex(make_value(22.0L, 7.0L), make_value(-13.0L, 17.0L));

    file.dataset("quad/scalar").write(scalarValue);
    file.dataset("quad/complex").write(complexValue);
    file.attribute("quad/scalar", "scalarAttr").write(scalarValue);
    file.attribute("quad/complex", "complexAttr").write(complexValue);

    require_equal(file.dataset("quad/scalar").read<h5pp::fp128>(), scalarValue);
    require_equal(file.dataset("quad/complex").read<h5pp::cx128>(), complexValue);
    require_equal(file.attribute("quad/scalar", "scalarAttr").read<h5pp::fp128>(), scalarValue);
    require_equal(file.attribute("quad/complex", "complexAttr").read<h5pp::cx128>(), complexValue);

    auto scalarTypeInfo = file.dataset("quad/scalar").getTypeInfo();
    REQUIRE(scalarTypeInfo.cppTypeBytes.has_value());
    REQUIRE(scalarTypeInfo.cppTypeBytes.value() == sizeof(h5pp::fp128));
    REQUIRE(scalarTypeInfo.cppTypeIndex.has_value());
    REQUIRE(scalarTypeInfo.cppTypeIndex.value() == std::type_index(typeid(h5pp::fp128)));
    REQUIRE(scalarTypeInfo.h5Type.has_value());
    REQUIRE(H5Tget_class(scalarTypeInfo.h5Type.value()) == H5T_FLOAT);

    auto complexTypeInfo = file.dataset("quad/complex").getTypeInfo();
    REQUIRE(complexTypeInfo.cppTypeBytes.has_value());
    REQUIRE(complexTypeInfo.cppTypeBytes.value() == sizeof(h5pp::cx128));
    REQUIRE(complexTypeInfo.h5Type.has_value());
    REQUIRE(H5Tget_class(complexTypeInfo.h5Type.value()) == H5T_COMPOUND);

    auto attrTypeInfo = file.attribute("quad/complex", "complexAttr").getTypeInfo();
    REQUIRE(attrTypeInfo.cppTypeBytes.has_value());
    REQUIRE(attrTypeInfo.cppTypeBytes.value() == sizeof(h5pp::cx128));
    REQUIRE(attrTypeInfo.h5Type.has_value());
    REQUIRE(H5Tget_class(attrTypeInfo.h5Type.value()) == H5T_COMPOUND);
}

TEST_CASE("float128 vectors preserve exact binary values", "[float128][vector]") {
    h5pp::File file(make_path("float128-vector"), h5pp::FileAccess::REPLACE, 0);

    std::vector<h5pp::fp128> scalarValues = {
        make_value(1.0L, 10.0L),
        make_value(-355.0L, 113.0L),
        make_value(1234567.0L, 8192.0L),
        make_value(-1.0L, 65536.0L),
    };
    std::vector<h5pp::cx128> complexValues = {
        make_complex(make_value(1.0L, 10.0L), make_value(-3.0L, 7.0L)),
        make_complex(make_value(22.0L, 7.0L), make_value(5.0L, 13.0L)),
        make_complex(make_value(-9.0L, 11.0L), make_value(1.0L, 65536.0L)),
    };

    file.dataset("quad/scalars").write(scalarValues);
    file.dataset("quad/complexes").write(complexValues);

    require_vector_equal(file.dataset("quad/scalars").read<std::vector<h5pp::fp128>>(), scalarValues);
    require_vector_equal(file.dataset("quad/complexes").read<std::vector<h5pp::cx128>>(), complexValues);

    auto scalarInfo = file.dataset("quad/scalars").getInfo();
    REQUIRE(scalarInfo.dsetDims.has_value());
    REQUIRE(scalarInfo.dsetDims.value() == std::vector<hsize_t>{scalarValues.size()});

    auto complexInfo = file.dataset("quad/complexes").getInfo();
    REQUIRE(complexInfo.dsetDims.has_value());
    REQUIRE(complexInfo.dsetDims.value() == std::vector<hsize_t>{complexValues.size()});
}

TEST_CASE("float128 compound records round-trip with explicit HDF5 types", "[float128][compound]") {
    h5pp::File file(make_path("float128-compound"), h5pp::FileAccess::REPLACE, 0);

    auto recordType = make_record_type();

    QuadRecord singleRecord {
        .scalar = make_value(22.0L, 7.0L),
        .complex = make_complex(make_value(-3.0L, 5.0L), make_value(7.0L, 9.0L)),
    };
    std::vector<QuadRecord> recordVector = {
        singleRecord,
        QuadRecord {
            .scalar = make_value(-355.0L, 113.0L),
            .complex = make_complex(make_value(1.0L, 3.0L), make_value(-2.0L, 5.0L)),
        },
        QuadRecord {
            .scalar = make_value(1234567.0L, 8192.0L),
            .complex = make_complex(make_value(-11.0L, 13.0L), make_value(17.0L, 19.0L)),
        },
    };

    h5pp::DatasetCreateOptions create;
    create.h5Type = recordType;
    file.dataset("quad/record").create(singleRecord, create).write(singleRecord);
    file.dataset("quad/records").create(recordVector, create).write(recordVector);

    h5pp::AttributeWriteOptions attrWrite;
    attrWrite.h5Type = recordType;
    file.attribute("quad/record", "recordAttr").write(singleRecord, attrWrite);

    h5pp::DatasetReadOptions read;
    read.h5Type = recordType;
    h5pp::AttributeReadOptions attrRead;
    attrRead.h5Type = recordType;

    require_equal(file.dataset("quad/record").read<QuadRecord>(read), singleRecord);
    require_vector_equal(file.dataset("quad/records").read<std::vector<QuadRecord>>(read), recordVector);
    require_equal(file.attribute("quad/record", "recordAttr").read<QuadRecord>(attrRead), singleRecord);

    auto info = file.dataset("quad/record").getInfo();
    REQUIRE(info.h5Type.has_value());
    REQUIRE(H5Tget_class(info.h5Type.value()) == H5T_COMPOUND);
}

#else

TEST_CASE("float128 support is unavailable in this build", "[float128]") { SUCCEED("H5PP_USE_QUADMATH and H5PP_USE_FLOAT128 are both disabled"); }

#endif

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    return session.run();
}
