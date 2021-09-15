#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <h5pp/h5pp.h>

TEST_CASE("Test finding links in a file", "[Find dataset]") {
    SECTION("Write dummy datasets"){
        h5pp::File file("output/findObjects.h5", h5pp::FilePermission::REPLACE, 2);
        file.writeDataset(0.0, "dsetA");
        file.writeDataset(0.1, "group1/dsetB2");
        file.writeDataset(0.1, "group1/dsetB1");
        file.writeDataset(0.1, "group1/dsetB0");
        file.writeDataset(0.2, "group1/group2/dsetC");
        file.writeDataset(0.2, "group3/group4/group5/dsetD");
        file.writeDataset(0.2, "group3/group4/group5/dsetE");
    }
    SECTION("Find datasets. Test typical usage") {
        h5pp::File file("output/findObjects.h5", h5pp::FilePermission::READONLY, 2);
        REQUIRE(file.findDatasets("dsetB1") == std::vector<std::string>{"group1/dsetB1"});
        REQUIRE(file.findDatasets("dsetA").size() == 1);
        REQUIRE(file.findDatasets("dsetB").size() == 3);
        REQUIRE(file.findDatasets("dsetC").size() == 1);
        REQUIRE(file.findDatasets("dsetD").size() == 1);
        REQUIRE(file.findDatasets("dsetE").size() == 1);

        REQUIRE(file.findDatasets("dsetD", "group3/group4/").size() == 1);
        REQUIRE(file.findDatasets("dset", "group3/group4").size() == 2);
        REQUIRE(file.findDatasets("dset", "/group3/group4").size() == 2);


    }
    SECTION("Find datasets. Test path conventions, custom root, maxhits and maxdepth") {
        h5pp::File file("output/findObjects.h5", h5pp::FilePermission::READONLY, 2);
        REQUIRE(file.findDatasets("dset", "group3/group4/", 1,0 ) == std::vector<std::string>{});
        REQUIRE(file.findDatasets("dset", "group3/group4/", 1,1 ) == std::vector<std::string>{"group5/dsetD"});
        REQUIRE(file.findDatasets("dset", "/group3/group4/", 1,1 ) == std::vector<std::string>{"group5/dsetD"});
        REQUIRE(file.findDatasets("dset", "group3/group4", 1,0 ) == std::vector<std::string>{});
        REQUIRE(file.findDatasets("dset", "group3/group4", 1,1 ) == std::vector<std::string>{"group5/dsetD"});
        REQUIRE(file.findDatasets("dset", "/group3/group4", 1,1 ) == std::vector<std::string>{"group5/dsetD"});
    }

    SECTION("Find groups. Test typical usage"){
        h5pp::File file("output/findObjects.h5", h5pp::FilePermission::READONLY, 2);
        REQUIRE(file.findGroups("group1") == std::vector<std::string>{"group1"});
        REQUIRE(file.findGroups("group5") == std::vector<std::string>{"group3/group4/group5"});
        REQUIRE(file.findGroups("group").size() == 5);
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
