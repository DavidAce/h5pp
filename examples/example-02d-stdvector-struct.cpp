#include <h5pp/h5pp.h>

/*
 * h5pp supports writing and reading simple POD structs such as coordinate structs often found in CUDA.
 * These types can be used as the atomic type in containers, such as std::vector<> or written directly
 * In this example we consider writing a vector of structs to a dataset
 *
 */

struct Int2 {
    int x, y;
};

struct Double3 {
    double x, y, z;
};

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-02d-stdvector-struct.h5", h5pp::FilePermission::REPLACE);

    std::vector<Int2>    coord2dWrite(10, {1, 2});
    std::vector<Double3> coord3dWrite(10, {10, 20, 30});

    file.writeDataset(coord2dWrite, "CoordinatesInTwoDimensions");   // Write data to file in dataset named "CoordinatesInTwoDimensions"
    file.writeDataset(coord3dWrite, "CoordinatesInThreeDimensions"); // Write data to file in dataset named "CoordinatesInThreeDimensions"

    // Declare a container for reading back data. No need to pre-allocate space in the vectors.
    std::vector<Int2>    coord2dRead;
    std::vector<Double3> coord3dRead;

    file.readDataset(coord2dRead, "CoordinatesInTwoDimensions");   // Read data. h5pp will automatically resize the vector
    file.readDataset(coord3dRead, "CoordinatesInThreeDimensions"); // Read data. h5pp will automatically resize the vector

    // Alternatively, you can read by assignment
    auto coord2dRead_alt = file.readDataset<std::vector<Int2>>("CoordinatesInTwoDimensions");      // Read data.
    auto coord3dRead_alt = file.readDataset<std::vector<Double3>>("CoordinatesInThreeDimensions"); // Read data.

    for(size_t idx = 0; idx < 10; idx++) {
        printf("index %zu\n", idx);
        printf("Wrote dataset in 2D: x: %d y: %d \n", coord2dWrite[idx].x, coord2dWrite[idx].y);
        printf("Read  dataset in 2D: x: %d y: %d | alt x: %d y: %d \n", coord2dRead[idx].x, coord2dRead[idx].y, coord2dRead_alt[idx].x, coord2dRead_alt[idx].y);
        printf("Wrote dataset in 3D: x: %f y: %f z: %f \n", coord3dWrite[idx].x, coord3dWrite[idx].y, coord3dWrite[idx].z);
        printf("Read  dataset in 3D: x: %f y: %f z: %f | alt x: %f y: %f z: %f \n",
               coord3dRead[idx].x,
               coord3dRead[idx].y,
               coord3dRead[idx].z,
               coord3dRead_alt[idx].x,
               coord3dRead_alt[idx].y,
               coord3dRead_alt[idx].z);
        printf("\n");
    }

    return 0;
}