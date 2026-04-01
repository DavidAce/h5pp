#include <complex>
#include <h5pp/h5pp.h>
#include <vector>

void printComplex(std::string_view message, const std::vector<std::complex<double>> &values) {
    h5pp::print("{}\n", message);
    for(const auto &value : values) h5pp::print("{} + i{}\n", std::real(value), std::imag(value));
    h5pp::print("\n");
}

void longForm(h5pp::File &file) {
    // Initialize a vector of complex doubles.
    std::vector<std::complex<double>> vWrite(10, {3.14, -2.71});

    // Write and read data using the long-form dataset handle.
    file.dataset("longForm/myStdVectorComplex").write(vWrite);
    auto vRead = file.dataset("longForm/myStdVectorComplex").read<std::vector<std::complex<double>>>();

    printComplex("Long form wrote dataset:", vWrite);
    printComplex("Long form read  dataset:", vRead);
}

void shortForm(h5pp::File &file) {
    // Initialize a vector of complex doubles.
    std::vector<std::complex<double>> vWrite(10, {3.14, -2.71});

    // Write and read data using the explicit File short form.
    file.writeDataset(vWrite, "shortForm/myStdVectorComplex");
    auto vRead = file.readDataset<std::vector<std::complex<double>>>("shortForm/myStdVectorComplex");

    printComplex("Short form wrote dataset:", vWrite);
    printComplex("Short form read  dataset:", vRead);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02c-stdvector-complex.h5", h5pp::FileAccess::REPLACE);
    longForm(file);  // Show the canonical handle-based API
    shortForm(file); // Show the compatibility short form
    return 0;
}
