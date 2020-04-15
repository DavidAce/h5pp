
#include <h5pp/h5pp.h>

int main() {

    // Initialize a file
    h5pp::File file("exampledir/example-step1-write-stdvector.h5");

    // Initialize a vector with 10 doubles
    std::vector<double> v (10, 3.14);

    // Write the vector to file.
    // Inside the file, the data will be stored in a dataset named "myStdVector"
    file.writeDataset(v, "myStdVector");
    return 0;
}