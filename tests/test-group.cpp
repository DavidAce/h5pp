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
        file.dataset("dsetA").write(0.0);
        file.dataset("group1/dsetB2").write(0.1);
        file.dataset("group1/dsetB1").write(0.1);
        file.dataset("group1/dsetB0").write(0.1);
        file.dataset("group1/group2/dsetC").write(0.2);
        file.dataset("group3/group4/group5/dsetD").write(0.2);
        file.dataset("group3/group4/group5/dsetE").write(0.2);
        file.link("group1/group2/dsetC").createSoftLink("links/toC");
    }
}

TEST_CASE("v2 groups and links navigate and query child paths", "[v2][group][link]") {
    auto file = make_file();
    write_tree(file);
    file.attribute("group1", "label").write(std::string("g1"));
    file.attribute("group1/group2/dsetC", "units").write(std::string("arb"));
    file.attribute("group1/group2/dsetC", "description").write(std::string("dataset C"));

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

TEST_CASE("v2 groups support child-path queries and link creation", "[v2][group]") {
    auto file = make_file();
    write_tree(file);
    file.attribute("group1/group2/dsetC", "units").write(std::string("arb"));

    auto root = file.group("group1");
    REQUIRE(root.linkExists("group2/dsetC"));
    REQUIRE(root.attributeExists("group2/dsetC", "units"));
    REQUIRE(root.getLinkInfo("group2/dsetC").linkPath.value() == "group1/group2/dsetC");
    REQUIRE(root.getDatasetInfo("group2/dsetC").dsetPath.value() == "group1/group2/dsetC");
    REQUIRE(root.getAttributeInfo("group2/dsetC", "units").attrName.value() == "units");
    REQUIRE(root.getAttributeNames("group2/dsetC") == std::vector<std::string>{"units"});

    root.createGroup("scratch/nested");
    REQUIRE(file.link("group1/scratch/nested").exists());

    root.createSoftLink("group2/dsetC", "links/toC");
    REQUIRE(file.dataset("group1/links/toC").read<double>() == Catch::Approx(0.2));

    root.deleteLink("links/toC");
    REQUIRE_FALSE(file.link("group1/links/toC").exists());
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    session.configData().shouldDebugBreak = true;
    return session.run();
}
