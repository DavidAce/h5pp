#include <h5pp/h5pp.h>

/*
 * h5pp supports writing and reading simple POD structs such as coordinate structs often found in CUDA.
 * These types can be used as the atomic type in containers, such as std::vector<> or written directly
 * In this example we consider writing them directly to a dataset
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
    h5pp::File file("exampledir/example-step1-struct-simple.h5", h5pp::FilePermission::REPLACE);

    Int2    coord2dWrite = {1, 2};
    Double3 coord3dWrite = {10, 20, 30};

    file.writeDataset(coord2dWrite, "CoordinateInTwoDimensions");   // Write data to file in dataset named "CoordinateInTwoDimensions"
    file.writeDataset(coord3dWrite, "CoordinateInThreeDimensions"); // Write data to file in dataset named "CoordinateInThreeDimensions"

    // Allocate space for reading data
    Int2    coord2dRead;
    Double3 coord3dRead;

    file.readDataset(coord2dRead, "CoordinateInTwoDimensions");   // Read data.
    file.readDataset(coord3dRead, "CoordinateInThreeDimensions"); // Read data.

    // Alternatively, you can read by assignment
    auto coord2dRead_alt = file.readDataset<Int2>("CoordinateInTwoDimensions");      // Read data.
    auto coord3dRead_alt = file.readDataset<Double3>("CoordinateInThreeDimensions"); // Read data.

    printf("Wrote dataset in 2D: x: %d y: %d \n", coord2dWrite.x, coord2dWrite.y);
    printf("Read  dataset in 2D: x: %d y: %d | alt x: %d y: %d \n", coord2dRead.x, coord2dRead.y, coord2dRead_alt.x, coord2dRead_alt.y);
    printf("Wrote dataset in 3D: x: %f y: %f z: %f \n", coord3dWrite.x, coord3dWrite.y, coord3dWrite.z);
    printf("Read  dataset in 3D: x: %f y: %f z: %f | alt x: %f y: %f z: %f \n", coord3dRead.x, coord3dRead.y, coord3dRead.z, coord3dRead_alt.x, coord3dRead_alt.y, coord3dRead_alt.z);

    return 0;
}