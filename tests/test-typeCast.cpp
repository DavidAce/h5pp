#include <catch2/catch_all.hpp>
#include <cstdint>
#include <h5pp/h5ppTypeCast.h>
#include <limits>

TEST_CASE("Integral casts preserve values when the target can represent them", "[type-cast]") {
    using h5pp::type::safe_cast;
    CHECK(safe_cast<int>(-42) == -42);
    CHECK(safe_cast<int16_t>(std::numeric_limits<int8_t>::min()) == -128);
    CHECK(safe_cast<int16_t>(std::numeric_limits<int8_t>::max()) == 127);
    CHECK(safe_cast<uint16_t>(std::numeric_limits<uint8_t>::max()) == 255);
    CHECK(safe_cast<int64_t>(std::numeric_limits<uint32_t>::max()) == INT64_C(4294967295));
}

TEST_CASE("Unsigned narrowing accepts its upper limit and rejects overflow", "[type-cast]") {
    using h5pp::type::safe_cast;
    CHECK(safe_cast<uint8_t>(uint16_t{0}) == 0);
    CHECK(safe_cast<uint8_t>(uint16_t{255}) == 255);
    REQUIRE_THROWS_WITH(safe_cast<uint8_t>(uint16_t{256}), Catch::Matchers::ContainsSubstring("integral overflow"));
}

TEST_CASE("Signed narrowing checks both limits", "[type-cast]") {
    using h5pp::type::safe_cast;
    CHECK(safe_cast<int8_t>(int16_t{-128}) == -128);
    CHECK(safe_cast<int8_t>(int16_t{127}) == 127);
    REQUIRE_THROWS_WITH(safe_cast<int8_t>(int16_t{-129}), Catch::Matchers::ContainsSubstring("integral sign error"));
    REQUIRE_THROWS_WITH(safe_cast<int8_t>(int16_t{128}), Catch::Matchers::ContainsSubstring("integral overflow"));
}

TEST_CASE("Signed to unsigned casts reject negative values and overflow", "[type-cast]") {
    using h5pp::type::safe_cast;
    CHECK(safe_cast<uint8_t>(int16_t{0}) == 0);
    CHECK(safe_cast<uint8_t>(int16_t{255}) == 255);
    REQUIRE_THROWS_WITH(safe_cast<uint8_t>(int16_t{-1}), Catch::Matchers::ContainsSubstring("integral sign error"));
    REQUIRE_THROWS_WITH(safe_cast<uint8_t>(int16_t{256}), Catch::Matchers::ContainsSubstring("integral overflow"));
}

TEST_CASE("Unsigned to signed casts reject values above the signed maximum", "[type-cast]") {
    using h5pp::type::safe_cast;
    CHECK(safe_cast<int8_t>(uint16_t{0}) == 0);
    CHECK(safe_cast<int8_t>(uint16_t{127}) == 127);
    REQUIRE_THROWS_WITH(safe_cast<int8_t>(uint16_t{128}), Catch::Matchers::ContainsSubstring("integral overflow"));
    REQUIRE_THROWS_WITH(safe_cast<int8_t>(uint8_t{255}), Catch::Matchers::ContainsSubstring("integral overflow"));
}

int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
