#include <complex>
#include <h5pp/h5pp.h>
#include <vector>

void printComplex(std::string_view message, const std::vector<std::complex<double>> &values) {
    h5pp::print("{}\n", message);
    for(const auto &value : values) h5pp::print("{} + i{}\n", std::real(value), std::imag(value));
    h5pp::print("\n");
}

void v2Api(h5pp::File &file) {
    // Initialize a vector of complex doubles.
    std::vector<std::complex<double>> vWrite(10, {3.14, -2.71});

    // Write and read data using the v2 API.
    file.dataset("v2Api/myStdVectorComplex").write(vWrite);
    auto vRead = file.dataset("v2Api/myStdVectorComplex").read<std::vector<std::complex<double>>>();

    printComplex("v2 API wrote dataset:", vWrite);
    printComplex("v2 API read  dataset:", vRead);
}

void v1Api(h5pp::File &file) {
    // Initialize a vector of complex doubles.
    std::vector<std::complex<double>> vWrite(10, {3.14, -2.71});

    // Write and read data using the v1 API.
    file.writeDataset(vWrite, "v1Api/myStdVectorComplex");
    auto vRead = file.readDataset<std::vector<std::complex<double>>>("v1Api/myStdVectorComplex");

    printComplex("v1 API wrote dataset:", vWrite);
    printComplex("v1 API read  dataset:", vRead);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02c-stdvector-complex.h5", h5pp::FileAccess::REPLACE);
    v2Api(file);  // Show the v2 API
    v1Api(file); // Show the v1 API
    return 0;
}
