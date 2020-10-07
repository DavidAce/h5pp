#include <h5pp/h5pp.h>

int main() {
    // Initialize a file
    h5pp::File          file("exampledir/example-02a-stdvector-double.h5", h5pp::FilePermission::REPLACE);

    // Initialize a vector of doubles.
    std::vector<double> v_write = {1.0, 2.0, 3.0, 4.0};

    // Write data
    file.writeDataset(v_write, "myStdVectorDouble");
    h5pp::print("Wrote dataset: {}\n", v_write);

    // Initialize an empty vector for reading.
    std::vector<double> v_read;

    // Read data. The vector is resized automatically by h5pp.
    file.readDataset(v_read, "myStdVectorDouble");
    h5pp::print("Read dataset: {}\n", v_read);

    // Or, read by assignment
    auto v_read_alt = file.readDataset<std::vector<double>>("myStdVectorDouble");
    h5pp::print("Read dataset alternate: {}\n", v_read_alt);
    return 0;
}