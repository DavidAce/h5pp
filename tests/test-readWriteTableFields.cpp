#if defined(__GNUC__) || defined(__clang__)
    #define PACK(__Declaration__) __Declaration__ __attribute__((packed, aligned(1)))
#elif defined(_MSC_VER)
    #define PACK(__Declaration__) __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#else
    #define PACK(__Declaration__) __Declaration__
#endif

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <h5pp/h5pp.h>
#include <iostream>

// Define the main table type that will be written
struct Particle {
    double x = 0, y = 1, z = 2, t = 3;
    double rho[3]   = {20, 3.13, 102.4};
    char   name[10] = "some name"; // Can be replaced by std::string
    void   dummy_function(int) {}
};

// Define structs that are subsets of a particle
struct Rho {
    double rho[3];
};

struct Axis {
    double axis = 0;
};

struct Coords {
    double x = 0, y = 0, z = 0, t = 0;
};

// Pay attention to the attribute here which gets rid of padding on the struct
PACK(struct RhoName {
    double rho[3];
    char   name[10];
});

TEST_CASE("Test reading columns from table", "[Table fields]") {
    SECTION("Initialize a file") {
        h5pp::File file("output/readWriteTableFields.h5", h5pp::FilePermission::REPLACE, 2);
        // Create a type for the char array from the template H5T_C_S1
        // The template describes a string with a single char.
        // Set the size with H5Tset_size.
        h5pp::hid::h5t MY_HDF5_NAME_TYPE = H5Tcopy(H5T_C_S1);
        H5Tset_size(MY_HDF5_NAME_TYPE, 10);
        // Optionally set the null terminator '\0' and possibly padding.
        H5Tset_strpad(MY_HDF5_NAME_TYPE, H5T_STR_NULLTERM);

        // Specify the array "rho" as rank-1 array of length 3
        std::vector<hsize_t> dims             = {3};
        h5pp::hid::h5t       MY_HDF5_RHO_TYPE = H5Tarray_create(H5T_NATIVE_DOUBLE, (unsigned int) dims.size(), dims.data());

        // Register the compound type
        h5pp::hid::h5t MY_HDF5_PARTICLE_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
        H5Tinsert(MY_HDF5_PARTICLE_TYPE, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
        H5Tinsert(MY_HDF5_PARTICLE_TYPE, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
        H5Tinsert(MY_HDF5_PARTICLE_TYPE, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
        H5Tinsert(MY_HDF5_PARTICLE_TYPE, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);
        H5Tinsert(MY_HDF5_PARTICLE_TYPE, "rho", HOFFSET(Particle, rho), MY_HDF5_RHO_TYPE);
        H5Tinsert(MY_HDF5_PARTICLE_TYPE, "name", HOFFSET(Particle, name), MY_HDF5_NAME_TYPE);

        file.createTable(MY_HDF5_PARTICLE_TYPE, "somegroup/particleTable", "particleTable");

        // Write table entries
        std::vector<Particle> particles(10);
        file.appendTableRecords(particles, "somegroup/particleTable");
    }

    SECTION("Single field by name and index") {
        h5pp::File        file("output/readWriteTableFields.h5", h5pp::FilePermission::READWRITE, 2);
        std::vector<Axis> axis_fields;
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", "y"));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", std::string("y")));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", std::string_view("y")));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", {"y"}));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", std::initializer_list<std::string>{"y"}));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", std::initializer_list<std::string_view>{"y"}));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", std::vector<std::string>{"y"}));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", std::vector<std::string_view>{"y"}));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", std::array<std::string, 1>{"y"}));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", std::array<std::string_view, 1>{"y"}));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", 1));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", {1}));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", std::vector<size_t>{1}));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", std::vector<long>{1}));
        axis_fields.emplace_back(file.readTableField<Axis>("somegroup/particleTable", std::array<size_t, 1>{1}));
        for(auto &a : axis_fields) { CHECK(a.axis == 1.0); }
    }

    SECTION("Single struct field by name") {
        h5pp::File file("output/readWriteTableFields.h5", h5pp::FilePermission::READWRITE, 2);
        auto       rho_field = file.readTableField<Rho>("somegroup/particleTable", "rho");
        CHECK(rho_field.rho[0] == 20);
        CHECK(rho_field.rho[1] == 3.13);
        CHECK(rho_field.rho[2] == 102.4);
    }

    SECTION("Multiple fields by name and index") {
        h5pp::File          file("output/readWriteTableFields.h5", h5pp::FilePermission::READWRITE, 2);
        std::vector<Coords> coords_fields;
        coords_fields.emplace_back(file.readTableField<Coords>("somegroup/particleTable", {"x", "y", "z", "t"}));
        coords_fields.emplace_back(file.readTableField<Coords>("somegroup/particleTable", std::vector<std::string>{"x", "y", "z", "t"}));
        coords_fields.emplace_back(
            file.readTableField<Coords>("somegroup/particleTable", std::vector<std::string_view>{"x", "y", "z", "t"}));
        coords_fields.emplace_back(file.readTableField<Coords>("somegroup/particleTable", std::array<std::string, 4>{"x", "y", "z", "t"}));
        coords_fields.emplace_back(file.readTableField<Coords>("somegroup/particleTable", {0, 1, 2, 3}));
        coords_fields.emplace_back(file.readTableField<Coords>("somegroup/particleTable", std::initializer_list<size_t>{0, 1, 2, 3}));
        coords_fields.emplace_back(file.readTableField<Coords>("somegroup/particleTable", std::vector<size_t>{0, 1, 2, 3}));
        coords_fields.emplace_back(file.readTableField<Coords>("somegroup/particleTable", std::array<size_t, 4>{0, 1, 2, 3}));
        for(auto &c : coords_fields) {
            CHECK(c.x == 0.0);
            CHECK(c.y == 1.0);
            CHECK(c.z == 2.0);
            CHECK(c.t == 3.0);
        }
    }

    SECTION("Multiple fields of different type by name and index") {
        h5pp::File           file("output/readWriteTableFields.h5", h5pp::FilePermission::READWRITE, 2);
        std::vector<RhoName> rhoName_fields;
        rhoName_fields.emplace_back(file.readTableField<RhoName>("somegroup/particleTable", {"rho", "name"}));
        rhoName_fields.emplace_back(file.readTableField<RhoName>("somegroup/particleTable", std::vector<std::string>{"rho", "name"}));
        rhoName_fields.emplace_back(file.readTableField<RhoName>("somegroup/particleTable", std::vector<std::string_view>{"rho", "name"}));
        rhoName_fields.emplace_back(file.readTableField<RhoName>("somegroup/particleTable", std::array<std::string, 2>{"rho", "name"}));
        rhoName_fields.emplace_back(file.readTableField<RhoName>("somegroup/particleTable", {4, 5}));
        rhoName_fields.emplace_back(file.readTableField<RhoName>("somegroup/particleTable", std::initializer_list<size_t>{4, 5}));
        rhoName_fields.emplace_back(file.readTableField<RhoName>("somegroup/particleTable", std::vector<size_t>{4, 5}));
        rhoName_fields.emplace_back(file.readTableField<RhoName>("somegroup/particleTable", std::array<size_t, 2>{4, 5}));
        for(auto &rn : rhoName_fields) {
            CHECK(rn.rho[0] == 20);
            CHECK(rn.rho[1] == 3.13);
            CHECK(rn.rho[2] == 102.4);
            CHECK(strncmp(rn.name, "some name", 10) == 0);
        }
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