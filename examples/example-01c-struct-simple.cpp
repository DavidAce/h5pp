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
    h5pp::File file("exampledir/example-01c-struct-simple.h5", h5pp::FilePermission::REPLACE);

    // Initialize some dummy data
    Int2    coord2dWrite = {1, 2};
    Double3 coord3dWrite = {10, 20, 30};

    // Write data
    file.writeDataset(coord2dWrite, "CoordinateInTwoDimensions");
    file.writeDataset(coord3dWrite, "CoordinateInThreeDimensions");

    // Allocate space for reading data
    Int2    coord2dRead;
    Double3 coord3dRead;

    // Read data
    file.readDataset(coord2dRead, "CoordinateInTwoDimensions");
    file.readDataset(coord3dRead, "CoordinateInThreeDimensions");

    // Or, read by assignment
    auto coord2dRead_alt = file.readDataset<Int2>("CoordinateInTwoDimensions");
    auto coord3dRead_alt = file.readDataset<Double3>("CoordinateInThreeDimensions");

    h5pp::print("Wrote dataset in 2D: x: {} y: {} \n", coord2dWrite.x, coord2dWrite.y);
    h5pp::print("Read  dataset in 2D: x: {} y: {} | alt x: {} y: {} \n",
                coord2dRead.x,
                coord2dRead.y,
                coord2dRead_alt.x,
                coord2dRead_alt.y);
    h5pp::print("Wrote dataset in 3D: x: {} y: {} z: {} \n", coord3dWrite.x, coord3dWrite.y, coord3dWrite.z);
    h5pp::print("Read  dataset in 3D: x: {} y: {} z: {} | alt x: {} y: {} z: {} \n",
                coord3dRead.x,
                coord3dRead.y,
                coord3dRead.z,
                coord3dRead_alt.x,
                coord3dRead_alt.y,
                coord3dRead_alt.z);

    return 0;
}