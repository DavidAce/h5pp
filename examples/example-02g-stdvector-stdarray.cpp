#include <array>
#include <h5pp/h5pp.h>
#include <vector>

/*
 * h5pp supports writing and reading buffers of std::array.
 * In this example we consider writing a vector containing std::array<double,3> elements to a dataset.
 */

void v2Api(h5pp::File &file) {
    // Initialize vectors with struct-type dummy data.
    std::vector<std::array<double, 3>> coords3d = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};

    // Write data. Using std::array as elements is supported by h5pp.
    file.dataset("v2Api/CoordinatesInThreeDimensions").write(coords3d);
    h5pp::print("v2 API wrote dataset [{}]:\n", "CoordinatesInThreeDimensions");
    h5pp::print("{}\n", coords3d);

    // Read data back from file.
    auto coords3dRead = file.dataset("v2Api/CoordinatesInThreeDimensions").read<std::vector<std::array<double, 3>>>();
    h5pp::print("v2 API read dataset [{}]:\n", "CoordinatesInThreeDimensions");
    h5pp::print("{}\n", coords3dRead);
}

void v1Api(h5pp::File &file) {
    // Initialize vectors with struct-type dummy data.
    std::vector<std::array<double, 3>> coords3d = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};

    // Write data using the v1 API.
    file.writeDataset(coords3d, "v1Api/CoordinatesInThreeDimensions");
    h5pp::print("v1 API wrote dataset [{}]:\n", "CoordinatesInThreeDimensions");
    h5pp::print("{}\n", coords3d);

    // Read data back from file.
    auto coords3dRead = file.readDataset<std::vector<std::array<double, 3>>>("v1Api/CoordinatesInThreeDimensions");
    h5pp::print("v1 API read dataset [{}]:\n", "CoordinatesInThreeDimensions");
    h5pp::print("{}\n", coords3dRead);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02g-stdvector-stdarray.h5", h5pp::FileAccess::REPLACE);
    v2Api(file);  // Show the v2 API
    v1Api(file); // Show the v1 API
    return 0;
}
