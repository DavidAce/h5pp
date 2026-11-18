#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }
}

TEST_CASE("link handles move across files in both source-driven and destination-driven forms", "[move-link][file]") {
    auto file_a_path = make_path("moveLinkA");
    auto file_b_path = make_path("moveLinkB");

    h5pp::File file_a(file_a_path, h5pp::FileAccess::REPLACE, 0);
    file_a.dataset("groupA/A").write(std::string("A"));
    file_a.dataset("groupA/numbers").write(std::vector<int>{1, 2, 3, 4});

    SECTION("same-file moves rename the link and keep the payload intact") {
        file_a.link("groupA/A").moveToFile(file_a_path, "groupA_from_file_A/A");
        REQUIRE_FALSE(file_a.link("groupA/A").exists());
        REQUIRE(file_a.link("groupA_from_file_A/A").exists());
        REQUIRE(file_a.dataset("groupA_from_file_A/A").read<std::string>() == "A");

        file_a.link("groupA_from_file_A/A").moveToFile(file_a_path, "groupA/A");
        REQUIRE(file_a.link("groupA/A").exists());
        REQUIRE_FALSE(file_a.link("groupA_from_file_A/A").exists());
        REQUIRE(file_a.dataset("groupA/A").read<std::string>() == "A");
    }

    SECTION("cross-file moves transfer ownership between files") {
        file_a.link("groupA/A").moveToFile(file_b_path, "groupA_from_file_A/A", h5pp::FileAccess::REPLACE);
        REQUIRE_FALSE(file_a.link("groupA/A").exists());

        h5pp::File file_b(file_b_path, h5pp::FileAccess::READWRITE, 0);
        REQUIRE(file_b.link("groupA_from_file_A/A").exists());
        REQUIRE(file_b.dataset("groupA_from_file_A/A").read<std::string>() == "A");

        file_a.link("groupA_from_file_B/A").moveFromFile(file_b_path, "groupA_from_file_A/A");
        REQUIRE(file_a.link("groupA_from_file_B/A").exists());
        REQUIRE(file_a.dataset("groupA_from_file_B/A").read<std::string>() == "A");
        REQUIRE_FALSE(file_b.link("groupA_from_file_A/A").exists());
    }
}

TEST_CASE("Nested group moves carry datasets and attributes across files", "[move-link][group]") {
    auto source_path = make_path("moveLink-group-source");
    auto target_path = make_path("moveLink-group-target");

    h5pp::File source(source_path, h5pp::FileAccess::REPLACE, 0);
    source.dataset("groupA/subgroup/data").write(std::vector<double>{1.0, 2.0, 3.0});
    source.attribute("groupA/subgroup/data", "kind").write(std::string("payload"));
    source.dataset("groupA/subgroup/extra").write(std::vector<int>{7, 8});

    source.link("groupA/subgroup").moveToFile(target_path, "imports/subgroup", h5pp::FileAccess::REPLACE);
    REQUIRE_FALSE(source.link("groupA/subgroup").exists());

    h5pp::File target(target_path, h5pp::FileAccess::READWRITE, 0);
    REQUIRE(target.link("imports/subgroup/data").exists());
    REQUIRE(target.link("imports/subgroup/extra").exists());
    REQUIRE(target.dataset("imports/subgroup/data").read<std::vector<double>>() == std::vector<double>{1.0, 2.0, 3.0});
    REQUIRE(target.attribute("imports/subgroup/data", "kind").read<std::string>() == "payload");
}

TEST_CASE("Moving a missing link fails instead of silently creating a target", "[move-link][errors]") {
    auto source_path = make_path("moveLink-errors-source");
    auto target_path = make_path("moveLink-errors-target");

    h5pp::File source(source_path, h5pp::FileAccess::REPLACE, 0);
    source.dataset("source/data").write(1.0);

    REQUIRE_THROWS_AS(source.link("source/missing").moveToFile(target_path, "imports/missing", h5pp::FileAccess::REPLACE),
                      std::runtime_error);
    REQUIRE_THROWS_AS(source.link("imports/no-file").moveFromFile(make_path("moveLink-errors-no-such-file"), "source/data"),
                      std::runtime_error);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    session.configData().shouldDebugBreak = true;
    return session.run();
}
