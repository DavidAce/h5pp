#include <array>
#include <h5pp/h5pp.h>
#include <vector>

/*
 * h5pp supports writing and reading buffers of std::array.
 * In this example we consider writing a vector containing std::array<double,3> elements to a dataset.
 */

void longForm(h5pp::File &file) {
    // Initialize vectors with struct-type dummy data.
    std::vector<std::array<double, 3>> coords3d = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};

    // Write data. Using std::array as elements is supported by h5pp.
    file.dataset("longForm/CoordinatesInThreeDimensions").write(coords3d);
    h5pp::print("Long form wrote dataset [{}]:\n", "CoordinatesInThreeDimensions");
    h5pp::print("{}\n", coords3d);

    // Read data back from file.
    auto coords3dRead = file.dataset("longForm/CoordinatesInThreeDimensions").read<std::vector<std::array<double, 3>>>();
    h5pp::print("Long form read dataset [{}]:\n", "CoordinatesInThreeDimensions");
    h5pp::print("{}\n", coords3dRead);
}

void shortForm(h5pp::File &file) {
    // Initialize vectors with struct-type dummy data.
    std::vector<std::array<double, 3>> coords3d = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};

    // Write data using the explicit File short form.
    file.writeDataset(coords3d, "shortForm/CoordinatesInThreeDimensions");
    h5pp::print("Short form wrote dataset [{}]:\n", "CoordinatesInThreeDimensions");
    h5pp::print("{}\n", coords3d);

    // Read data back from file.
    auto coords3dRead = file.readDataset<std::vector<std::array<double, 3>>>("shortForm/CoordinatesInThreeDimensions");
    h5pp::print("Short form read dataset [{}]:\n", "CoordinatesInThreeDimensions");
    h5pp::print("{}\n", coords3dRead);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02g-stdvector-stdarray.h5", h5pp::FileAccess::REPLACE);
    longForm(file);  // Show the canonical handle-based API
    shortForm(file); // Show the compatibility short form
    return 0;
}
