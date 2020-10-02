#if defined(__GNUC__) || defined(__clang__)
    #define PACK( __Declaration__ ) __Declaration__ __attribute__((packed, aligned(1)))
#elif defined(_MSC_VER)
    #define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#else
    #define PACK( __Declaration__ ) __Declaration__
#endif


#include <h5pp/h5pp.h>
#include <iostream>

struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
    double rho[3]   = {20, 3.13, 102.4};
    char   name[10] = "some name"; // Can be replaced by std::string
    void   dummy_function(int) {}
};

int main() {
    h5pp::File file("output/readWriteTableFields.h5", h5pp::FilePermission::REPLACE, 0);

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

    // Try reading just a column in a table

    struct Rho {
        double rho[3];
    };
    // Read a single
    auto rho_field = file.readTableField<Rho>("somegroup/particleTable", "rho");
    h5pp::print("Rho: x={} y={} z={}\n", rho_field.rho[0], rho_field.rho[1], rho_field.rho[2]);

    // Read a few
    auto rho_fields_few = file.readTableField<std::vector<Rho>>("somegroup/particleTable", "rho", 5, 3);
    for(auto &f : rho_fields_few) h5pp::print("Rho: x={} y={} z={}\n", f.rho[0], f.rho[1], f.rho[2]);

    // Read them all
    auto rho_fields_all = file.readTableField<std::vector<Rho>>("somegroup/particleTable", "rho");
    for(auto &f : rho_fields_all) h5pp::print("Rho: x={} y={} z={}\n", f.rho[0], f.rho[1], f.rho[2]);

    // Try reading two columns
    // Pay attention to the attribute which gets rid of padding on the struct
    PACK(struct RhoName {
        double rho[3];
        char   name[10];
    });
    h5pp::print("Size of RhoName = {}\n", sizeof(RhoName));
    // Read a single
    auto rhoname_field = file.readTableField<RhoName>("somegroup/particleTable", {"rho", "name"});
    h5pp::print("Rho: x={} y={} z={} name={}\n", rhoname_field.rho[0], rhoname_field.rho[1], rhoname_field.rho[2], rhoname_field.name);

    // Read a few
    auto rhoname_fields_few = file.readTableField<std::vector<RhoName>>("somegroup/particleTable", {"rho", "name"}, 5, 3);
    for(auto &f : rhoname_fields_few) h5pp::print("Rho: x={} y={} name={} name={}\n", f.rho[0], f.rho[1], f.rho[2], f.name);

    // Read them all
    auto rhoname_fields_all = file.readTableField<std::vector<RhoName>>("somegroup/particleTable", {"rho", "name"});
    for(auto &f : rhoname_fields_all) h5pp::print("Rho: x={} y={} z={} name={}\n", f.rho[0], f.rho[1], f.rho[2], f.name);

    return 0;
}