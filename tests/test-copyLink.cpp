#include <complex>
#include <h5pp/h5pp.h>
#include <iostream>

/*! \brief Prints the content of a vector nicely */
template<typename T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
    if(!v.empty()) {
        out << "[ ";
        std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, " "));
        out << "]";
    }
    return out;
}

using namespace std::complex_literals;

// Store some dummy data to an hdf5 file

int main() {
    static_assert(
        h5pp::type::sfinae::has_data<std::vector<double>>() and
        "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    size_t     logLevel = 0;
    h5pp::File fileA("output/copyLinkA.h5", h5pp::FilePermission::REPLACE, logLevel);

    fileA.writeDataset("A", "groupA/A");
    fileA.copyLinkToFile("groupA/A", "output/copyLinkB.h5", "groupA_from_file_A/A", h5pp::FilePermission::REPLACE);
    fileA.copyLinkFromFile("groupA_from_file_B/A", "output/copyLinkB.h5", "groupA_from_file_A/A");
}