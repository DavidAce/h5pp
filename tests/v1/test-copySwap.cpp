#include <catch2/catch_all.hpp>
#include <h5pp/v1/h5pp.h>
#include <string>
#include <string_view>
#include <utility>

namespace {
    h5pp::fs::path make_path(std::string_view name) { return h5pp::fs::path(h5pp::format(H5PP_TEST_DIR "{}.h5", name)); }

    h5pp::v1::File make_file(std::string_view name) { return h5pp::v1::File(make_path(name), h5pp::FileAccess::REPLACE, 0); }
}

TEST_CASE("File copy operations preserve bindings, settings and usability", "[file-copy][copy]") {
    h5pp::v1::File file0;
    REQUIRE(file0.getFilePath().empty());
    REQUIRE(file0.getFileAccess() == h5pp::FileAccess::RENAME);

    auto file_a = make_file("copySwap-A");
    auto file_b = make_file("copySwap-B");

    file_a.setCompressionLevel(2);
    file_b.setCompressionLevel(5);
    file_a.writeDataset(std::string("A"), "groupA/A");
    file_b.writeDataset(std::string("B"), "groupB/B");
    file_b.setKeepFileOpened();

    h5pp::v1::File file_c;
    file_c = file_b;
    REQUIRE(file_c.getFilePath() == file_b.getFilePath());
    REQUIRE(file_c.getFileAccess() == file_b.getFileAccess());
    REQUIRE(file_c.getCompressionLevel() == file_b.getCompressionLevel());

    file_c.writeDataset(std::string("C"), "groupC/C");
    REQUIRE(file_b.readDataset<std::string>("groupB/B") == "B");
    REQUIRE(file_b.readDataset<std::string>("groupC/C") == "C");

    h5pp::v1::File file_d(file_c);
    REQUIRE(file_d.getFilePath() == file_b.getFilePath());
    REQUIRE(file_d.getFileAccess() == file_b.getFileAccess());
    REQUIRE(file_d.getCompressionLevel() == file_b.getCompressionLevel());

    file_d.writeDataset(std::string("D"), "groupD/D");
    REQUIRE(file_c.readDataset<std::string>("groupD/D") == "D");

    file_d = file_a;
    REQUIRE(file_d.getFilePath() == file_a.getFilePath());
    REQUIRE(file_d.getFileAccess() == file_a.getFileAccess());
    REQUIRE(file_d.getCompressionLevel() == file_a.getCompressionLevel());
    file_d.writeDataset(std::string("A2"), "groupA/afterAssign");
    REQUIRE(file_a.readDataset<std::string>("groupA/afterAssign") == "A2");

    file_b.setKeepFileClosed();
    file_c.setKeepFileClosed();

    h5pp::v1::File reopened_b(file_b.getFilePath(), h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopened_b.readDataset<std::string>("groupB/B") == "B");
    REQUIRE(reopened_b.readDataset<std::string>("groupC/C") == "C");
    REQUIRE(reopened_b.readDataset<std::string>("groupD/D") == "D");
}

TEST_CASE("File move operations rebind objects to the moved file identity", "[file-copy][move]") {
    h5pp::v1::File file_e(h5pp::v1::File(make_path("copySwap-E"), h5pp::FileAccess::REPLACE, 0));
    auto       path_e = file_e.getFilePath();
    file_e.setCompressionLevel(6);
    file_e.writeDataset(std::string("E"), "groupE/E");

    h5pp::v1::File file_f;
    file_f      = h5pp::v1::File(make_path("copySwap-F"), h5pp::FileAccess::REPLACE, 0);
    auto path_f = file_f.getFilePath();
    file_f.writeDataset(std::string("F"), "groupF/F");

    h5pp::v1::File file_g(std::move(file_e));
    REQUIRE(file_g.getFilePath() == path_e);
    REQUIRE(file_g.getCompressionLevel() == 6);
    file_g.writeDataset(std::string("G"), "groupG/G");

    file_f = std::move(file_g);
    REQUIRE(file_f.getFilePath() == path_e);
    REQUIRE(file_f.getCompressionLevel() == 6);
    file_f.writeDataset(std::string("after-move"), "groupMove/afterMove");

    h5pp::v1::File reopened_e(path_e, h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopened_e.readDataset<std::string>("groupE/E") == "E");
    REQUIRE(reopened_e.readDataset<std::string>("groupG/G") == "G");
    REQUIRE(reopened_e.readDataset<std::string>("groupMove/afterMove") == "after-move");

    h5pp::v1::File reopened_f(path_f, h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopened_f.readDataset<std::string>("groupF/F") == "F");
    REQUIRE_FALSE(reopened_f.linkExists("groupMove/afterMove"));
}

TEST_CASE("std::swap exchanges file bindings and subsequent writes land on the swapped files", "[file-copy][swap]") {
    auto left  = make_file("copySwap-left");
    auto right = make_file("copySwap-right");

    auto left_path  = left.getFilePath();
    auto right_path = right.getFilePath();

    left.setCompressionLevel(1);
    right.setCompressionLevel(8);
    left.writeDataset(std::string("left"), "left/data");
    right.writeDataset(std::string("right"), "right/data");

    std::swap(left, right);

    REQUIRE(left.getFilePath() == right_path);
    REQUIRE(right.getFilePath() == left_path);
    REQUIRE(left.getCompressionLevel() == 8);
    REQUIRE(right.getCompressionLevel() == 1);
    REQUIRE(left.readDataset<std::string>("right/data") == "right");
    REQUIRE(right.readDataset<std::string>("left/data") == "left");

    left.writeDataset(std::string("written-via-left"), "swap/left");
    right.writeDataset(std::string("written-via-right"), "swap/right");

    h5pp::v1::File reopened_left(left_path, h5pp::FileAccess::READONLY, 0);
    h5pp::v1::File reopened_right(right_path, h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopened_left.readDataset<std::string>("left/data") == "left");
    REQUIRE(reopened_left.readDataset<std::string>("swap/right") == "written-via-right");
    REQUIRE(reopened_right.readDataset<std::string>("right/data") == "right");
    REQUIRE(reopened_right.readDataset<std::string>("swap/left") == "written-via-left");
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
