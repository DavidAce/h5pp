#include <h5pp/h5pp.h>

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-02c-stdvector-complex.h5", h5pp::FilePermission::REPLACE);

    // Initialize  a vector of complex doubles.
    std::vector<std::complex<double>> v_write(10, {3.14, -2.71});
    // Write data
    file.writeDataset(v_write, "myStdVectorComplex");

    // Print
    printf("Wrote dataset: \n");
    for(auto &c : v_write) printf("%f %fi \n", c.real(), c.imag());

    // Read data. The vector is resized automatically by h5pp.
    std::vector<std::complex<double>> v_read;
    file.readDataset(v_read, "myStdVectorComplex");

    // Print
    printf("Read dataset: \n");
    for(auto &c : v_read) printf("%f %fi \n", c.real(), c.imag());

    // Alternatively, read by assignment
    auto v_read_alt = file.readDataset<std::vector<std::complex<double>>>("myStdVectorComplex");

    printf("Read dataset alternate: \n");
    for(auto &c : v_read_alt) printf("%f %fi \n", c.real(), c.imag());

    return 0;
}