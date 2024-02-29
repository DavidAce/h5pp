#include <h5pp/h5pp.h>

template<typename ...Args>
void print_complex(std::string_view msg, const std::vector<std::complex<double>> &v) {
    h5pp::print("{}\n",msg);
    for(const auto &c : v) h5pp::print("{} + i{}\n, ", std::real(c), std::imag(c));
    h5pp::print("\n");
}

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-02c-stdvector-complex.h5", h5pp::FileAccess::REPLACE);

    // Initialize  a vector of complex doubles.
    std::vector<std::complex<double>> v_write(10, {3.14, -2.71});
    // Write data
    file.writeDataset(v_write, "myStdVectorComplex");
    print_complex("Wrote dataset: ", v_write);

    // Read data. The vector is resized automatically by h5pp.
    std::vector<std::complex<double>> v_read;
    file.readDataset(v_read, "myStdVectorComplex");
    print_complex("Read dataset:  ", v_read);

    // Alternatively, read by assignment
    auto v_read_alt = file.readDataset<std::vector<std::complex<double>>>("myStdVectorComplex");
    print_complex("Read dataset alternate:  ", v_read_alt);

    return 0;
}