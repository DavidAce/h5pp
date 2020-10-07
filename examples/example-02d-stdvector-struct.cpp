#include <h5pp/h5pp.h>

/*
 * h5pp supports writing and reading simple POD structs such as coordinate structs often found in CUDA.
 * These types can be used as the atomic type in containers, such as std::vector<> or written directly
 * In this example we consider writing a vector of structs to a dataset
 *
 */

struct Int2 {
    int         x, y;
    [[nodiscard]] std::string string() const { return h5pp::format("x: {} y: {}", x, y); }
};

struct Double3 {
    double      x, y, z;
    [[nodiscard]] std::string string() const { return h5pp::format("x: {} y: {} z: {}", x, y, z); }
};

template<typename ScalarN>
void writeThenRead(h5pp::File &file, const std::vector<ScalarN> &dset, std::string_view dsetName) {
    // Write data
    file.writeDataset(dset, dsetName); // Write data to file in dataset named "CoordinatesInTwoDimensions"
    h5pp::print("Wrote dataset [{}]:\n", dsetName);
    for(auto &c : dset) h5pp::print("{}\n", c.string());

    // Declare a container for reading back the dataset from file. No need to pre-allocate space in the vectors.
    // h5pp will automatically resize the vector
    std::vector<ScalarN> dsetRead;
    file.readDataset(dsetRead, dsetName); // Read data.
    h5pp::print("Read  dataset [{}]:\n", dsetName);
    for(auto &c : dsetRead) h5pp::print("{}\n", c.string());

    // Alternatively, you can read 2D data by assignment
    auto dsetRead_alt = file.readDataset<std::vector<ScalarN>>(dsetName); // Read data.
    h5pp::print("Read  dataset [{}] by assignment:\n", dsetName);
    for(auto &c : dsetRead_alt) h5pp::print("{}\n", c.string());
}

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-02d-stdvector-struct.h5", h5pp::FilePermission::REPLACE);

    // Initialize vectors with struct-type dummy data
    std::vector<Int2>    coord2d = {{1,2},{3,4},{5,6}};
    std::vector<Double3> coord3d = {{10.0,20.0,30.0},{40.0,50.0,60.0},{70.0,80.0,90.0}};

    // Read and write as in the previous examples
    writeThenRead(file, coord2d, "CoordinatesInTwoDimensions");
    writeThenRead(file, coord3d, "CoordinatesInThreeDimensions");
    return 0;
}