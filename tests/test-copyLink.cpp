#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/h5pp.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    h5pp::fs::path make_path(std::string_view name) {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::fs::path(h5pp::format(H5PP_TEST_DIR "{}.h5", name));
    }

    h5pp::File make_file(std::string_view name) { return h5pp::File(make_path(name), h5pp::FileAccess::REPLACE, 0); }

    void write_source_tree(h5pp::File &file, std::string_view root) {
        auto root_path    = std::string(root);
        auto text_path    = h5pp::format("{}/A", root);
        auto numbers_path = h5pp::format("{}/numbers", root);
        auto complex_path = h5pp::format("{}/nested/complex", root);

        file.dataset(text_path).write(std::string("A"));
        file.dataset(numbers_path).write(std::vector<int>{1, 2, 3, 4});
        file.dataset(complex_path)
            .write(std::vector<std::complex<double>>{
                {1.0, 2.0},
                {3.0, 4.0}
        });

        file.attribute(root_path, "group_attr").write(std::string("group-attr"));
        file.attribute(text_path, "dataset_attr").write(std::string("text-attr"));
        file.attribute(numbers_path, "dataset_attr").write(std::string("numbers-attr"));
        file.attribute(complex_path, "dataset_attr").write(std::string("complex-attr"));
    }
}

TEST_CASE("link handles copy across files in both source-driven and destination-driven forms", "[copy-link][file]") {
    auto file_a = make_file("copyLink-A");
    write_source_tree(file_a, "groupA");

    auto file_b_path = make_path("copyLink-B");
    file_a.link("groupA/A").copyToFile(file_b_path, "groupA_from_file_A/A", h5pp::FileAccess::REPLACE);

    h5pp::File file_b(file_b_path, h5pp::FileAccess::READWRITE, 0);
    REQUIRE(file_b.dataset("groupA_from_file_A/A").read<std::string>() == "A");
    REQUIRE(file_b.attribute("groupA_from_file_A/A", "dataset_attr").read<std::string>() == "text-attr");

    file_b.dataset("groupB/B").write(std::string("B"));
    file_b.dataset("groupB/numbers").write(std::vector<int>{9, 8, 7});
    file_b.attribute("groupB", "group_attr").write(std::string("groupB-attr"));
    file_b.attribute("groupB/B", "dataset_attr").write(std::string("from-file-b"));
    file_b.attribute("groupB/numbers", "dataset_attr").write(std::string("numbers-from-file-b"));

    file_a.link("groupA_from_file_B/B").copyFromFile(file_b.getFilePath(), "groupB/B");
    file_a.link("groupA_from_file_B/numbers").copyFromFile(file_b.getFilePath(), "groupB/numbers");

    REQUIRE(file_a.dataset("groupA/A").read<std::string>() == "A");
    REQUIRE(file_a.attribute("groupA/A", "dataset_attr").read<std::string>() == "text-attr");
    REQUIRE(file_a.dataset("groupA_from_file_B/B").read<std::string>() == "B");
    REQUIRE(file_a.dataset("groupA_from_file_B/numbers").read<std::vector<int>>() == std::vector<int>{9, 8, 7});
    REQUIRE(file_a.attribute("groupA_from_file_B/B", "dataset_attr").read<std::string>() == "from-file-b");
    REQUIRE(file_a.attribute("groupA_from_file_B/numbers", "dataset_attr").read<std::string>() == "numbers-from-file-b");
}

TEST_CASE("copyLink location APIs support recursive group copies and relative source paths", "[copy-link][location]") {
    auto source = make_file("copyLink-location-source");
    auto target = make_file("copyLink-location-target");
    auto mirror = make_file("copyLink-location-mirror");

    write_source_tree(source, "sourceGroup");
    target.group("receiving").create();

    auto target_handle   = target.advanced().openFileHandle();
    auto receiving_group = h5pp::hdf5::openLink<h5pp::hid::h5g>(target_handle, "receiving");
    source.link("sourceGroup").copyToLocation(receiving_group, "copiedGroup");

    REQUIRE(target.dataset("receiving/copiedGroup/A").read<std::string>() == "A");
    REQUIRE(target.dataset("receiving/copiedGroup/numbers").read<std::vector<int>>() == std::vector<int>{1, 2, 3, 4});
    REQUIRE(target.dataset("receiving/copiedGroup/nested/complex").read<std::vector<std::complex<double>>>() ==
            std::vector<std::complex<double>>{
                {1.0, 2.0},
                {3.0, 4.0}
    });
    REQUIRE(target.attribute("receiving/copiedGroup", "group_attr").read<std::string>() == "group-attr");
    REQUIRE(target.attribute("receiving/copiedGroup/A", "dataset_attr").read<std::string>() == "text-attr");
    REQUIRE(target.attribute("receiving/copiedGroup/nested/complex", "dataset_attr").read<std::string>() == "complex-attr");

    auto nested_group = h5pp::hdf5::openLink<h5pp::hid::h5g>(target_handle, "receiving/copiedGroup/nested");
    mirror.link("imports/complexCopy").copyFromLocation(nested_group, "complex");

    REQUIRE(mirror.dataset("imports/complexCopy").read<std::vector<std::complex<double>>>() == std::vector<std::complex<double>>{
                                                                                                   {1.0, 2.0},
                                                                                                   {3.0, 4.0}
    });
    REQUIRE(mirror.attribute("imports/complexCopy", "dataset_attr").read<std::string>() == "complex-attr");
    REQUIRE(source.attribute("sourceGroup", "group_attr").read<std::string>() == "group-attr");
}

TEST_CASE("copyLink reports missing sources and target collisions", "[copy-link][errors]") {
    auto source = make_file("copyLink-errors-source");
    auto target = make_file("copyLink-errors-target");

    source.dataset("source/data").write(std::vector<int>{1, 2, 3});
    source.attribute("source/data", "dataset_attr").write(std::string("source-attr"));
    target.dataset("existing/data").write(std::vector<int>{9, 9, 9});
    target.attribute("existing/data", "dataset_attr").write(std::string("existing-attr"));

    REQUIRE_THROWS_AS(source.link("missing/data").copyToFile(target.getFilePath(), "imports/missing"), std::runtime_error);
    REQUIRE_THROWS_AS(source.link("source/data").copyToFile(target.getFilePath(), "existing/data"), std::runtime_error);
    REQUIRE_THROWS_AS(target.link("imports/no-file").copyFromFile(make_path("copyLink-errors-no-such-file"), "source/data"),
                      std::runtime_error);

    REQUIRE_FALSE(target.link("imports/missing").exists());
    REQUIRE_FALSE(target.link("imports/no-file").exists());
    REQUIRE(target.dataset("existing/data").read<std::vector<int>>() == std::vector<int>{9, 9, 9});
    REQUIRE(target.attribute("existing/data", "dataset_attr").read<std::string>() == "existing-attr");
}

TEST_CASE("link handles copy within the same file and preserve attributes", "[copy-link][same-file]") {
    auto file = make_file("copyLink-same-file");

    file.dataset("source/value").write(9);
    file.attribute("source/value", "kind").write(std::string("payload"));

    auto source = file.link("source/value");
    source.copyTo("copies/value", h5pp::FileAccess::REPLACE);

    REQUIRE(file.dataset("copies/value").read<int>() == 9);
    REQUIRE(file.attribute("copies/value", "kind").read<std::string>() == "payload");
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    return session.run();
}
