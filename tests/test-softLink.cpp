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
        file.writeDataset(42.0, "realgroup1/realgroup2/dsetA");
        file.writeDataset(43.0, "realgroup1/realgroup2/realgroup3/dsetB");
        file.writeAttribute("realgroup1/realgroup2/dsetA", "note", std::string("through-soft-link"));
    }
}

TEST_CASE("Soft links expose target datasets and attributes", "[soft-link]") {
    auto path = make_path("softLink");
    seed_file(path);

    h5pp::File file(path, h5pp::FileAccess::READWRITE, 0);
    REQUIRE_NOTHROW(file.link("realgroup1/realgroup2").createSoftLink("softlinks/realgroup2"));
    REQUIRE_NOTHROW(file.link("realgroup1/realgroup2/realgroup3").createSoftLink("softlinks/realgroup3"));
    REQUIRE_NOTHROW(file.link("realgroup1/realgroup2/dsetA").createSoftLink("softlinks/dsetA"));

    REQUIRE(file.linkExists("softlinks/realgroup2/dsetA"));
    REQUIRE(file.linkExists("softlinks/realgroup3/dsetB"));
    REQUIRE(file.linkExists("softlinks/dsetA"));
    REQUIRE(file.readDataset<double>("softlinks/realgroup2/dsetA") == 42.0);
    REQUIRE(file.readDataset<double>("softlinks/realgroup3/dsetB") == 43.0);
    REQUIRE(file.readAttribute<std::string>("softlinks/dsetA", "note") == "through-soft-link");

    auto link_info = file.getLinkInfo("softlinks/dsetA");
    REQUIRE(link_info.h5LinkType.has_value());
    REQUIRE(link_info.h5LinkType.value() == H5L_TYPE_SOFT);
}

TEST_CASE("Writing through soft links mutates the target object graph", "[soft-link]") {
    auto path = make_path("softLink-write");
    seed_file(path);

    h5pp::File file(path, h5pp::FileAccess::READWRITE, 0);
    file.link("realgroup1/realgroup2").createSoftLink("softlinks/realgroup2");
    file.link("realgroup1/realgroup2/realgroup3").createSoftLink("softlinks/realgroup3");

    REQUIRE_NOTHROW(file.writeDataset(44.0, "softlinks/realgroup2/dsetC"));
    REQUIRE_NOTHROW(file.writeDataset(45.0, "softlinks/realgroup3/dsetD"));
    REQUIRE_NOTHROW(file.writeAttribute("softlinks/realgroup2/dsetC", "values", std::vector<int>{1, 2, 3}));

    REQUIRE(file.linkExists("softlinks/realgroup2/dsetC"));
    REQUIRE(file.linkExists("softlinks/realgroup3/dsetD"));
    REQUIRE(file.linkExists("realgroup1/realgroup2/dsetC"));
    REQUIRE(file.linkExists("realgroup1/realgroup2/realgroup3/dsetD"));
    REQUIRE(file.readDataset<double>("realgroup1/realgroup2/dsetC") == 44.0);
    REQUIRE(file.readDataset<double>("realgroup1/realgroup2/realgroup3/dsetD") == 45.0);
    REQUIRE(file.readAttribute<std::vector<int>>("realgroup1/realgroup2/dsetC", "values") == std::vector<int>{1, 2, 3});
}

TEST_CASE("Deleting soft links leaves the underlying targets intact", "[soft-link]") {
    auto path = make_path("softLink-delete");
    seed_file(path);

    {
        h5pp::File file(path, h5pp::FileAccess::READWRITE, 0);
        file.link("realgroup1/realgroup2").createSoftLink("softlinks/realgroup2");
        file.link("realgroup1/realgroup2/realgroup3").createSoftLink("softlinks/realgroup3");

        REQUIRE(file.linkExists("softlinks/realgroup3/dsetB"));
        REQUIRE_NOTHROW(file.link("softlinks/realgroup3").deleteLink());
        REQUIRE_FALSE(file.linkExists("softlinks/realgroup3"));
        REQUIRE(file.linkExists("realgroup1/realgroup2/realgroup3/dsetB"));
    }

    h5pp::File reopened(path, h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopened.readDataset<double>("realgroup1/realgroup2/dsetA") == 42.0);
    REQUIRE(reopened.readDataset<double>("realgroup1/realgroup2/realgroup3/dsetB") == 43.0);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    return session.run();
}
