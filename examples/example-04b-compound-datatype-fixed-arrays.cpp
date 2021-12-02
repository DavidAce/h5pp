#include <h5pp/h5pp.h>

// In this example we want to treat a whole struct as a single writeable unit, a so-called compound data type.
// To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.

// This time we consider the case where the struct data members are fixed size arrays.
struct SpaceTimePoint {
    std::array<double, 4> coordinates = {0, 0, 0, 0}; // Specify the array "coordinates" as rank-1 array of length 4
    char                  type[32]    = "spacetime";
    void                  dummy_function(int) {} // Functions are OK
};

void print_point(const SpaceTimePoint &p) {
    h5pp::print("x:{:.3f} y:{:.3f} z:{:.3f} t:{:.3f} type: %s\n",
                p.coordinates[0],
                p.coordinates[1],
                p.coordinates[2],
                p.coordinates[3],
                p.type);
}

int main() {
    size_t     logLevel = 2; // Default log level is 2: "info"
    h5pp::File file("exampledir/example-04b-custom-struct-advanced.h5", h5pp::FileAccess::REPLACE, logLevel);

    // We can create a custom HDF5 type for multi-dimensional arrays using H5Tarray_create. It takes the
    // rank (i.e. number of indices) and the size of each dimension in a c-style array pointer.
    // In this case we just have a 1D array.
    std::array<hsize_t, 1> dims = {4}; // "dims" describes the shape of a "coordinates" array: in this case has 1 dimension of size 4
    h5pp::hid::h5t         H5_COORD_TYPE = H5Tarray_create(H5T_NATIVE_DOUBLE, static_cast<unsigned>(dims.size()), dims.data());

    // Create a custom HDF5 type for the char array.
    // HDF5 provides a string template "H5T_C_S1", which describes a string with a single char.
    // We can copy the template and then modify the size to fit our string.
    h5pp::hid::h5t H5_CHAR_TYPE = H5Tcopy(H5T_C_S1);
    // Now set the size with H5Tset_size. Remember to add at least 1 extra char to leave space for the null terminator '\0'
    H5Tset_size(H5_CHAR_TYPE, 10);
    // Optionally set the null terminator '\0' and possibly padding.
    H5Tset_strpad(H5_CHAR_TYPE, H5T_STR_NULLTERM);

    // Register whole struct as a the compound type. It is important to register
    // the struct members in the same order they were declared
    h5pp::hid::h5t H5_POINT_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(SpaceTimePoint));
    H5Tinsert(H5_POINT_TYPE, "coordinates", HOFFSET(SpaceTimePoint, coordinates), H5_COORD_TYPE);
    H5Tinsert(H5_POINT_TYPE, "type", HOFFSET(SpaceTimePoint, type), H5_CHAR_TYPE);

    // Now we can write single points or even containers with points.

    // Write a single point to file
    SpaceTimePoint point{{1, 2, 3, 0}, "space"};
    file.writeDataset(point, "point", H5_POINT_TYPE);

    // Read a single point back from file and print
    auto point_read = file.readDataset<SpaceTimePoint>("point");
    h5pp::print("Single entry read \n");
    print_point(point_read);

    // Or write a container full of them! Let's make a vector with 10 points.
    std::vector<SpaceTimePoint> points(10);

    // Give some dummy data to each point
    int id = 1;
    for(auto &p : points) {
        p.coordinates[0] = id + 100;
        p.coordinates[1] = id + 200;
        p.coordinates[2] = id + 300;
        p.coordinates[3] = id + 400;
        id++;
    }

    // Write them all at once
    file.writeDataset(points, "points", H5_POINT_TYPE);

    // Now let's read them back into a new vector and print
    auto points_read = file.readDataset<std::vector<SpaceTimePoint>>("points");
    h5pp::print("Multiple entry read\n");
    for(auto &p : points_read) print_point(p);

    return 0;
}