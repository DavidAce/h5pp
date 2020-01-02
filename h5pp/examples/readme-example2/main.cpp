#include <h5pp/h5pp.h>

int main() {

    // Initialize a file
    h5pp::File file("myDir/someFile.h5", h5pp::AccessMode::READONLY, h5pp::CreateMode::OPEN );

    // Initialize an empty a vector of doubles
    std::vector<double> v;

    // Read data. The vector is resized automatically by h5pp.
    file.readDataset(v, "myStdVector");

    return 0;
}