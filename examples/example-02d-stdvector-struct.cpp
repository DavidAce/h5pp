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
void writeThenRead(h5pp::File &file, const std::vector<ScalarN> &dataset, std::string_view dsetName) {
    file.dataset(dsetName).write(dataset); // Write data using the v2 API
    printDataset("v2 API wrote dataset", dsetName, dataset);

    auto datasetRead = file.dataset(dsetName).read<std::vector<ScalarN>>();
    printDataset("v2 API read  dataset", dsetName, datasetRead);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02d-stdvector-struct.h5", h5pp::FileAccess::REPLACE);

    // Initialize vectors with struct-type dummy data.
    std::vector<Int2> coord2d = {
        {1, 2},
        {3, 4},
        {5, 6}
    };
    std::vector<Double3> coord3d = {
        {10.0, 20.0, 30.0},
        {40.0, 50.0, 60.0},
        {70.0, 80.0, 90.0}
    };

    writeThenRead(file, coord2d, "v2Api/CoordinatesInTwoDimensions");
    writeThenRead(file, coord3d, "v2Api/CoordinatesInThreeDimensions");
    return 0;
}
