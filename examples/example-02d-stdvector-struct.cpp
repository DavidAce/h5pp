#include <h5pp/h5pp.h>
#include <vector>

/*
 * h5pp supports writing and reading simple structs such as coordinate structs often found in CUDA.
 * This is currently limited to structs with 2 or 3 numeric members of equal type.
 * These types can be used as the atomic type in containers, such as std::vector<>, or written directly.
 * In this example we consider writing a vector of structs to a dataset.
 */

struct Int2 {
    int                       x, y;
    [[nodiscard]] std::string string() const { return h5pp::format("x: {} y: {}", x, y); }
};

struct Double3 {
    double                    x, y, z;
    [[nodiscard]] std::string string() const { return h5pp::format("x: {} y: {} z: {}", x, y, z); }
};

template<typename ScalarN>
void printDataset(std::string_view message, std::string_view dsetName, const std::vector<ScalarN> &dataset) {
    h5pp::print("{} [{}]:\n", message, dsetName);
    for(const auto &coord : dataset) h5pp::print("{}\n", coord.string());
}

template<typename ScalarN>
void writeThenReadLong(h5pp::File &file, const std::vector<ScalarN> &dataset, std::string_view dsetName) {
    file.dataset(dsetName).write(dataset); // Write data using the long-form dataset handle
    printDataset("Long form wrote dataset", dsetName, dataset);

    auto datasetRead = file.dataset(dsetName).read<std::vector<ScalarN>>();
    printDataset("Long form read  dataset", dsetName, datasetRead);
}

template<typename ScalarN>
void writeThenReadShort(h5pp::File &file, const std::vector<ScalarN> &dataset, std::string_view dsetName) {
    file.writeDataset(dataset, dsetName); // Write data using the explicit File short form
    printDataset("Short form wrote dataset", dsetName, dataset);

    auto datasetRead = file.readDataset<std::vector<ScalarN>>(dsetName);
    printDataset("Short form read  dataset", dsetName, datasetRead);
}

void longForm(h5pp::File &file) {
    // Initialize vectors with struct-type dummy data.
    std::vector<Int2>    coord2d = {{1, 2}, {3, 4}, {5, 6}};
    std::vector<Double3> coord3d = {{10.0, 20.0, 30.0}, {40.0, 50.0, 60.0}, {70.0, 80.0, 90.0}};

    writeThenReadLong(file, coord2d, "longForm/CoordinatesInTwoDimensions");
    writeThenReadLong(file, coord3d, "longForm/CoordinatesInThreeDimensions");
}

void shortForm(h5pp::File &file) {
    // Initialize vectors with struct-type dummy data.
    std::vector<Int2>    coord2d = {{1, 2}, {3, 4}, {5, 6}};
    std::vector<Double3> coord3d = {{10.0, 20.0, 30.0}, {40.0, 50.0, 60.0}, {70.0, 80.0, 90.0}};

    writeThenReadShort(file, coord2d, "shortForm/CoordinatesInTwoDimensions");
    writeThenReadShort(file, coord3d, "shortForm/CoordinatesInThreeDimensions");
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02d-stdvector-struct.h5", h5pp::FileAccess::REPLACE);
    longForm(file);  // Show the canonical handle-based API
    shortForm(file); // Show the compatibility short form
    return 0;
}
