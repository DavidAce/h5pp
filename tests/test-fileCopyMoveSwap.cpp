#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <string>
#include <string_view>
#include <utility>

namespace {
    h5pp::fs::path make_path(std::string_view name) {
        h5pp::fs::create_directories("output");
        return h5pp::fs::path(h5pp::format(H5PP_TEST_DIR "{}.h5", name));
    }

    h5pp::File make_file(std::string_view name) { return h5pp::File(make_path(name), h5pp::FileAccess::REPLACE, 0); }
}

TEST_CASE("File copy operations preserve bindings, settings and usability", "[file-copy-move-swap][copy]") {
    h5pp::File file0;
    REQUIRE(file0.getFilePath().empty());
    REQUIRE(file0.getFileAccess() == h5pp::FileAccess::RENAME);

    auto fileA = make_file("copySwap-A");
    auto fileB = make_file("copySwap-B");

    fileA.setCompressionLevel(2);
    fileB.setCompressionLevel(5);
    fileA.writeDataset(std::string("A"), "groupA/A");
    fileB.writeDataset(std::string("B"), "groupB/B");
    fileB.setKeepFileOpened();

    h5pp::File fileC;
    fileC = fileB;
    REQUIRE(fileC.getFilePath() == fileB.getFilePath());
    REQUIRE(fileC.getFileAccess() == fileB.getFileAccess());
    REQUIRE(fileC.getCompressionLevel() == fileB.getCompressionLevel());

    fileC.writeDataset(std::string("C"), "groupC/C");
    REQUIRE(fileB.readDataset<std::string>("groupB/B") == "B");
    REQUIRE(fileB.readDataset<std::string>("groupC/C") == "C");

    h5pp::File fileD(fileC);
    REQUIRE(fileD.getFilePath() == fileB.getFilePath());
    REQUIRE(fileD.getFileAccess() == fileB.getFileAccess());
    REQUIRE(fileD.getCompressionLevel() == fileB.getCompressionLevel());

    fileD.writeDataset(std::string("D"), "groupD/D");
    REQUIRE(fileC.readDataset<std::string>("groupD/D") == "D");

    fileD = fileA;
    REQUIRE(fileD.getFilePath() == fileA.getFilePath());
    REQUIRE(fileD.getFileAccess() == fileA.getFileAccess());
    REQUIRE(fileD.getCompressionLevel() == fileA.getCompressionLevel());
    fileD.writeDataset(std::string("A2"), "groupA/afterAssign");
    REQUIRE(fileA.readDataset<std::string>("groupA/afterAssign") == "A2");

    fileB.setKeepFileClosed();
    fileC.setKeepFileClosed();

    h5pp::File reopenedB(fileB.getFilePath(), h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopenedB.readDataset<std::string>("groupB/B") == "B");
    REQUIRE(reopenedB.readDataset<std::string>("groupC/C") == "C");
    REQUIRE(reopenedB.readDataset<std::string>("groupD/D") == "D");
}

TEST_CASE("File move operations rebind objects to the moved file identity", "[file-copy-move-swap][move]") {
    h5pp::File fileE(h5pp::File(make_path("copySwap-E"), h5pp::FileAccess::REPLACE, 0));
    auto       pathE = fileE.getFilePath();
    fileE.setCompressionLevel(6);
    fileE.writeDataset(std::string("E"), "groupE/E");

    h5pp::File fileF;
    fileF      = h5pp::File(make_path("copySwap-F"), h5pp::FileAccess::REPLACE, 0);
    auto pathF = fileF.getFilePath();
    fileF.writeDataset(std::string("F"), "groupF/F");

    h5pp::File fileG(std::move(fileE));
    REQUIRE(fileG.getFilePath() == pathE);
    REQUIRE(fileG.getCompressionLevel() == 6);
    fileG.writeDataset(std::string("G"), "groupG/G");

    fileF = std::move(fileG);
    REQUIRE(fileF.getFilePath() == pathE);
    REQUIRE(fileF.getCompressionLevel() == 6);
    fileF.writeDataset(std::string("after-move"), "groupMove/afterMove");

    h5pp::File reopenedE(pathE, h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopenedE.readDataset<std::string>("groupE/E") == "E");
    REQUIRE(reopenedE.readDataset<std::string>("groupG/G") == "G");
    REQUIRE(reopenedE.readDataset<std::string>("groupMove/afterMove") == "after-move");

    h5pp::File reopenedF(pathF, h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopenedF.readDataset<std::string>("groupF/F") == "F");
    REQUIRE_FALSE(reopenedF.linkExists("groupMove/afterMove"));
}

TEST_CASE("std::swap exchanges file bindings and subsequent writes land on the swapped files", "[file-copy-move-swap][swap]") {
    auto left  = make_file("copySwap-left");
    auto right = make_file("copySwap-right");

    auto leftPath  = left.getFilePath();
    auto rightPath = right.getFilePath();

    left.setCompressionLevel(1);
    right.setCompressionLevel(8);
    left.writeDataset(std::string("left"), "left/data");
    right.writeDataset(std::string("right"), "right/data");

    std::swap(left, right);

    REQUIRE(left.getFilePath() == rightPath);
    REQUIRE(right.getFilePath() == leftPath);
    REQUIRE(left.getCompressionLevel() == 8);
    REQUIRE(right.getCompressionLevel() == 1);
    REQUIRE(left.readDataset<std::string>("right/data") == "right");
    REQUIRE(right.readDataset<std::string>("left/data") == "left");

    left.writeDataset(std::string("written-via-left"), "swap/left");
    right.writeDataset(std::string("written-via-right"), "swap/right");

    h5pp::File reopenedLeft(leftPath, h5pp::FileAccess::READONLY, 0);
    h5pp::File reopenedRight(rightPath, h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopenedLeft.readDataset<std::string>("left/data") == "left");
    REQUIRE(reopenedLeft.readDataset<std::string>("swap/right") == "written-via-right");
    REQUIRE(reopenedRight.readDataset<std::string>("right/data") == "right");
    REQUIRE(reopenedRight.readDataset<std::string>("swap/left") == "written-via-left");
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
