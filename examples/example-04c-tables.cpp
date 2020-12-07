
#include <h5pp/h5pp.h>

/*
 * In this example we treat a struct as a single entry to an HDF5 table.
 * Tables can be suitable for time-series data or coordinate sets, for instance.
 * To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.
 * Note that the struct it cannot have dynamic-size members such as std::vector or std::string.
 * It is safest to stick to std::trivial and std::standard_layout (aka std::pod) structs.
 *
 */
struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
};


int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-04c-tables.h5", h5pp::FilePermission::REPLACE,0);

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

    /*
     * NOTE
     * The full signature of readTableRecords(...) is
     * readTableRecords(std::string_view tablePath, std::optional<size_t> startEntry = std::nullopt, std::optional<size_t> numEntries = std::nullopt)
     * When startEntry and/or numEntries (arguments 3 and 4) are missing, h5pp can decide them for you. The behavior is explained in this pseudocode:
     *
     * If the read buffer is resizeable:
     *      If startEntry and numEntries == std::nullopt
     *          startEntry = 0
     *          numEntries = totalRecords
     *      If startEntry == std::nullopt and numEntries <= totalRecords:
     *          startEntry = totalRecords - numEntries
     *      If startEntry = (between 0 to totalRecords-1)  and numEntries == std::nullopt:
     *          numEntries = totalRecords - startEntry
     *
     * If the read buffer is not resizeable:
     *      If startEntry and numEntries == std::nullopt
     *          startEntry = totalRecords-1
     *          numEntries = 1
     *      If startEntry == std::nullopt and numEntries <= totalRecords:
     *          startEntry = totalRecords - numEntries
     *      If startEntry = (between 0 to totalRecords-1)  and numEntries == std::nullopt:
     *          numEntries = 1

     * Note:
     * A "resizeable" container is one that has member ".resize(...)", e.g. std::vector
     * A non-resizeable container, does not have ".resize(...)", e.g. C-style arrays
     *
     */

    auto particle_read = file.readTableRecords<Particle>("somegroup/particleTable");
    h5pp::print("Single entry read:\n x:{:.3f} y:{:.3f} z:{:.3f} t:{:.3f}\n",particle_read.x,particle_read.y,particle_read.z,particle_read.t);


    // Or read multiple entries into a resizeable container. Start from entry 0 and read 10 entries.
    auto particles_read = file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", 0, 10);
    h5pp::print("Multiple entry read:\n");
    for(auto &&p : particles_read) h5pp::print("x:{:.3f} y:{:.3f} z:{:.3f} t:{:.3f}\n",p.x,p.y,p.z,p.t);

    return 0;
}