#include <catch2/catch_all.hpp>
#include <h5pp/v1/h5pp.h>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }
}

TEST_CASE("Optional reads return values for existing objects and nullopt for missing ones", "[optional]") {
    auto path = make_path("readOptional");

    h5pp::v1::File writer(path, h5pp::FileAccess::REPLACE, 0);
    REQUIRE_NOTHROW(writer.writeDataset(42.0, "someGroup/someNumber"));
    REQUIRE_NOTHROW(writer.writeDataset(std::vector<int>{1, 2, 3}, "someGroup/vector"));
    REQUIRE_NOTHROW(writer.writeAttribute("My favorite number", "someGroup/someNumber", "someComment"));
    REQUIRE_NOTHROW(writer.writeAttribute(std::vector<std::string>{"alpha", "beta"}, "someGroup/vector", "labels"));

    h5pp::v1::File file(path, h5pp::FileAccess::READWRITE, 0);

    auto number  = file.readDataset<std::optional<double>>("someGroup/someNumber");
    auto vector  = file.readDataset<std::optional<std::vector<int>>>("someGroup/vector");
    auto comment = file.readAttribute<std::optional<std::string>>("someGroup/someNumber", "someComment");
    auto labels  = file.readAttribute<std::optional<std::vector<std::string>>>("someGroup/vector", "labels");

    REQUIRE(number.has_value());
    REQUIRE(vector.has_value());
    REQUIRE(comment.has_value());
    REQUIRE(labels.has_value());

    REQUIRE(number.value() == 42.0);
    REQUIRE(vector.value() == std::vector<int>{1, 2, 3});
    REQUIRE(comment.value() == "My favorite number");
    REQUIRE(labels.value() == std::vector<std::string>{"alpha", "beta"});

    auto missing_number  = file.readDataset<std::optional<double>>("someGroup/anotherNumber");
    auto missing_vector  = file.readDataset<std::optional<std::vector<int>>>("someGroup/missingVector");
    auto missing_comment = file.readAttribute<std::optional<std::string>>("someGroup/someNumber", "anotherComment");
    auto missing_labels  = file.readAttribute<std::optional<std::vector<std::string>>>("someGroup/vector", "missingLabels");

    REQUIRE_FALSE(missing_number.has_value());
    REQUIRE_FALSE(missing_vector.has_value());
    REQUIRE_FALSE(missing_comment.has_value());
    REQUIRE_FALSE(missing_labels.has_value());
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
