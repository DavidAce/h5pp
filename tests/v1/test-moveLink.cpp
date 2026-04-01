#include <catch2/catch_all.hpp>
#include <h5pp/v1/h5pp.h>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }
}

TEST_CASE("moveLinkToFile and moveLinkFromFile preserve data while removing the original link", "[move-link][file]") {
    auto file_a_path = make_path("moveLinkA");
    auto file_b_path = make_path("moveLinkB");

    h5pp::v1::File file_a(file_a_path, h5pp::FileAccess::REPLACE, 0);
    file_a.writeDataset(std::string("A"), "groupA/A");
    file_a.writeDataset(std::vector<int>{1, 2, 3, 4}, "groupA/numbers");

    SECTION("same-file moves rename the link and keep the payload intact") {
        file_a.moveLinkToFile("groupA/A", file_a_path, "groupA_from_file_A/A");
        REQUIRE_FALSE(file_a.linkExists("groupA/A"));
        REQUIRE(file_a.linkExists("groupA_from_file_A/A"));
        REQUIRE(file_a.readDataset<std::string>("groupA_from_file_A/A") == "A");

        file_a.moveLinkToFile("groupA_from_file_A/A", file_a_path, "groupA/A");
        REQUIRE(file_a.linkExists("groupA/A"));
        REQUIRE_FALSE(file_a.linkExists("groupA_from_file_A/A"));
        REQUIRE(file_a.readDataset<std::string>("groupA/A") == "A");
    }

    SECTION("cross-file moves transfer ownership between files") {
        file_a.moveLinkToFile("groupA/A", file_b_path, "groupA_from_file_A/A", h5pp::FileAccess::REPLACE);
        REQUIRE_FALSE(file_a.linkExists("groupA/A"));

        h5pp::v1::File file_b(file_b_path, h5pp::FileAccess::READWRITE, 0);
        REQUIRE(file_b.linkExists("groupA_from_file_A/A"));
        REQUIRE(file_b.readDataset<std::string>("groupA_from_file_A/A") == "A");

        file_a.moveLinkFromFile("groupA_from_file_B/A", file_b_path, "groupA_from_file_A/A");
        REQUIRE(file_a.linkExists("groupA_from_file_B/A"));
        REQUIRE(file_a.readDataset<std::string>("groupA_from_file_B/A") == "A");
        REQUIRE_FALSE(file_b.linkExists("groupA_from_file_A/A"));
    }
}

TEST_CASE("Nested group moves carry datasets and attributes across files", "[move-link][group]") {
    auto source_path = make_path("moveLink-group-source");
    auto target_path = make_path("moveLink-group-target");

    h5pp::v1::File source(source_path, h5pp::FileAccess::REPLACE, 0);
    source.writeDataset(std::vector<double>{1.0, 2.0, 3.0}, "groupA/subgroup/data");
    source.writeAttribute(std::string("payload"), "groupA/subgroup/data", "kind");
    source.writeDataset(std::vector<int>{7, 8}, "groupA/subgroup/extra");

    source.moveLinkToFile("groupA/subgroup", target_path, "imports/subgroup", h5pp::FileAccess::REPLACE);
    REQUIRE_FALSE(source.linkExists("groupA/subgroup"));

    h5pp::v1::File target(target_path, h5pp::FileAccess::READWRITE, 0);
    REQUIRE(target.linkExists("imports/subgroup/data"));
    REQUIRE(target.linkExists("imports/subgroup/extra"));
    REQUIRE(target.readDataset<std::vector<double>>("imports/subgroup/data") == std::vector<double>{1.0, 2.0, 3.0});
    REQUIRE(target.readAttribute<std::string>("imports/subgroup/data", "kind") == "payload");
}

TEST_CASE("Moving a missing link fails instead of silently creating a target", "[move-link][errors]") {
    auto source_path = make_path("moveLink-errors-source");
    auto target_path = make_path("moveLink-errors-target");

    h5pp::v1::File source(source_path, h5pp::FileAccess::REPLACE, 0);
    source.writeDataset(1.0, "source/data");

    REQUIRE_THROWS_AS(source.moveLinkToFile("source/missing", target_path, "imports/missing", h5pp::FileAccess::REPLACE),
                      std::runtime_error);
    REQUIRE_THROWS_AS(source.moveLinkFromFile("imports/no-file", make_path("moveLink-errors-no-such-file"), "source/data"),
                      std::runtime_error);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    session.configData().shouldDebugBreak = true;
    return session.run();
}
