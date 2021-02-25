
#include <h5pp/h5pp.h>
#include <iostream>

struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
    int    id       = 0;
    char   name[10] = "some name"; // Can't be replaced by std::string, or anything resizeable?
    void   dummy_function(int) {}
    bool   operator==(const Particle &p) const {
        return x == p.x and y == p.y and z == p.z and t == p.t and strncmp(name, p.name, 10) == 0 and id == p.id;
    }
    bool operator!=(const Particle &p) const { return not(*this == p); }
};

void print_particle(const Particle &p) {
    std::cout << " \t x: " << p.x << " \t y: " << p.y << " \t z: " << p.z << " \t t: " << p.t << " \t id: " << p.id << "\t name: " << p.name
              << std::endl;
}

int main() {
    h5pp::File file("output/userType.h5", h5pp::FilePermission::REPLACE, 0);

    // Create a type for the char array from the template H5T_C_S1
    // The template describes a string with a single char.
    // Set the size with H5Tset_size.
    h5pp::hid::h5t MY_HDF5_NAME_TYPE = H5Tcopy(H5T_C_S1);
    H5Tset_size(MY_HDF5_NAME_TYPE, 10);
    // Optionally set the null terminator '\0' and possibly padding.
    H5Tset_strpad(MY_HDF5_NAME_TYPE, H5T_STR_NULLTERM);

    // Register the compound type
    h5pp::hid::h5t MY_HDF5_PARTICLE_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "name", HOFFSET(Particle, name), MY_HDF5_NAME_TYPE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "id", HOFFSET(Particle, id), H5T_NATIVE_INT);

    // Define a single particle
    Particle p1;
    p1.x  = 1;
    p1.y  = 2;
    p1.z  = 3;
    p1.t  = 4;
    p1.id = 5;
    strncpy(p1.name, "new name", 10);

    // Write a single particle
    file.writeDataset(p1, "singleParticle", MY_HDF5_PARTICLE_TYPE);

    // Read a single particle
    auto p1_read = file.readDataset<Particle>("singleParticle");
    print_particle(p1);
    print_particle(p1_read);
    if(p1 != p1_read) throw std::runtime_error("Single particle mismatch");

    // Define multiple particles

    std::vector<Particle> particles(10);
    int                   i = 0;
    for(auto &p : particles) {
        p.x  = i;
        p.y  = i + 10;
        p.z  = i + 100;
        p.t  = i + 1000;
        p.id = i++;
    }
    for(auto &p : particles) print_particle(p);

    file.writeDataset(particles, "particles", MY_HDF5_PARTICLE_TYPE);

    // read them back
    auto particles_read = file.readDataset<std::vector<Particle>>("particles");
    for(auto &p : particles_read) print_particle(p);

    if(particles.size() != particles_read.size()) throw std::runtime_error("Particles container size mismatch");
    i = 0;
    for(auto &p : particles)
        if(p != particles_read[(size_t) i++]) throw std::runtime_error("Particle mismatch position " + std::to_string(--i));

    // TODO: Add support for packed datatypes. The test below will not crash, but the data in the hdf5 file will be scrambled.
    // One can optionally repack the datatype to squeeze out any padding present in the struct.
    // This way the data is written in the most space-efficient way.
    h5pp::hid::h5t MY_PACKED_PARTICLE_TYPE = H5Tcopy(MY_HDF5_PARTICLE_TYPE);
    H5Tpack(MY_PACKED_PARTICLE_TYPE);
    file.writeDataset(particles, "particles_packed_TODO", MY_PACKED_PARTICLE_TYPE);

    return 0;
}