#include <catch2/catch_all.hpp>
#include <h5pp/v1/h5pp.h>
#include <string>
#include <string_view>
#include <vector>

namespace {
    h5pp::fs::path make_path(std::string_view name) { return h5pp::fs::path(h5pp::format(H5PP_TEST_DIR "{}.h5", name)); }

    h5pp::v1::File make_file(std::string_view name) { return h5pp::v1::File(make_path(name), h5pp::FileAccess::REPLACE, 0); }
}

TEST_CASE("copyFileTo preserves source contents and object attributes", "[copy-file][copy]") {
    auto source = make_file("copyFile-source");

    source.writeDataset(std::string("A"), "groupA/A");
    source.writeDataset(std::vector<int>{1, 2, 3, 4}, "groupA/numbers");
    source.writeDataset(std::string("root"), "rootData");
    source.writeAttribute(std::string("dataset-attr"), "groupA/A", "dataset_attr");
    source.writeAttribute(std::string("group-attr"), "groupA", "group_attr");

    auto source_path = source.getFilePath();
    auto target_path = make_path("copyFile-copy");
    auto copied_path = source.copyFileTo(target_path, h5pp::FileAccess::REPLACE);

    REQUIRE(source.getFilePath() == source_path);
    REQUIRE(h5pp::fs::exists(copied_path));
    REQUIRE(source.readDataset<std::string>("groupA/A") == "A");

    h5pp::v1::File copied(copied_path, h5pp::FileAccess::READONLY, 0);
    REQUIRE(copied.readDataset<std::string>("groupA/A") == "A");
    REQUIRE(copied.readDataset<std::vector<int>>("groupA/numbers") == std::vector<int>{1, 2, 3, 4});
    REQUIRE(copied.readDataset<std::string>("rootData") == "root");
    REQUIRE(copied.readAttribute<std::string>("groupA/A", "dataset_attr") == "dataset-attr");
    REQUIRE(copied.readAttribute<std::string>("groupA", "group_attr") == "group-attr");
}

TEST_CASE("copyFileTo with REPLACE overwrites the existing target file", "[copy-file][replace]") {
    auto source = make_file("copyFile-replace-source");
    auto target = make_file("copyFile-replace-target");

    source.writeDataset(std::string("A"), "groupA/A");
    source.writeDataset(std::vector<int>{7, 8, 9}, "groupA/numbers");
    source.writeAttribute(std::string("dataset-attr"), "groupA/A", "dataset_attr");

    target.writeDataset(std::string("B"), "groupB/B");
    target.writeAttribute(std::string("old-attr"), "groupB/B", "dataset_attr");

    auto replaced_path = source.copyFileTo(target.getFilePath(), h5pp::FileAccess::REPLACE);
    REQUIRE(h5pp::fs::exists(replaced_path));

    h5pp::v1::File replaced(replaced_path, h5pp::FileAccess::READONLY, 0);
    REQUIRE(replaced.readDataset<std::string>("groupA/A") == "A");
    REQUIRE(replaced.readDataset<std::vector<int>>("groupA/numbers") == std::vector<int>{7, 8, 9});
    REQUIRE(replaced.readAttribute<std::string>("groupA/A", "dataset_attr") == "dataset-attr");
    REQUIRE_FALSE(replaced.linkExists("groupB/B"));
}

TEST_CASE("moveFileTo updates file path, removes the source path and keeps the object usable", "[copy-file][move]") {
    auto file = make_file("copyFile-move-source");

    file.writeDataset(std::string("A"), "groupA/A");
    file.writeAttribute(std::string("before-move"), "groupA/A", "dataset_attr");

    auto source_path = h5pp::fs::absolute(make_path("copyFile-move-source"));
    auto target_path = make_path("copyFile-move-target");
    auto moved_path  = file.moveFileTo(target_path, h5pp::FileAccess::REPLACE);

    REQUIRE(file.getFilePath() == moved_path.string());
    REQUIRE_FALSE(h5pp::fs::exists(source_path));
    REQUIRE(h5pp::fs::exists(moved_path));

    REQUIRE(file.readDataset<std::string>("groupA/A") == "A");
    REQUIRE(file.readAttribute<std::string>("groupA/A", "dataset_attr") == "before-move");

    file.writeDataset(std::string("written-after-move"), "groupA/afterMove");
    file.writeAttribute(std::string("post-move"), "groupA/afterMove", "dataset_attr");

    h5pp::v1::File moved(moved_path, h5pp::FileAccess::READONLY, 0);
    REQUIRE(moved.readDataset<std::string>("groupA/A") == "A");
    REQUIRE(moved.readDataset<std::string>("groupA/afterMove") == "written-after-move");
    REQUIRE(moved.readAttribute<std::string>("groupA/A", "dataset_attr") == "before-move");
    REQUIRE(moved.readAttribute<std::string>("groupA/afterMove", "dataset_attr") == "post-move");
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
