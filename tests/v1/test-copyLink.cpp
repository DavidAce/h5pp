#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/v1/h5pp.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    h5pp::fs::path make_path(std::string_view name) { return h5pp::fs::path(h5pp::format(H5PP_TEST_DIR "{}.h5", name)); }

    h5pp::v1::File make_file(std::string_view name) { return h5pp::v1::File(make_path(name), h5pp::FileAccess::REPLACE, 0); }

    void write_source_tree(h5pp::v1::File &file, std::string_view root) {
        auto root_path    = std::string(root);
        auto text_path    = h5pp::format("{}/A", root);
        auto numbers_path = h5pp::format("{}/numbers", root);
        auto complex_path = h5pp::format("{}/nested/complex", root);

        file.writeDataset(std::string("A"), text_path);
        file.writeDataset(std::vector<int>{1, 2, 3, 4}, numbers_path);
        file.writeDataset(
            std::vector<std::complex<double>>{
                {1.0, 2.0},
                {3.0, 4.0}
        },
            complex_path);

        file.writeAttribute(std::string("group-attr"), root_path, "group_attr");
        file.writeAttribute(std::string("text-attr"), text_path, "dataset_attr");
        file.writeAttribute(std::string("numbers-attr"), numbers_path, "dataset_attr");
        file.writeAttribute(std::string("complex-attr"), complex_path, "dataset_attr");
    }
}

TEST_CASE("copyLinkToFile and copyLinkFromFile preserve the original cross-file workflow", "[copy-link][file]") {
    auto file_a = make_file("copyLink-A");
    write_source_tree(file_a, "groupA");

    auto file_b_path = make_path("copyLink-B");
    file_a.copyLinkToFile("groupA/A", file_b_path, "groupA_from_file_A/A", h5pp::FileAccess::REPLACE);

    h5pp::v1::File file_b(file_b_path, h5pp::FileAccess::READWRITE, 0);
    REQUIRE(file_b.readDataset<std::string>("groupA_from_file_A/A") == "A");
    REQUIRE(file_b.readAttribute<std::string>("groupA_from_file_A/A", "dataset_attr") == "text-attr");

    file_b.writeDataset(std::string("B"), "groupB/B");
    file_b.writeDataset(std::vector<int>{9, 8, 7}, "groupB/numbers");
    file_b.writeAttribute(std::string("groupB-attr"), "groupB", "group_attr");
    file_b.writeAttribute(std::string("from-file-b"), "groupB/B", "dataset_attr");
    file_b.writeAttribute(std::string("numbers-from-file-b"), "groupB/numbers", "dataset_attr");

    file_a.copyLinkFromFile("groupA_from_file_B/B", file_b.getFilePath(), "groupB/B");
    file_a.copyLinkFromFile("groupA_from_file_B/numbers", file_b.getFilePath(), "groupB/numbers");

    REQUIRE(file_a.readDataset<std::string>("groupA/A") == "A");
    REQUIRE(file_a.readAttribute<std::string>("groupA/A", "dataset_attr") == "text-attr");
    REQUIRE(file_a.readDataset<std::string>("groupA_from_file_B/B") == "B");
    REQUIRE(file_a.readDataset<std::vector<int>>("groupA_from_file_B/numbers") == std::vector<int>{9, 8, 7});
    REQUIRE(file_a.readAttribute<std::string>("groupA_from_file_B/B", "dataset_attr") == "from-file-b");
    REQUIRE(file_a.readAttribute<std::string>("groupA_from_file_B/numbers", "dataset_attr") == "numbers-from-file-b");
}

TEST_CASE("copyLink location APIs support recursive group copies and relative source paths", "[copy-link][location]") {
    auto source = make_file("copyLink-location-source");
    auto target = make_file("copyLink-location-target");
    auto mirror = make_file("copyLink-location-mirror");

    write_source_tree(source, "sourceGroup");
    target.createGroup("receiving");

    auto target_handle   = target.openFileHandle();
    auto receiving_group = h5pp::hdf5::openLink<h5pp::hid::h5g>(target_handle, "receiving");
    source.copyLinkToLocation("sourceGroup", receiving_group, "copiedGroup");

    REQUIRE(target.readDataset<std::string>("receiving/copiedGroup/A") == "A");
    REQUIRE(target.readDataset<std::vector<int>>("receiving/copiedGroup/numbers") == std::vector<int>{1, 2, 3, 4});
    REQUIRE(target.readDataset<std::vector<std::complex<double>>>("receiving/copiedGroup/nested/complex") ==
            std::vector<std::complex<double>>{
                {1.0, 2.0},
                {3.0, 4.0}
    });
    REQUIRE(target.readAttribute<std::string>("receiving/copiedGroup", "group_attr") == "group-attr");
    REQUIRE(target.readAttribute<std::string>("receiving/copiedGroup/A", "dataset_attr") == "text-attr");
    REQUIRE(target.readAttribute<std::string>("receiving/copiedGroup/nested/complex", "dataset_attr") == "complex-attr");

    auto nested_group = h5pp::hdf5::openLink<h5pp::hid::h5g>(target_handle, "receiving/copiedGroup/nested");
    mirror.copyLinkFromLocation("imports/complexCopy", nested_group, "complex");

    REQUIRE(mirror.readDataset<std::vector<std::complex<double>>>("imports/complexCopy") == std::vector<std::complex<double>>{
                                                                                                {1.0, 2.0},
                                                                                                {3.0, 4.0}
    });
    REQUIRE(mirror.readAttribute<std::string>("imports/complexCopy", "dataset_attr") == "complex-attr");
    REQUIRE(source.readAttribute<std::string>("sourceGroup", "group_attr") == "group-attr");
}

TEST_CASE("copyLink reports missing sources and target collisions", "[copy-link][errors]") {
    auto source = make_file("copyLink-errors-source");
    auto target = make_file("copyLink-errors-target");

    source.writeDataset(std::vector<int>{1, 2, 3}, "source/data");
    source.writeAttribute(std::string("source-attr"), "source/data", "dataset_attr");
    target.writeDataset(std::vector<int>{9, 9, 9}, "existing/data");
    target.writeAttribute(std::string("existing-attr"), "existing/data", "dataset_attr");

    REQUIRE_THROWS_AS(source.copyLinkToFile("missing/data", target.getFilePath(), "imports/missing"), std::runtime_error);
    REQUIRE_THROWS_AS(source.copyLinkToFile("source/data", target.getFilePath(), "existing/data"), std::runtime_error);
    REQUIRE_THROWS_AS(target.copyLinkFromFile("imports/no-file", make_path("copyLink-errors-no-such-file"), "source/data"),
                      std::runtime_error);

    REQUIRE_FALSE(target.linkExists("imports/missing"));
    REQUIRE_FALSE(target.linkExists("imports/no-file"));
    REQUIRE(target.readDataset<std::vector<int>>("existing/data") == std::vector<int>{9, 9, 9});
    REQUIRE(target.readAttribute<std::string>("existing/data", "dataset_attr") == "existing-attr");
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
