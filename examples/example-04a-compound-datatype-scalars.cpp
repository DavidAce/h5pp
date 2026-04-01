#include <h5pp/h5pp.h>
#include <vector>

// In this example we want to treat a whole struct as a single writable unit, a so-called compound data type.
// To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.

// First define a trivial "POD" struct.
struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
    int    id = 0;
    void   dummy_function(int) {} // Functions are OK
    // See example 04b for the case of fixed-size array members.
};

void printParticle(const Particle &particle) {
    h5pp::print("x:{:.3f} y:{:.3f} z:{:.3f} t:{:.3f} id:{}\n", particle.x, particle.y, particle.z, particle.t, particle.id);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-04a-compound-datatype-scalars.h5", h5pp::FileAccess::REPLACE, 0);

    // Register the compound type
    h5pp::hid::h5t H5_PARTICLE_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
    H5Tinsert(H5_PARTICLE_TYPE, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_TYPE, "id", HOFFSET(Particle, id), H5T_NATIVE_INT);

    // Tell h5pp to use the registered HDF5 type when creating the dataset.
    h5pp::DatasetCreateOptions createOptions;
    createOptions.h5Type = H5_PARTICLE_TYPE;

    // Now we can write single particles or even containers with particles.

    // Write a single particle
    Particle particle;
    file.dataset("particle").ensure(createOptions).write(particle);

    // Now read a single particle back from file
    auto particleRead = file.dataset("particle").read<Particle>();
    h5pp::print("Single particle read\n");
    printParticle(particleRead);

    // Or write a container full of them. Let's put 10 particles in a vector.
    std::vector<Particle> particles(10);

    // Give each particle some dummy data
    int id = 1;
    for(auto &entry : particles) {
        entry.x  = id + 100;
        entry.y  = id + 200;
        entry.z  = id + 300;
        entry.t  = id + 400;
        entry.id = id++;
    }

    // Write them all at once
    file.dataset("particles").ensure(createOptions).write(particles);

    // Now let's read them all back
    auto particlesRead = file.dataset("particles").read<std::vector<Particle>>();
    h5pp::print("Multiple particles read\n");
    for(const auto &entry : particlesRead) printParticle(entry);

    return 0;
}
