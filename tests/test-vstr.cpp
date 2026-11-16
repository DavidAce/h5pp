#include <catch2/catch_all.hpp>
#include <h5pp/h5ppVstr.h>
#include <string>
#include <string_view>
#include <utility>

TEST_CASE("Variable-length strings can be move-constructed", "[vstr]") {
    SECTION("populated source") {
        h5pp::vstr_t source("hello");
        h5pp::vstr_t moved(std::move(source));
        CHECK(moved == std::string_view("hello"));
        CHECK(moved.size() == 5);
        CHECK(moved.c_str()[moved.size()] == '\0');
    }
    SECTION("empty source") {
        h5pp::vstr_t source;
        h5pp::vstr_t moved(std::move(source));
        CHECK(moved.empty());
        moved.append("hello");
        CHECK(moved == std::string_view("hello"));
        CHECK(moved.c_str()[moved.size()] == '\0');
    }
}

TEST_CASE("Assigning a string view copies only the selected characters", "[vstr]") {
    const std::string text = "prefix-target-suffix";
    h5pp::vstr_t      value("old contents");
    value = std::string_view(text).substr(7, 6);
    CHECK(value == std::string_view("target"));
    CHECK(value.size() == 6);
    CHECK(value.c_str()[value.size()] == '\0');

    value = std::string_view{};
    CHECK(value.empty());
    CHECK(value.c_str()[0] == '\0');
}

TEST_CASE("Variable-length strings support erasing, appending and iteration", "[vstr]") {
    h5pp::vstr_t value("abcdef");
    SECTION("erase by position and count") { value.erase(1, 3); }
    SECTION("erase by iterator range") { value.erase(value.begin() + 1, value.begin() + 4); }
    CHECK(value == std::string_view("aef"));
    CHECK(value.size() == 3);
    CHECK(value.c_str()[value.size()] == '\0');

    value.append(std::string("gh"));
    CHECK(value == std::string_view("aefgh"));
    CHECK(value.size() == 5);
    CHECK(value.c_str()[value.size()] == '\0');
    CHECK(std::string(value.begin(), value.end()) == "aefgh");
    const auto &constant = value;
    CHECK(std::string(constant.begin(), constant.end()) == "aefgh");

    value.erase(0, value.size());
    CHECK(value.empty());
    CHECK(value.c_str()[0] == '\0');
}

int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
