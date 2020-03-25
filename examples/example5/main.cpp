
#include <h5pp/h5pp.h>
#include <iostream>

// In this example we want to treat a whole struct as a single writeable unit.
// To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.

// First invent a typical "plain-old-data" (POD) struct.
// Note that it cannot have dynamic-size members.
struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
    int    dummy = 4;
    void   testfunction(int) {} // Functions are OK
    // Se example 6 for the case of static-size array members
};

int main() {
    h5pp::File file("exampledir/example5.h5", h5pp::FilePermission::REPLACE);

    // Register the compound type
    h5pp::hid::h5t MY_HDF5_PARTICLE_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "dummy", HOFFSET(Particle, dummy), H5T_NATIVE_INT);

    // Now we can write single particles or even containers with particles.
    // For instance, let's write 10 particles in a vector.
    std::vector<Particle> particles(10);
    file.writeDataset(particles, "particles", MY_HDF5_PARTICLE_TYPE);

    // read them back
    auto particles_read = file.readDataset<std::vector<Particle>>("particles");
    // Print the contents
    for(auto &elem : particles_read) {
        std::cout << " \t x: " << elem.x << " \t y: " << elem.y << " \t z: " << elem.z << " \t t: " << elem.t << " \t dummy: " << elem.dummy << std::endl;
    }

    return 0;
}