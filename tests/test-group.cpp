#include <algorithm>
#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <string_view>
#include <vector>

namespace {
    h5pp::File make_file() {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::File(H5PP_TEST_DIR "group.h5", h5pp::FileAccess::REPLACE, 0);
    }

    void write_tree(h5pp::File &file) {
        file.writeDataset(0.0, "dsetA");
        file.writeDataset(0.1, "group1/dsetB2");
        file.writeDataset(0.1, "group1/dsetB1");
        file.writeDataset(0.1, "group1/dsetB0");
        file.writeDataset(0.2, "group1/group2/dsetC");
        file.writeDataset(0.2, "group3/group4/group5/dsetD");
        file.writeDataset(0.2, "group3/group4/group5/dsetE");
        file.createSoftLink("group1/group2/dsetC", "links/toC");
    }
}

TEST_CASE("group and link handles navigate and expose rooted queries", "[group][link][handles]") {
    auto file = make_file();
    write_tree(file);
    file.writeAttribute("group1", "label", std::string("g1"));
    file.writeAttribute("group1/group2/dsetC", "units", std::string("arb"));
    file.writeAttribute("group1/group2/dsetC", "description", std::string("dataset C"));

    auto group = file.group("group1");
    REQUIRE(group.exists());
    REQUIRE(group.getPath() == "group1");
    REQUIRE(group.dataset("dsetB1").read<double>() == Catch::Approx(0.1));
    REQUIRE(group.group("group2").dataset("dsetC").read<double>() == Catch::Approx(0.2));
    REQUIRE(group.attribute("label").read<std::string>() == "g1");
    REQUIRE(group.findAttributes("lab") == std::vector<std::string>{"label"});

    auto link = file.link("group1/group2/dsetC");
    REQUIRE(link.exists());
    REQUIRE(link.getPath() == "group1/group2/dsetC");
    REQUIRE(link.asDataset().read<double>() == Catch::Approx(0.2));
    auto attrNames = link.getAttributeNames();
    std::sort(attrNames.begin(), attrNames.end());
    REQUIRE(attrNames == std::vector<std::string>{"description", "units"});
    REQUIRE(link.findAttributes("unit") == std::vector<std::string>{"units"});
    REQUIRE(link.asDataset().findAttributes("desc") == std::vector<std::string>{"description"});
    REQUIRE_FALSE(link.asDataset().attribute("missing").exists());
}

TEST_CASE("group handles expose rooted existence info and link creation", "[group][rooted]") {
    auto file = make_file();
    write_tree(file);
    file.writeAttribute("group1/group2/dsetC", "units", std::string("arb"));

    auto root = file.group("group1");
    REQUIRE(root.linkExists("group2/dsetC"));
    REQUIRE(root.attributeExists("group2/dsetC", "units"));
    REQUIRE(root.getLinkInfo("group2/dsetC").linkPath.value() == "group1/group2/dsetC");
    REQUIRE(root.getDatasetInfo("group2/dsetC").dsetPath.value() == "group1/group2/dsetC");
    REQUIRE(root.getAttributeInfo("group2/dsetC", "units").attrName.value() == "units");
    REQUIRE(root.getAttributeNames("group2/dsetC") == std::vector<std::string>{"units"});

    root.createGroup("scratch/nested");
    REQUIRE(file.linkExists("group1/scratch/nested"));

    root.createSoftLink("group2/dsetC", "links/toC");
    REQUIRE(file.readDataset<double>("group1/links/toC") == Catch::Approx(0.2));

    root.deleteLink("links/toC");
    REQUIRE_FALSE(file.linkExists("group1/links/toC"));
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    session.configData().shouldDebugBreak = true;
    return session.run();
}
