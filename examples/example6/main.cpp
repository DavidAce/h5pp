
#include <h5pp/h5pp.h>
#include <iostream>

// In this example we want to treat a whole struct as a single writeable unit again,
// but as opposed to example 5, this time write it as entries in an HDF5 table.
// Tables are suitable for time-series data, for instance.

// To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.
// First invent a typical "plain-old-data" (POD) struct.
// Note that it cannot have dynamic-size members.
// The char-array member will add some complexity to the problem, since
// it also must be defined in advance.
struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
    char   name[10] = "some name"; // Cannot be replaced by std::string
};

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example6.h5", h5pp::FilePermission::REPLACE);

    // Create a type for the char array from the template H5T_C_S1
    // The template describes a string with a single char.
    // Note that we use h5pp::hid::h5t to describe types in a type-safe way,
    // but regular hid_t (or auto works) just as well.
    h5pp::hid::h5t MY_CHAR10_TYPE = H5Tcopy(H5T_C_S1);
    // Set the actual size with H5Tset_size, or h5pp::hdf5::setStringSize(...)
    H5Tset_size(MY_CHAR10_TYPE, 10);
    // Optionally set the null terminator '\0'
    H5Tset_strpad(MY_CHAR10_TYPE, H5T_STR_NULLTERM);

    // Register the compound type
    h5pp::hid::h5t MY_HDF5_PARTICLE_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "name", HOFFSET(Particle, name), MY_CHAR10_TYPE);

    // Create an empty table
    file.createTable(MY_HDF5_PARTICLE_TYPE, "somegroup/particleTable", "Table Title");

    // Write table entries
    std::vector<Particle> particles(10);
    file.appendTableEntries(particles, "somegroup/particleTable");

    // Read single entry

    // NOTE
    // If none of startEntry or numEntries (arguments 3 and 4) are given to readTableEntries:
    //          If container is resizeable: startEntry = 0, numEntries = totalRecords
    //          If container is not resizeable: startEntry = last entry, numEntries = 1.
    // If startEntry is given but numEntries is not:
    //          If container is resizeable -> read from startEntries to the end
    //          If container is not resizeable -> read from startEntries a single entry
    // If numEntries given but startEntries is not -> read the last numEntries records

    auto particle_read = file.readTableEntries<Particle>("somegroup/particleTable");

    std::cout << "Single entry read \n"
              << " \t x: " << particle_read.x << " \t y: " << particle_read.y << " \t z: " << particle_read.z << " \t t: " << particle_read.t << " \t name: " << particle_read.name
              << std::endl;

    // Or read multiple entries into a resizeable container. Start from entry 0 and read 10 entries.
    auto particles_read = file.readTableEntries<std::vector<Particle>>("somegroup/particleTable", 0, 10);
    std::cout << "Multiple entries read \n";
    for(auto &elem : particles_read) {
        std::cout << " \t x: " << elem.x << " \t y: " << elem.y << " \t z: " << elem.z << " \t t: " << elem.t << " \t name: " << elem.name << std::endl;
    }

    return 0;
}