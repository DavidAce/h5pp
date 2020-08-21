
#include <h5pp/h5pp.h>
#include <iostream>

// In this example we want to treat a whole struct as a single writeable unit again,
// but as opposed to example 5, this time write it as entries in an HDF5 table.
// Tables are suitable for time-series data, for instance.

// To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.
// First define typical struct.
// Note that it cannot have dynamic-size members.
struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
};


int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-03b-tables.h5", h5pp::FilePermission::REPLACE,0);

    // Register the compound type
    h5pp::hid::h5t H5_PARTICLE_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
    H5Tinsert(H5_PARTICLE_TYPE, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);

    // Create an empty table
    file.createTable(H5_PARTICLE_TYPE, "somegroup/particleTable", "Title");

    // Write table entries
    std::vector<Particle> particles(10,{1,2,3,4});
    file.appendTableRecords(particles, "somegroup/particleTable");

    // Read single entry

    // NOTE
    // The full signature of readTableRecords(...) is
    // readTableRecords(std::string_view tablePath, std::optional<size_t> startEntry = std::nullopt, std::optional<size_t> numEntries = std::nullopt)
    //
    // If none of startEntry or numEntries (arguments 3 and 4) are given to readTableRecords:
    //          If container is resizeable: startEntry = 0, numEntries = totalRecords
    //          If container is not resizeable: startEntry = last entry, numEntries = 1.
    // If startEntry is given but numEntries is not:
    //          If container is resizeable -> read from startEntries to the end
    //          If container is not resizeable -> read from startEntries a single entry
    // If numEntries given but startEntries is not -> read the last numEntries records
    //
    // A "resizeable" container is one that has member ".resize(...)", e.g. std::vector
    // A non-resizeable container, does not have ".resize(...)", e.g. C-style arrays

    auto particle_read = file.readTableRecords<Particle>("somegroup/particleTable");
    std::cout << "Single entry read \n";
    printf("x:%g y:%g z:%g t:%g\n",particle_read.x,particle_read.y,particle_read.z,particle_read.t);


    // Or read multiple entries into a resizeable container. Start from entry 0 and read 10 entries.
    auto particles_read = file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", 0, 10);
    std::cout << "Multiple entries read \n";
    for(auto &p : particles_read) printf("x:%g y:%g z:%g t:%g\n",p.x,p.y,p.z,p.t);

    return 0;
}