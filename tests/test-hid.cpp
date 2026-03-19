#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <h5pp/h5pp.h>

namespace {
    std::string make_file_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format("output/{}.h5", name);
    }
}

TEST_CASE("hid wrapper move constructor transfers ownership without increasing refcount", "[hid]") {
    auto path = make_file_path("test-hid-move-ctor");
    hid_t raw = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(raw > 0);

    h5pp::hid::h5f src(raw);
    REQUIRE(src.refcount() == 1);

    h5pp::hid::h5f dst(std::move(src));

    REQUIRE_FALSE(static_cast<bool>(src));
    REQUIRE(static_cast<bool>(dst));
    REQUIRE(dst.refcount() == 1);
}

TEST_CASE("hid wrapper move assignment transfers ownership without increasing refcount", "[hid]") {
    auto path = make_file_path("test-hid-move-assign");
    hid_t raw = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(raw > 0);

    h5pp::hid::h5f src(raw);
    h5pp::hid::h5f dst;

    dst = std::move(src);

    REQUIRE_FALSE(static_cast<bool>(src));
    REQUIRE(static_cast<bool>(dst));
    REQUIRE(dst.refcount() == 1);
}

TEST_CASE("hid wrapper destructor releases the owned identifier", "[hid]") {
    auto  path = make_file_path("test-hid-destructor-release");
    hid_t raw  = -1;

    REQUIRE_NOTHROW([&] {
        h5pp::hid::h5f file(H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT));
        REQUIRE(static_cast<bool>(file));
        raw = file.value();
        REQUIRE(H5Iis_valid(raw) > 0);
    }());

    REQUIRE(raw > 0);
    REQUIRE(H5Iis_valid(raw) == 0);
}

TEST_CASE("hid wrapper rejects wrong raw identifier types", "[hid]") {
    hid_t transient_type = H5Tcopy(H5T_NATIVE_INT);
    REQUIRE(transient_type > 0);

    REQUIRE_THROWS_AS(h5pp::hid::h5d(transient_type), std::runtime_error);
    REQUIRE(H5Tclose(transient_type) >= 0);
}

TEST_CASE("h5o accepts object ids and datatype ids", "[hid]") {
    auto path = make_file_path("test-hid-h5o-validation");
    hid_t raw = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(raw > 0);
    h5pp::hid::h5f file(raw);

    SECTION("group ids are accepted") {
        hid_t group = H5Gcreate(file, "/group", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        REQUIRE(group > 0);
        REQUIRE_NOTHROW(h5pp::hid::h5o(group));
    }

    SECTION("dataset ids are accepted") {
        hsize_t dims[1] = {1};
        hid_t   space   = H5Screate_simple(1, dims, nullptr);
        REQUIRE(space > 0);
        hid_t dataset = H5Dcreate(file, "/dataset", H5T_NATIVE_INT, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        REQUIRE(dataset > 0);
        H5Sclose(space);
        REQUIRE_NOTHROW(h5pp::hid::h5o(dataset));
    }

    SECTION("committed datatypes are accepted") {
        hid_t named_type = H5Tcopy(H5T_NATIVE_INT);
        REQUIRE(named_type > 0);
        REQUIRE(H5Tcommit2(file, "/named_type", named_type, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) >= 0);
        REQUIRE(H5Tcommitted(named_type) > 0);
        REQUIRE_NOTHROW(h5pp::hid::h5o(named_type));
    }

    SECTION("transient datatypes are accepted and closed") {
        hid_t transient_type = H5Tcopy(H5T_NATIVE_INT);
        REQUIRE(transient_type > 0);
        REQUIRE(H5Tcommitted(transient_type) == 0);
        {
            h5pp::hid::h5o object(transient_type);
            REQUIRE(static_cast<bool>(object));
            REQUIRE(H5Iis_valid(transient_type) > 0);
        }
        REQUIRE(H5Iis_valid(transient_type) == 0);
    }
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
