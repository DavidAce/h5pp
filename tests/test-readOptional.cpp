#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }
}

TEST_CASE("Optional reads return values for existing objects and nullopt for missing ones", "[optional]") {
    auto path = make_path("readOptional");

    h5pp::File writer(path, h5pp::FileAccess::REPLACE, 0);
    REQUIRE_NOTHROW(writer.dataset("someGroup/someNumber").write(42.0));
    REQUIRE_NOTHROW(writer.dataset("someGroup/vector").write(std::vector<int>{1, 2, 3}));
    REQUIRE_NOTHROW(writer.attribute("someGroup/someNumber", "someComment").write(std::string("My favorite number")));
    REQUIRE_NOTHROW(writer.attribute("someGroup/vector", "labels").write(std::vector<std::string>{"alpha", "beta"}));

    h5pp::File file(path, h5pp::FileAccess::READWRITE, 0);

    auto number  = file.dataset("someGroup/someNumber").read<std::optional<double>>();
    auto vector  = file.dataset("someGroup/vector").read<std::optional<std::vector<int>>>();
    auto comment = file.attribute("someGroup/someNumber", "someComment").read<std::optional<std::string>>();
    auto labels  = file.attribute("someGroup/vector", "labels").read<std::optional<std::vector<std::string>>>();

    REQUIRE(number.has_value());
    REQUIRE(vector.has_value());
    REQUIRE(comment.has_value());
    REQUIRE(labels.has_value());

    REQUIRE(number.value() == 42.0);
    REQUIRE(vector.value() == std::vector<int>{1, 2, 3});
    REQUIRE(comment.value() == "My favorite number");
    REQUIRE(labels.value() == std::vector<std::string>{"alpha", "beta"});

    auto missing_number  = file.dataset("someGroup/anotherNumber").read<std::optional<double>>();
    auto missing_vector  = file.dataset("someGroup/missingVector").read<std::optional<std::vector<int>>>();
    auto missing_comment = file.attribute("someGroup/someNumber", "anotherComment").read<std::optional<std::string>>();
    auto missing_labels  = file.attribute("someGroup/vector", "missingLabels").read<std::optional<std::vector<std::string>>>();

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
