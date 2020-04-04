
#include <h5pp/h5pp.h>
#include <iostream>

struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
    double rho[3]   = {20,3.13,102.4};
    char   name[10] = "some name"; // Can be replaced by std::string
    void   dummy_function(int) {}
};

int main() {
    h5pp::File file("output/readWriteTables.h5", h5pp::FilePermission::REPLACE, 0);

    // Create a type for the char array from the template H5T_C_S1
    // The template describes a string with a single char.
    // Set the size with H5Tset_size.
    h5pp::hid::h5t MY_HDF5_NAME_TYPE = H5Tcopy(H5T_C_S1);
    H5Tset_size(MY_HDF5_NAME_TYPE, 10);
    // Optionally set the null terminator '\0' and possibly padding.
    H5Tset_strpad(MY_HDF5_NAME_TYPE, H5T_STR_NULLTERM);

    // Specify the array "rho" as rank-1 array of length 3
    std::vector<hsize_t> dims = {3};
    h5pp::hid::h5t MY_HDF5_RHO_TYPE = H5Tarray_create(H5T_NATIVE_DOUBLE, dims.size(), dims.data());

    // Register the compound type
    h5pp::hid::h5t MY_HDF5_PARTICLE_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "name", HOFFSET(Particle, name), MY_HDF5_NAME_TYPE);
    H5Tinsert(MY_HDF5_PARTICLE_TYPE, "rho", HOFFSET(Particle, rho), MY_HDF5_RHO_TYPE);

    file.createTable(MY_HDF5_PARTICLE_TYPE, "somegroup/particleTable", "particleTable");

    // Write table entries
    std::vector<Particle> particles(10);
    file.appendTableEntries(particles, "somegroup/particleTable");

    // Read single entry
    Particle particle_read;
    file.readTableEntries(particle_read, "somegroup/particleTable");

    std::cout << "Single entry read \n"
              << " \t x: " << particle_read.x << " \t y: " << particle_read.y << " \t z: " << particle_read.z << " \t t: " << particle_read.t << " \t name: " << particle_read.name
              << " \t rho: " << particle_read.rho[0] << " " << particle_read.rho[1] << " " << particle_read.rho[2] << std::endl;
    // Read multiple entries into resizeable container
    std::vector<Particle> particles_read;
    file.readTableEntries(particles_read, "somegroup/particleTable", 0, 5);
    std::cout << "Multiple entries read \n";
    for(auto &elem : particles_read) {
        std::cout << " \t x: " << elem.x << " \t y: " << elem.y << " \t z: " << elem.z << " \t t: " << elem.t << " \t name: " << elem.name << " \t rho: " << particle_read.rho[0] << " " << particle_read.rho[1] << " " << particle_read.rho[2]
                  << std::endl;
    }

    return 0;
}