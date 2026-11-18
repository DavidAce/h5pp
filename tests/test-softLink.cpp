#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <string>
#include <string_view>
#include <vector>

namespace {
    std::string make_path(std::string_view name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }

    void seed_file(const std::string &path) {
        h5pp::File file(path, h5pp::FileAccess::REPLACE, 0);
        file.dataset("realgroup1/realgroup2/dsetA").write(42.0);
        file.dataset("realgroup1/realgroup2/realgroup3/dsetB").write(43.0);
        file.attribute("realgroup1/realgroup2/dsetA", "note").write(std::string("through-soft-link"));
    }
}

TEST_CASE("Soft links expose target datasets and attributes", "[soft-link]") {
    auto path = make_path("softLink");
    seed_file(path);

    h5pp::File file(path, h5pp::FileAccess::READWRITE, 0);
    REQUIRE_NOTHROW(file.link("realgroup1/realgroup2").createSoftLink("softlinks/realgroup2"));
    REQUIRE_NOTHROW(file.link("realgroup1/realgroup2/realgroup3").createSoftLink("softlinks/realgroup3"));
    REQUIRE_NOTHROW(file.link("realgroup1/realgroup2/dsetA").createSoftLink("softlinks/dsetA"));

    REQUIRE(file.link("softlinks/realgroup2/dsetA").exists());
    REQUIRE(file.link("softlinks/realgroup3/dsetB").exists());
    REQUIRE(file.link("softlinks/dsetA").exists());
    REQUIRE(file.dataset("softlinks/realgroup2/dsetA").read<double>() == 42.0);
    REQUIRE(file.dataset("softlinks/realgroup3/dsetB").read<double>() == 43.0);
    REQUIRE(file.attribute("softlinks/dsetA", "note").read<std::string>() == "through-soft-link");

    auto link_info = file.link("softlinks/dsetA").getInfo();
    REQUIRE(link_info.h5LinkType.has_value());
    REQUIRE(link_info.h5LinkType.value() == H5L_TYPE_SOFT);
}

TEST_CASE("Writing through soft links mutates the target object graph", "[soft-link]") {
    auto path = make_path("softLink-write");
    seed_file(path);

    h5pp::File file(path, h5pp::FileAccess::READWRITE, 0);
    file.link("realgroup1/realgroup2").createSoftLink("softlinks/realgroup2");
    file.link("realgroup1/realgroup2/realgroup3").createSoftLink("softlinks/realgroup3");

    REQUIRE_NOTHROW(file.dataset("softlinks/realgroup2/dsetC").write(44.0));
    REQUIRE_NOTHROW(file.dataset("softlinks/realgroup3/dsetD").write(45.0));
    REQUIRE_NOTHROW(file.attribute("softlinks/realgroup2/dsetC", "values").write(std::vector<int>{1, 2, 3}));

    REQUIRE(file.link("softlinks/realgroup2/dsetC").exists());
    REQUIRE(file.link("softlinks/realgroup3/dsetD").exists());
    REQUIRE(file.link("realgroup1/realgroup2/dsetC").exists());
    REQUIRE(file.link("realgroup1/realgroup2/realgroup3/dsetD").exists());
    REQUIRE(file.dataset("realgroup1/realgroup2/dsetC").read<double>() == 44.0);
    REQUIRE(file.dataset("realgroup1/realgroup2/realgroup3/dsetD").read<double>() == 45.0);
    REQUIRE(file.attribute("realgroup1/realgroup2/dsetC", "values").read<std::vector<int>>() == std::vector<int>{1, 2, 3});
}

TEST_CASE("Deleting soft links leaves the underlying targets intact", "[soft-link]") {
    auto path = make_path("softLink-delete");
    seed_file(path);

    {
        h5pp::File file(path, h5pp::FileAccess::READWRITE, 0);
        file.link("realgroup1/realgroup2").createSoftLink("softlinks/realgroup2");
        file.link("realgroup1/realgroup2/realgroup3").createSoftLink("softlinks/realgroup3");

        REQUIRE(file.link("softlinks/realgroup3/dsetB").exists());
        REQUIRE_NOTHROW(file.link("softlinks/realgroup3").deleteLink());
        REQUIRE_FALSE(file.link("softlinks/realgroup3").exists());
        REQUIRE(file.link("realgroup1/realgroup2/realgroup3/dsetB").exists());
    }

    h5pp::File reopened(path, h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopened.dataset("realgroup1/realgroup2/dsetA").read<double>() == 42.0);
    REQUIRE(reopened.dataset("realgroup1/realgroup2/realgroup3/dsetB").read<double>() == 43.0);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    return session.run();
}
