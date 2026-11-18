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
    fileA.dataset("groupA/A").write(std::string("A"));
    fileB.dataset("groupB/B").write(std::string("B"));
    fileB.setKeepFileOpened();

    h5pp::File fileC;
    fileC = fileB;
    REQUIRE(fileC.getFilePath() == fileB.getFilePath());
    REQUIRE(fileC.getFileAccess() == fileB.getFileAccess());
    REQUIRE(fileC.getCompressionLevel() == fileB.getCompressionLevel());

    fileC.dataset("groupC/C").write(std::string("C"));
    REQUIRE(fileB.dataset("groupB/B").read<std::string>() == "B");
    REQUIRE(fileB.dataset("groupC/C").read<std::string>() == "C");

    h5pp::File fileD(fileC);
    REQUIRE(fileD.getFilePath() == fileB.getFilePath());
    REQUIRE(fileD.getFileAccess() == fileB.getFileAccess());
    REQUIRE(fileD.getCompressionLevel() == fileB.getCompressionLevel());

    fileD.dataset("groupD/D").write(std::string("D"));
    REQUIRE(fileC.dataset("groupD/D").read<std::string>() == "D");

    fileD = fileA;
    REQUIRE(fileD.getFilePath() == fileA.getFilePath());
    REQUIRE(fileD.getFileAccess() == fileA.getFileAccess());
    REQUIRE(fileD.getCompressionLevel() == fileA.getCompressionLevel());
    fileD.dataset("groupA/afterAssign").write(std::string("A2"));
    REQUIRE(fileA.dataset("groupA/afterAssign").read<std::string>() == "A2");

    fileB.setKeepFileClosed();
    fileC.setKeepFileClosed();

    h5pp::File reopenedB(fileB.getFilePath(), h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopenedB.dataset("groupB/B").read<std::string>() == "B");
    REQUIRE(reopenedB.dataset("groupC/C").read<std::string>() == "C");
    REQUIRE(reopenedB.dataset("groupD/D").read<std::string>() == "D");
}

TEST_CASE("File move operations rebind objects to the moved file identity", "[file-copy-move-swap][move]") {
    h5pp::File fileE(h5pp::File(make_path("copySwap-E"), h5pp::FileAccess::REPLACE, 0));
    auto       pathE = fileE.getFilePath();
    fileE.setCompressionLevel(6);
    fileE.dataset("groupE/E").write(std::string("E"));

    h5pp::File fileF;
    fileF      = h5pp::File(make_path("copySwap-F"), h5pp::FileAccess::REPLACE, 0);
    auto pathF = fileF.getFilePath();
    fileF.dataset("groupF/F").write(std::string("F"));

    h5pp::File fileG(std::move(fileE));
    REQUIRE(fileG.getFilePath() == pathE);
    REQUIRE(fileG.getCompressionLevel() == 6);
    fileG.dataset("groupG/G").write(std::string("G"));

    fileF = std::move(fileG);
    REQUIRE(fileF.getFilePath() == pathE);
    REQUIRE(fileF.getCompressionLevel() == 6);
    fileF.dataset("groupMove/afterMove").write(std::string("after-move"));

    h5pp::File reopenedE(pathE, h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopenedE.dataset("groupE/E").read<std::string>() == "E");
    REQUIRE(reopenedE.dataset("groupG/G").read<std::string>() == "G");
    REQUIRE(reopenedE.dataset("groupMove/afterMove").read<std::string>() == "after-move");

    h5pp::File reopenedF(pathF, h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopenedF.dataset("groupF/F").read<std::string>() == "F");
    REQUIRE_FALSE(reopenedF.link("groupMove/afterMove").exists());
}

TEST_CASE("std::swap exchanges file bindings and subsequent writes land on the swapped files", "[file-copy-move-swap][swap]") {
    auto left  = make_file("copySwap-left");
    auto right = make_file("copySwap-right");

    auto leftPath  = left.getFilePath();
    auto rightPath = right.getFilePath();

    left.setCompressionLevel(1);
    right.setCompressionLevel(8);
    left.dataset("left/data").write(std::string("left"));
    right.dataset("right/data").write(std::string("right"));

    std::swap(left, right);

    REQUIRE(left.getFilePath() == rightPath);
    REQUIRE(right.getFilePath() == leftPath);
    REQUIRE(left.getCompressionLevel() == 8);
    REQUIRE(right.getCompressionLevel() == 1);
    REQUIRE(left.dataset("right/data").read<std::string>() == "right");
    REQUIRE(right.dataset("left/data").read<std::string>() == "left");

    left.dataset("swap/left").write(std::string("written-via-left"));
    right.dataset("swap/right").write(std::string("written-via-right"));

    h5pp::File reopenedLeft(leftPath, h5pp::FileAccess::READONLY, 0);
    h5pp::File reopenedRight(rightPath, h5pp::FileAccess::READONLY, 0);
    REQUIRE(reopenedLeft.dataset("left/data").read<std::string>() == "left");
    REQUIRE(reopenedLeft.dataset("swap/right").read<std::string>() == "written-via-right");
    REQUIRE(reopenedRight.dataset("right/data").read<std::string>() == "right");
    REQUIRE(reopenedRight.dataset("swap/left").read<std::string>() == "written-via-left");
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
