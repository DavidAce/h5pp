#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <string_view>
#include <vector>

namespace {
    h5pp::File make_file() { return h5pp::File("output/findObjects.h5", h5pp::FileAccess::REPLACE, 0); }

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

TEST_CASE("findDatasets locates datasets across roots, depths and hit limits", "[find][datasets]") {
    auto file = make_file();
    write_tree(file);

    REQUIRE(file.findDatasets("dsetB1") == std::vector<std::string>{"group1/dsetB1"});
    REQUIRE(file.findDatasets("dsetB").size() == 3);
    REQUIRE(file.findDatasets("dsetD", "group3/group4/").size() == 1);
    REQUIRE(file.findDatasets("dset", "group3/group4").size() == 2);
    REQUIRE(file.findDatasets("dset", "/group3/group4").size() == 2);

    REQUIRE(file.findDatasets("dset", "group3/group4/", 1, 0).empty());
    REQUIRE(file.findDatasets("dset", "group3/group4/", 1, 1) == std::vector<std::string>{"group5/dsetD"});
    REQUIRE(file.findDatasets("dset", "/group3/group4", 1, 1) == std::vector<std::string>{"group5/dsetD"});

    REQUIRE(file.findDatasets("", "/").size() == 7);
    REQUIRE(file.findDatasets("does-not-exist").empty());
}

TEST_CASE("findGroups and findLinks include groups and optionally symlinked datasets", "[find][groups]") {
    auto file = make_file();
    write_tree(file);

    REQUIRE(file.findGroups("group1") == std::vector<std::string>{"group1"});
    REQUIRE(file.findGroups("group5") == std::vector<std::string>{"group3/group4/group5"});
    REQUIRE(file.findGroups("group").size() == 5);
    REQUIRE(file.findGroups("", "/").size() == 6);

    auto without_symlinks = file.findLinks("dset", "/", -1, -1, false);
    auto with_symlinks    = file.findLinks("dset", "/", -1, -1, true);
    auto symlink_hits     = file.findLinks("toC", "/", -1, -1, true);
    REQUIRE(without_symlinks.size() == 7);
    REQUIRE(with_symlinks.size() >= without_symlinks.size());
    REQUIRE(std::find(without_symlinks.begin(), without_symlinks.end(), "links/toC") == without_symlinks.end());
    REQUIRE(symlink_hits == std::vector<std::string>{"links/toC"});
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
