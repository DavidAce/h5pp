
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <h5pp/h5pp.h>
#include <iostream>

struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
    double rho[3]   = {20, 3.13, 102.4};
    char   name[10] = "some name"; // Can be replaced by std::string
    void   dummy_function(int) {}
};

void compare(const Particle &lhs, const Particle &rhs) {
    CHECK(lhs.x == rhs.x);
    CHECK(lhs.y == rhs.y);
    CHECK(lhs.z == rhs.z);
    CHECK(lhs.t == rhs.t);
    CHECK(lhs.rho[0] == rhs.rho[0]);
    CHECK(lhs.rho[1] == rhs.rho[1]);
    CHECK(lhs.rho[2] == rhs.rho[2]);
    CHECK(strncmp(lhs.name, rhs.name, 10) == 0);
}

h5pp::File     file("output/readWriteTables.h5", h5pp::FileAccess::REPLACE, 2);
h5pp::hid::h5t MY_HDF5_NAME_TYPE;
h5pp::hid::h5t MY_HDF5_RHO_TYPE;
h5pp::hid::h5t MY_HDF5_PARTICLE_TYPE;

TEST_CASE("Test reading columns from table", "[Table fields]") {
    SECTION("Initialize file") {
        file.setCompressionLevel(6);
        // Create a type for the char array from the template H5T_C_S1
        // The template describes a string with a single char.
        // Set the size with H5Tset_size.
        MY_HDF5_NAME_TYPE = H5Tcopy(H5T_C_S1);
        H5Tset_size(MY_HDF5_NAME_TYPE, 10);
        // Optionally set the null terminator '\0' and possibly padding.
        H5Tset_strpad(MY_HDF5_NAME_TYPE, H5T_STR_NULLTERM);

        // Specify the array "rho" as rank-1 array of length 3
        std::vector<hsize_t> rho_dims = {3};
        MY_HDF5_RHO_TYPE              = H5Tarray_create(H5T_NATIVE_DOUBLE, (unsigned int) rho_dims.size(), rho_dims.data());

        // Register the compound type
        MY_HDF5_PARTICLE_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
        H5Tinsert(MY_HDF5_PARTICLE_TYPE, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
        H5Tinsert(MY_HDF5_PARTICLE_TYPE, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
        H5Tinsert(MY_HDF5_PARTICLE_TYPE, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
        H5Tinsert(MY_HDF5_PARTICLE_TYPE, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);
        H5Tinsert(MY_HDF5_PARTICLE_TYPE, "rho", HOFFSET(Particle, rho), MY_HDF5_RHO_TYPE);
        H5Tinsert(MY_HDF5_PARTICLE_TYPE, "name", HOFFSET(Particle, name), MY_HDF5_NAME_TYPE);
    }

    SECTION("Create table") {
        auto tableInfo = file.createTable(MY_HDF5_PARTICLE_TYPE, "somegroup/particleTable", "particleTable", std::nullopt, 6);
        CHECK(tableInfo.tableTitle.value() == "particleTable");
        CHECK(tableInfo.numRecords.value() == 0);
        CHECK(tableInfo.recordBytes.value() == sizeof(Particle));
        CHECK_THAT(tableInfo.fieldSizes.value(), Catch::Equals(std::vector<size_t>{8, 8, 8, 8, 24, 10}));
    }

    SECTION("Write table entries") {
        std::vector<Particle> particles(10);
        auto                  tableInfo = file.appendTableRecords(particles, "somegroup/particleTable");
        CHECK(tableInfo.tableTitle.value() == "particleTable");
        CHECK(tableInfo.numRecords.value() == 10);
        CHECK(tableInfo.recordBytes.value() == sizeof(Particle));
        CHECK_THAT(tableInfo.fieldSizes.value(), Catch::Equals(std::vector<size_t>{8, 8, 8, 8, 24, 10}));
    }

    SECTION("Read single entry") {
        std::vector<Particle> particle_read;
        particle_read.emplace_back(file.readTableRecords<Particle>("somegroup/particleTable"));
        particle_read.emplace_back(file.readTableRecords<Particle>(std::string("somegroup/particl\0eTable", 24))); // Non-standard string!
        particle_read.emplace_back(file.readTableRecords<Particle>(std::string("somegroup/particleTable")));
        particle_read.emplace_back(file.readTableRecords<Particle>(std::string_view("somegroup/particleTable")));
        for(auto &p : particle_read) compare(p, Particle());
    }

    SECTION("Read multiple entries into resizeable container") {
        std::vector<std::vector<Particle>> result_container;
        result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable"));
        result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", 0));
        result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", 5));
        result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", std::nullopt, 5));
        result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", 0, std::nullopt));
        result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", 0, 5));
        result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", h5pp::TableSelection::ALL));
        result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", h5pp::TableSelection::FIRST));
        result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", h5pp::TableSelection::LAST));
        for(auto &particle_read : result_container)
            for(auto &p : particle_read) compare(p, Particle());
    }
    SECTION("Copy a table entry to another file") {
        auto       info1 = file.getTableInfo("somegroup/particleTable");
        h5pp::File file2("output/readWriteTablesCopy.h5", h5pp::FileAccess::REPLACE, 2);
        auto       info2 = file2.appendTableRecords(file.openFileHandle(),
                                                    "somegroup/particleTable",
                                                    "somegroup/particleTable",
                                                    h5pp::TableSelection::LAST);
        CHECK(info2.tableTitle.value() == info1.tableTitle.value());
        CHECK(info2.numRecords.value() == 1);
        CHECK(info2.recordBytes.value() == info1.recordBytes.value());
        CHECK_THAT(info2.fieldSizes.value(), Catch::Equals(info1.fieldSizes.value()));
    }
}

int main(int argc, char *argv[]) {
    Catch::Session session; // There must be exactly one instance
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) // Indicates a command line error
        return returnCode;

    //    session.configData().showSuccessfulTests = true;
    //    session.configData().reporterName = "compact";
    session.configData().shouldDebugBreak = true;

    return session.run();
}
