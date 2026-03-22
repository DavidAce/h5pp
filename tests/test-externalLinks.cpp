#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <string_view>

namespace {
    h5pp::fs::path make_path(std::string_view name) { return h5pp::fs::path(h5pp::format("output/{}.h5", name)); }
}

TEST_CASE("External links resolve datasets from another file", "[external-link][dataset]") {
    h5pp::File file_a(make_path("externalLink-A"), h5pp::FileAccess::REPLACE, 0);
    file_a.writeDataset(42.0, "groupA/dsetA");
    file_a.writeAttribute(std::string("from-a"), "groupA/dsetA", "dataset_attr");

    h5pp::File file_b(make_path("externalLink-B"), h5pp::FileAccess::REPLACE, 0);
    file_b.createExternalLink("externalLink-A.h5", "groupA/dsetA", "links/dsetA");

    REQUIRE(file_b.linkExists("links/dsetA"));
    REQUIRE(file_b.readDataset<double>("links/dsetA") == Catch::Approx(42.0));
    REQUIRE(file_b.readAttribute<std::string>("links/dsetA", "dataset_attr") == "from-a");
}

TEST_CASE("External links can target groups through a path relative to the current file", "[external-link][group]") {
    auto source_dir = h5pp::fs::path("output/externalLinks");
    auto nested_dir = source_dir / "nested";

    h5pp::File file_a(source_dir / "source.h5", h5pp::FileAccess::REPLACE, 0);
    file_a.writeDataset(std::vector<int>{1, 2, 3}, "groupA/dsetA");
    file_a.writeDataset(std::string("text"), "groupA/text");
    file_a.writeAttribute(std::string("group-attr"), "groupA", "group_attr");

    h5pp::File file_b(nested_dir / "target.h5", h5pp::FileAccess::REPLACE, 0);
    file_b.createExternalLink("../source.h5", "groupA", "external/groupA");

    REQUIRE(file_b.readDataset<std::vector<int>>("external/groupA/dsetA") == std::vector<int>{1, 2, 3});
    REQUIRE(file_b.readDataset<std::string>("external/groupA/text") == "text");
    REQUIRE(file_b.readAttribute<std::string>("external/groupA", "group_attr") == "group-attr");
}

TEST_CASE("Creating an external link to a missing target does not eagerly fail, but dereferencing it does", "[external-link][errors]") {
    h5pp::File file(make_path("externalLink-missing"), h5pp::FileAccess::REPLACE, 0);
    file.createExternalLink("missing-target.h5", "group/dset", "dangling/link");

    REQUIRE(file.linkExists("dangling/link"));
    REQUIRE_THROWS(file.readDataset<double>("dangling/link"));
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;

    session.configData().shouldDebugBreak = true;
    return session.run();
}
