#include <h5pp/h5pp.h>
#include <vector>

/*
 * In this example we treat a struct as a single record in an HDF5 table.
 * Tables are suitable for time-series data, particle streams, and similar record-oriented data.
 *
 * To use a struct as a table record, its memory layout has to be registered with HDF5 in advance.
 * The struct should not have dynamically sized members such as std::vector or std::string.
 */
struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
};

void printParticle(const Particle &particle) {
    h5pp::print("x:{:.3f} y:{:.3f} z:{:.3f} t:{:.3f}\n", particle.x, particle.y, particle.z, particle.t);
}

int main() {
    // Initialize a file.
    h5pp::File file(H5PP_EXAMPLE_DIR "example-04d-tables.h5", h5pp::FileAccess::REPLACE, 0);

    // Register the compound type used by each table record.
    h5pp::hid::h5t H5_PARTICLE_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
    H5Tinsert(H5_PARTICLE_TYPE, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);

    // Create an empty table and keep a handle to it for subsequent operations.
    auto table = file.table("somegroup/particleTable").create(H5_PARTICLE_TYPE, "Title");

    // Append 10 particle records.
    std::vector<Particle> particles(10, {1, 2, 3, 4});
    table.appendRecords(particles);

    // Read a single record. For a non-resizeable target type, h5pp reads one record by default.
    auto particleRead = table.readRecords<Particle>();
    h5pp::print("Single record read:\n");
    printParticle(particleRead);

    // Or read multiple records into a resizeable container.
    auto particlesRead = table.readRecords<std::vector<Particle>>(0, 10);
    h5pp::print("Multiple records read:\n");
    for(const auto &particle : particlesRead) printParticle(particle);

    return 0;
}
