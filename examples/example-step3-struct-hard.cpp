
#include <h5pp/h5pp.h>
#include <iostream>

// In this example we want to treat a whole struct as a single writeable unit.
// To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.

// This time its members are going to be arrays
// Note that it cannot have dynamic-size members.
struct SpaceTimePoint {
    double coordinates[4] = {0, 0, 0, 0};
    char   type[10]       = "minkowski";
    void   dummy_function(int) {} // Functions are OK
    // Se example 6 for the case of static-size array members
};

void print_point(const SpaceTimePoint &p) {
    std::cout << " \t x: " << p.coordinates[0] << " \t y: " << p.coordinates[1] << " \t z: " << p.coordinates[2] << " \t t: " << p.coordinates[3] << " \t type: " << p.type
              << std::endl;
}

int main() {
    h5pp::File file("exampledir/example-step3-struct-hard.h5", h5pp::FilePermission::REPLACE, 0);

    // Specify the array "coordinates" as rank-1 array of length 4
    std::vector<hsize_t> dims = {4};
    // We can create a multi-dimensional array using H5Tarray_create. It takes the
    // rank (i.e. number of indices) and the size of each dimension in a c-style array pointer.
    // In this case we just have a 1D array.
    h5pp::hid::h5t H5_COORD_TYPE = H5Tarray_create(H5T_NATIVE_DOUBLE, dims.size(), dims.data());

    // Create a type for the char array from the template H5T_C_S1
    // The template describes a string with a single char.
    h5pp::hid::h5t H5_CHAR_TYPE = H5Tcopy(H5T_C_S1);
    // Set the size with H5Tset_size.
    H5Tset_size(H5_CHAR_TYPE, 10);
    // Optionally set the null terminator '\0' and possibly padding.
    H5Tset_strpad(H5_CHAR_TYPE, H5T_STR_NULLTERM);

    // Register the compound type
    h5pp::hid::h5t H5_POINT_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(SpaceTimePoint));
    H5Tinsert(H5_POINT_TYPE, "coordinates", HOFFSET(SpaceTimePoint, coordinates), H5_COORD_TYPE);
    H5Tinsert(H5_POINT_TYPE, "type", HOFFSET(SpaceTimePoint, type), H5_CHAR_TYPE);

    // Now we can write single points or even containers with points.

    // Write a single point
    SpaceTimePoint point;
    file.writeDataset(point, "point", H5_POINT_TYPE);

    // Or write a container full of them! Let's write 10 points into a vector.
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

    // ...or read all 10 points into a new vector
    auto points_read = file.readDataset<std::vector<SpaceTimePoint>>("points");
    std::cout << "Multiple entries read \n";
    for(auto &p : points_read) print_point(p);

    return 0;
}