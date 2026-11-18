#include <complex>
#include <h5pp/h5pp.h>
#include <vector>

void printComplex(std::string_view message, const std::vector<std::complex<double>> &values) {
    h5pp::print("{}\n", message);
    for(const auto &value : values) h5pp::print("{} + i{}\n", std::real(value), std::imag(value));
    h5pp::print("\n");
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02c-stdvector-complex.h5", h5pp::FileAccess::REPLACE);

    // Initialize a vector of complex doubles.
    std::vector<std::complex<double>> vWrite(10, {3.14, -2.71});

    // Write and read data using the v2 API.
    file.dataset("v2Api/myStdVectorComplex").write(vWrite);
    auto vRead = file.dataset("v2Api/myStdVectorComplex").read<std::vector<std::complex<double>>>();

    printComplex("v2 API wrote dataset:", vWrite);
    printComplex("v2 API read  dataset:", vRead);
    return 0;
}
