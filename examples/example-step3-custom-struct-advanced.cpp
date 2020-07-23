
#include <h5pp/h5pp.h>
#include <iostream>

// This example writes and reads a custom struct to file.
// To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.

// The struct can only contain fixed-size members,
// i.e., struct byte layout has to be known at compile-time.
struct SpaceTimePoint {
    std::array<double, 4>                   coordinates = {0, 0, 0, 0}; // Specify the array "coordinates" as rank-1 array of length 4
    char                                    type[10]    = "minkowski";
    void                                    dummy_function(int) {} // Functions are OK
};

void print_point(const SpaceTimePoint &p) {
}

int main() {
    h5pp::File file("exampledir/example-step3-custom-struct-advanced.h5", h5pp::FilePermission::REPLACE, 0);

    // We can create a multi-dimensional array using H5Tarray_create. It takes the
    // rank (i.e. number of indices) and the size of each dimension in a c-style array pointer.
    // In this case we just have a 1D array.
    std::array<hsize_t, 1> dims = {4}; // "dims" is an array describing the extent of each dimension. In this case we "coordinates" has 1 dimension of size 4
    h5pp::hid::h5t H5_COORD_TYPE = H5Tarray_create(H5T_NATIVE_DOUBLE, static_cast<unsigned>(dims.size()), dims.data());

    // Create a type for the char array from the template H5T_C_S1
    // The template describes a string with a single char.
    h5pp::hid::h5t H5_CHAR_TYPE = H5Tcopy(H5T_C_S1);
    // Set the size with H5Tset_size.
    // Remember to add at least 1 extra char to leave space for the null terminator '\0'
    H5Tset_size(H5_CHAR_TYPE, 10);
    // Optionally set the null terminator '\0' and possibly padding.
    H5Tset_strpad(H5_CHAR_TYPE, H5T_STR_NULLTERM);

    // Register the compound type. It is important to register
    // the struct members in the same order they were declared
    h5pp::hid::h5t H5_POINT_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(SpaceTimePoint));
    H5Tinsert(H5_POINT_TYPE, "coordinates", HOFFSET(SpaceTimePoint, coordinates), H5_COORD_TYPE);
    H5Tinsert(H5_POINT_TYPE, "type", HOFFSET(SpaceTimePoint, type), H5_CHAR_TYPE);

    // Now we can write single points or even containers with points.

    // Write a single point
    SpaceTimePoint point;
    file.writeDataset(point, "point", H5_POINT_TYPE);

    // Or write a container full of them! Let's make a vector with 10 points.
    std::vector<SpaceTimePoint> points(10);
    int                         id = 1;
    for(auto &p : points) {
        p.coordinates[0] = id + 100;
        p.coordinates[1] = id + 200;
        p.coordinates[2] = id + 300;
        p.coordinates[3] = id + 400;
        id++;
    }

    // Write them all at once
    file.writeDataset(points, "points", H5_POINT_TYPE);

    // Now let's read them back

    // Read a single points read some specific entries
    auto point_read = file.readDataset<SpaceTimePoint>("point");
    std::cout << "Single entry read \n";
    print_point(point_read);
    printf("x:%f y: %f z: %f t: %f type: %s\n",  point_read.coordinates[0], point_read.coordinates[1], point_read.coordinates[2], point_read.coordinates[3], point_read.type);
    // ...or read all 10 points into a new vector
    auto points_read = file.readDataset<std::vector<SpaceTimePoint>>("points");
    std::cout << "Multiple entries read \n";
    for(auto &p : points_read)     printf("x:%f y: %f z: %f t: %f type: %s\n",  p.coordinates[0], p.coordinates[1], p.coordinates[2], p.coordinates[3], p.type);
    return 0;
}