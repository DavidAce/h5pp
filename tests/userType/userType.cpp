
#include <h5pp/h5pp.h>
#include <iostream>

struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
    char   name[10] = "some name"; // Can be replaced by std::string
    int    dummy    = 4;
    void   testfunction(int) {}
};

int main() {
    h5pp::File file("output/userType.h5", h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE, 0);

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
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "dummy", HOFFSET(Particle, dummy), H5T_NATIVE_INT);

    std::vector<Particle> particles(10);
    file.writeDataset(particles, MY_HDF5_PARTICLE_TYPE, "particles");

    // TODO: Add support for packed datatypes. The test below will not crash, but the data in the hdf5 file will be scrambled.
    // One can optionally repack the datatype to squeeze out any padding present in the struct.
    // This way the data is written in the most space-efficient way.
    h5pp::hid::h5t MY_PACKED_PARTICLE_TYPE = H5Tcopy(MY_HDF5_PARTICLE_TYPE);
    H5Tpack(MY_PACKED_PARTICLE_TYPE);
    file.writeDataset(particles, MY_PACKED_PARTICLE_TYPE, "particles_packed_TODO");

    // read it back
    std::vector<Particle> particles_read;
    file.readDataset(particles_read, "particles");

    for(auto &elem : particles_read) {
        std::cout << " \t x: " << elem.x << " \t y: " << elem.y << " \t z: " << elem.z << " \t t: " << elem.t << " \t name: " << elem.name << " \t dummy: " << elem.dummy
                  << std::endl;
    }

    return 0;
}