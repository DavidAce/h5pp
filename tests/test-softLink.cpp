#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <h5pp/h5pp.h>

TEST_CASE("Test adding a soft link", "[Soft Link]") {
    SECTION("Create a file and add some dummy data") {
        h5pp::File file("output/softLink.h5", h5pp::FileAccess::REPLACE, 2);
        REQUIRE_NOTHROW(file.writeDataset(42.0, "realgroup1/realgroup2/dsetA"));
        REQUIRE_NOTHROW(file.writeDataset(43.0, "realgroup1/realgroup2/realgroup3/dsetB"));
    }
    SECTION("Add some soft links into softlinks") {
        h5pp::File file("output/softLink.h5", h5pp::FileAccess::READWRITE, 2);
        REQUIRE_NOTHROW(file.createSoftLink("realgroup1/realgroup2",            "softlinks/realgroup2"));
        REQUIRE_NOTHROW(file.createSoftLink("realgroup1/realgroup2/realgroup3", "softlinks/realgroup3"));
    }
    SECTION("Find the datasets through soft links") {
        h5pp::File file("output/softLink.h5", h5pp::FileAccess::READONLY, 2);
        REQUIRE(file.linkExists("softlinks/realgroup2/dsetA"));
        REQUIRE(file.linkExists("softlinks/realgroup3/dsetB"));
    }
    SECTION("Write a new dataset through the soft link") {
        h5pp::File file("output/softLink.h5", h5pp::FileAccess::READWRITE, 2);
        REQUIRE_NOTHROW(file.writeDataset(44.0, "softlinks/realgroup2/dsetC"));
        REQUIRE_NOTHROW(file.writeDataset(45.0, "softlinks/realgroup3/dsetD"));
        REQUIRE(file.linkExists("softlinks/realgroup2/dsetC"));
        REQUIRE(file.linkExists("softlinks/realgroup3/dsetD"));
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