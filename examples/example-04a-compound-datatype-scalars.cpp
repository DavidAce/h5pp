
#include <h5pp/h5pp.h>

// In this example we want to treat a whole struct as a single writeable unit, a so-called compound data type.
// To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.

// First define a trivial "POD" struct.
struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
    int    id = 0;
    void   dummy_function(int) {} // Functions are OK
    // Se example 04b for the case of static-size array members
};

void print_particle(const Particle &p) { h5pp::print("x:{:.3f} y:{:.3f} z:{:.3f} t:{:.3f} id:{}\n", p.x, p.y, p.z, p.t, p.id); }

int main() {
    h5pp::File file("exampledir/example-04a-custom-struct-easy.h5", h5pp::FileAccess::REPLACE, 0);

    // Register the compound type
    h5pp::hid::h5t H5_PARTICLE_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
    H5Tinsert(H5_PARTICLE_TYPE, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "id", HOFFSET(Particle, id), H5T_NATIVE_INT);

    // Now we can write single particles or even containers with particles.

    // Write a single particle
    Particle particle;
    file.writeDataset(particle, "particle", H5_PARTICLE_TYPE);

    // Now read a single particle back from file
    auto particle_read = file.readDataset<Particle>("particle");
    h5pp::print("Single particle read \n");
    print_particle(particle_read);

    // Or write a container full of them! Let's put 10 particles in a vector.
    std::vector<Particle> particles(10);

    // Give each particle some dummy data
    int id = 1;
    for(auto &p : particles) {
        p.x  = id + 100;
        p.y  = id + 200;
        p.z  = id + 300;
        p.t  = id + 400;
        p.id = id++;
    }

    // Write them all at once
    file.writeDataset(particles, "particles", H5_PARTICLE_TYPE);

    // Now let's read them all back
    auto particles_read = file.readDataset<std::vector<Particle>>("particles");
    h5pp::print("Multiple particles read \n");
    for(auto &p : particles_read) print_particle(p);

    return 0;
}