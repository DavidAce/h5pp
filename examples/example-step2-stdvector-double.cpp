#include <h5pp/h5pp.h>

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-step2-stdvector-double.h5", h5pp::FilePermission::REPLACE);

    // Initialize a vector of doubles.
    std::vector<double> v_write(10, 3.14);
    // Write data
    file.writeDataset(v_write, "myStdVectorDouble");

    // Print
    printf("Wrote dataset: \n");
    for(auto &d : v_write) printf("%f\n", d);

    // Read data. The vector is resized automatically by h5pp.
    std::vector<double> v_read;
    file.readDataset(v_read, "myStdVectorDouble");

    // Print
    printf("Read dataset: \n");
    for(auto &d : v_read) printf("%f\n", d);

    // Alternatively, read by assignment
    auto v_read_alt = file.readDataset<std::vector<double>>("myStdVectorDouble");

    printf("Read dataset alternate: \n");
    for(auto &d : v_read_alt) printf("%f\n", d);

    return 0;
}