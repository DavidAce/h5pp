#include <h5pp/h5pp.h>

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-02c-stdvector-complex.h5", h5pp::FilePermission::REPLACE);

    // Initialize  a vector of complex doubles.
    std::vector<std::complex<double>> v_write(10, {3.14, -2.71});
    // Write data
    file.writeDataset(v_write, "myStdVectorComplex");
    h5pp::print("Wrote dataset: {}\n",v_write);

    // Read data. The vector is resized automatically by h5pp.
    std::vector<std::complex<double>> v_read;
    file.readDataset(v_read, "myStdVectorComplex");
    h5pp::print("Read dataset: {}\n",v_read);

    // Alternatively, read by assignment
    auto v_read_alt = file.readDataset<std::vector<std::complex<double>>>("myStdVectorComplex");
    h5pp::print("Read dataset alternate: {}\n", v_read_alt);

    return 0;
}