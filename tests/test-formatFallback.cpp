#if __has_include(<format>)
    #include <format>
#endif
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <h5pp/h5ppFormat.h>

TEST_CASE("Fallback runtime formatting is independent of std::format", "[format]") {
    std::string       name      = "example";
    const std::string constName = "constant";
    std::string       pattern   = "Value: {}";

    CHECK(h5pp::format_runtime("{}", name) == "example");
    CHECK(h5pp::format_runtime("{}", constName) == "constant");
    CHECK(h5pp::format_runtime("{}", std::string("temporary")) == "temporary");
    CHECK(h5pp::format_runtime(pattern, name) == "Value: example");
    CHECK(h5pp::format_runtime(std::string_view("{}: {}"), name, 42) == "example: 42");
    CHECK(h5pp::format_runtime("plain text") == "plain text");
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    return session.run(argc, argv);
}
