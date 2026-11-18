#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <string_view>

namespace {
    h5pp::fs::path make_path(std::string_view name) { return h5pp::fs::path(h5pp::format(H5PP_TEST_DIR "{}.h5", name)); }
}

TEST_CASE("External links resolve datasets from another file", "[external-link][dataset]") {
    h5pp::File file_a(make_path("externalLink-A"), h5pp::FileAccess::REPLACE, 0);
    file_a.dataset("groupA/dsetA").write(42.0);
    file_a.attribute("groupA/dsetA", "dataset_attr").write(std::string("from-a"));

    h5pp::File file_b(make_path("externalLink-B"), h5pp::FileAccess::REPLACE, 0);
    file_b.link("links/dsetA").createExternalLink("externalLink-A.h5", "groupA/dsetA");

    REQUIRE(file_b.link("links/dsetA").exists());
    REQUIRE(file_b.dataset("links/dsetA").read<double>() == Catch::Approx(42.0));
    REQUIRE(file_b.attribute("links/dsetA", "dataset_attr").read<std::string>() == "from-a");
}

TEST_CASE("External links can target groups through a path relative to the current file", "[external-link][group]") {
    auto source_dir = h5pp::fs::path(H5PP_TEST_DIR "externalLinks");
    auto nested_dir = source_dir / "nested";

    h5pp::File file_a(source_dir / "source.h5", h5pp::FileAccess::REPLACE, 0);
    file_a.dataset("groupA/dsetA").write(std::vector<int>{1, 2, 3});
    file_a.dataset("groupA/text").write(std::string("text"));
    file_a.attribute("groupA", "group_attr").write(std::string("group-attr"));

    h5pp::File file_b(nested_dir / "target.h5", h5pp::FileAccess::REPLACE, 0);
    file_b.link("external/groupA").createExternalLink("../source.h5", "groupA");

    REQUIRE(file_b.dataset("external/groupA/dsetA").read<std::vector<int>>() == std::vector<int>{1, 2, 3});
    REQUIRE(file_b.dataset("external/groupA/text").read<std::string>() == "text");
    REQUIRE(file_b.attribute("external/groupA", "group_attr").read<std::string>() == "group-attr");
}

TEST_CASE("Creating an external link to a missing target does not eagerly fail, but dereferencing it does", "[external-link][errors]") {
    h5pp::File file(make_path("externalLink-missing"), h5pp::FileAccess::REPLACE, 0);
    file.link("dangling/link").createExternalLink("missing-target.h5", "group/dset");

    REQUIRE(file.link("dangling/link").exists());
    REQUIRE_THROWS(file.dataset("dangling/link").read<double>());
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    session.configData().shouldDebugBreak = true;
    return session.run();
}
