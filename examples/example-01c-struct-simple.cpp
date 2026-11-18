#include <h5pp/h5pp.h>

/*
 * h5pp supports writing and reading simple structs such as coordinate structs often found in CUDA.
 * This is currently limited to structs with 2 or 3 numeric members of equal type.
 * These types can be used as the atomic type in containers, such as std::vector<>, or written directly.
 * In this example we consider writing them directly to a dataset.
 */

struct Int2 {
    int x, y;
};

struct Double3 {
    double x, y, z;
};

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-01c-struct-simple.h5", h5pp::FileAccess::REPLACE);

    // Initialize some dummy data
    Int2    coord2dWrite = {1, 2};
    Double3 coord3dWrite = {10., 20., 30.};

    // Write and read data using the v2 API.
    file.dataset("v2Api/CoordinateInTwoDimensions").write(coord2dWrite);
    file.dataset("v2Api/CoordinateInThreeDimensions").write(coord3dWrite);
    Int2    coord2dRead = file.dataset("v2Api/CoordinateInTwoDimensions").read<Int2>();
    Double3 coord3dRead = file.dataset("v2Api/CoordinateInThreeDimensions").read<Double3>();

    h5pp::print("v2 API wrote dataset in 2D: x: {} y: {} \n", coord2dWrite.x, coord2dWrite.y);
    h5pp::print("v2 API read  dataset in 2D: x: {} y: {} \n", coord2dRead.x, coord2dRead.y);
    h5pp::print("v2 API wrote dataset in 3D: x: {} y: {} z: {} \n", coord3dWrite.x, coord3dWrite.y, coord3dWrite.z);
    h5pp::print("v2 API read  dataset in 3D: x: {} y: {} z: {} \n", coord3dRead.x, coord3dRead.y, coord3dRead.z);
    return 0;
}
