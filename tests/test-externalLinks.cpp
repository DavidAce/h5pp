#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <h5pp/h5pp.h>

TEST_CASE("Test adding external links from other another file", "[External Link]") {
    SECTION("Create an external file A") {
        h5pp::File fileA("output/externalLink-A.h5", h5pp::FileAccess::REPLACE, 2);
        fileA.writeDataset(42.0, "dsetA");
    }
    SECTION("Add external link to file B") {
        h5pp::File fileB("output/externalLink-B.h5", h5pp::FileAccess::REPLACE, 2);
        fileB.createExternalLink("externalLink-A.h5", "dsetA", "dsetA");
        REQUIRE(fileB.readDataset<double>("dsetA") == 42.0);
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