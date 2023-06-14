#include <h5pp/h5pp.h>

/*
 * h5pp supports writing and reading buffers of std::array.
 * In this example we consider writing a vector containing std::array<double,3> elements to a dataset
 */


int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-02g-stdvector-stdarray.h5", h5pp::FileAccess::REPLACE);

    // Initialize vectors with struct-type dummy data
    std::vector<std::array<double,3>>    coords3d = {{1, 2, 3}, {4, 5, 6}, {7,8, 9}};

    // Write data. Using std::array as elements is supported by h5pp
    file.writeDataset(coords3d, "CoordinatesInThreeDimensions"); // Write data to file in dataset named "CoordinatesInTwoDimensions"
    h5pp::print("Wrote dataset [{}]:\n",  "CoordinatesInThreeDimensions");
    h5pp::print("{}\n", coords3d);

    // Declare a container for reading back the dataset from file. No need to pre-allocate space in the vectors.
    // h5pp will automatically resize the vector
    auto coorsd3dRead = file.readDataset<std::vector<std::array<double,3>>>("CoordinatesInThreeDimensions"); // Read data.
    h5pp::print("Read dataset [{}]:\n",  "CoordinatesInThreeDimensions");
    h5pp::print("{}\n", coorsd3dRead);

    return 0;
}