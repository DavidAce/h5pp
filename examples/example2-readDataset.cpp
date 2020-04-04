#include <h5pp/h5pp.h>

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example2.h5", h5pp::FilePermission::REPLACE);

    // Initialize an empty a vector of doubles
    std::vector<double> v(10,3.14);
    // Write data (see example 1)
    file.writeDataset(v,"myStdVector");

    // Read data. The vector is resized automatically by h5pp.
    file.readDataset(v, "myStdVector");

    // Alternatively, you can read by assignment
    v = file.readDataset<std::vector<double>>("myStdVector");

    return 0;
}