#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <h5pp/h5pp.h>

TEST_CASE("Test reading into std::optional", "[Optional]") {
    SECTION("Create a file and add some dummy data") {
        h5pp::File file("output/readOptional.h5", h5pp::FileAccess::REPLACE, 2);
        REQUIRE_NOTHROW(file.writeDataset(42.0, "someGroup/someNumber"));
        REQUIRE_NOTHROW(file.writeAttribute("My favorite number", "someGroup/someNumber", "someComment"));
    }
    SECTION("Read an existing dataset into std::optional") {
        h5pp::File                 file("output/readOptional.h5", h5pp::FileAccess::READWRITE, 2);
        std::optional<double>      dset;
        std::optional<std::string> attr;
        REQUIRE_NOTHROW(dset = file.readDataset<std::optional<double>>("someGroup/someNumber"));
        REQUIRE_NOTHROW(attr = file.readAttribute<std::optional<std::string>>("someGroup/someNumber", "someComment"));
        REQUIRE(dset.has_value());
        REQUIRE(attr.has_value());

        REQUIRE(dset.value() == 42.0);
        REQUIRE(attr.value() == "My favorite number");
    }
    SECTION("Read an existing dataset into std::optional") {
        h5pp::File                 file("output/readOptional.h5", h5pp::FileAccess::READWRITE, 2);
        std::optional<double>      dset;
        std::optional<std::string> attr;
        REQUIRE_NOTHROW(dset = file.readDataset<std::optional<double>>("someGroup/anotherNumber"));
        REQUIRE_NOTHROW(attr = file.readAttribute<std::optional<std::string>>("someGroup/someNumber", "anotherComment"));
        REQUIRE(not dset.has_value());
        REQUIRE(not attr.has_value());
    }
}

int main(int argc, char *argv[]) {
    Catch::Session session; // There must be exactly one instance
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) // Indicates a command line error
        return returnCode;
    //    session.configData().showSuccessfulTests = true;
    //    session.configData().reporterName = "compact";
    return session.run();
}