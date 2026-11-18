#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <string>
#include <string_view>
#include <vector>

namespace {
    h5pp::fs::path make_path(std::string_view name) {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::fs::path(h5pp::format(H5PP_TEST_DIR "{}.h5", name));
    }

    h5pp::File make_file(std::string_view name) { return h5pp::File(make_path(name), h5pp::FileAccess::REPLACE, 0); }
}

TEST_CASE("copyFileTo preserves source contents and object attributes", "[copy-file][copy]") {
    auto source = make_file("copyFile-source");

    source.dataset("groupA/A").write(std::string("A"));
    source.dataset("groupA/numbers").write(std::vector<int>{1, 2, 3, 4});
    source.dataset("rootData").write(std::string("root"));
    source.attribute("groupA/A", "dataset_attr").write(std::string("dataset-attr"));
    source.attribute("groupA", "group_attr").write(std::string("group-attr"));

    auto sourcePath = source.getFilePath();
    auto targetPath = make_path("copyFile-copy");
    auto copiedPath = source.copyFileTo(targetPath, h5pp::FileAccess::REPLACE);

    REQUIRE(source.getFilePath() == sourcePath);
    REQUIRE(h5pp::fs::exists(copiedPath));
    REQUIRE(source.dataset("groupA/A").read<std::string>() == "A");

    h5pp::File copied(copiedPath, h5pp::FileAccess::READONLY, 0);
    REQUIRE(copied.dataset("groupA/A").read<std::string>() == "A");
    REQUIRE(copied.dataset("groupA/numbers").read<std::vector<int>>() == std::vector<int>{1, 2, 3, 4});
    REQUIRE(copied.dataset("rootData").read<std::string>() == "root");
    REQUIRE(copied.attribute("groupA/A", "dataset_attr").read<std::string>() == "dataset-attr");
    REQUIRE(copied.attribute("groupA", "group_attr").read<std::string>() == "group-attr");
}

TEST_CASE("copyFileTo with REPLACE overwrites the existing target file", "[copy-file][replace]") {
    auto source = make_file("copyFile-replace-source");
    auto target = make_file("copyFile-replace-target");

    source.dataset("groupA/A").write(std::string("A"));
    source.dataset("groupA/numbers").write(std::vector<int>{7, 8, 9});
    source.attribute("groupA/A", "dataset_attr").write(std::string("dataset-attr"));

    target.dataset("groupB/B").write(std::string("B"));
    target.attribute("groupB/B", "dataset_attr").write(std::string("old-attr"));

    auto replacedPath = source.copyFileTo(target.getFilePath(), h5pp::FileAccess::REPLACE);
    REQUIRE(h5pp::fs::exists(replacedPath));

    h5pp::File replaced(replacedPath, h5pp::FileAccess::READONLY, 0);
    REQUIRE(replaced.dataset("groupA/A").read<std::string>() == "A");
    REQUIRE(replaced.dataset("groupA/numbers").read<std::vector<int>>() == std::vector<int>{7, 8, 9});
    REQUIRE(replaced.attribute("groupA/A", "dataset_attr").read<std::string>() == "dataset-attr");
    REQUIRE_FALSE(replaced.link("groupB/B").exists());
}

TEST_CASE("moveFileTo updates file path, removes the source path and keeps the object usable", "[copy-file][move]") {
    auto file = make_file("copyFile-move-source");

    file.dataset("groupA/A").write(std::string("A"));
    file.attribute("groupA/A", "dataset_attr").write(std::string("before-move"));

    auto sourcePath = h5pp::fs::absolute(make_path("copyFile-move-source"));
    auto targetPath = make_path("copyFile-move-target");
    auto movedPath  = file.moveFileTo(targetPath, h5pp::FileAccess::REPLACE);

    REQUIRE(file.getFilePath() == movedPath.string());
    REQUIRE_FALSE(h5pp::fs::exists(sourcePath));
    REQUIRE(h5pp::fs::exists(movedPath));

    REQUIRE(file.dataset("groupA/A").read<std::string>() == "A");
    REQUIRE(file.attribute("groupA/A", "dataset_attr").read<std::string>() == "before-move");

    file.dataset("groupA/afterMove").write(std::string("written-after-move"));
    file.attribute("groupA/afterMove", "dataset_attr").write(std::string("post-move"));

    h5pp::File moved(movedPath, h5pp::FileAccess::READONLY, 0);
    REQUIRE(moved.dataset("groupA/A").read<std::string>() == "A");
    REQUIRE(moved.dataset("groupA/afterMove").read<std::string>() == "written-after-move");
    REQUIRE(moved.attribute("groupA/A", "dataset_attr").read<std::string>() == "before-move");
    REQUIRE(moved.attribute("groupA/afterMove", "dataset_attr").read<std::string>() == "post-move");
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
