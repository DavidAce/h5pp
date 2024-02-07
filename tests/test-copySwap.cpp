
#include <complex>
#include <h5pp/h5pp.h>

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
        h5pp::type::sfinae::has_data_v<std::vector<double>> and
        "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    size_t logLevel = 0;

    h5pp::File file0;
    h5pp::print("file0: {}\n", file0.getFilePath());

    h5pp::File fileA("output/copySwapA.h5", h5pp::FileAccess::REPLACE, logLevel);
    h5pp::File fileB("output/copySwapB.h5", h5pp::FileAccess::REPLACE, logLevel);

    fileA.writeDataset("A", "groupA/A");
    fileB.writeDataset("B", "groupB/B");

    h5pp::File fileC;

    fileC = fileB;
    fileC.writeDataset("C", "groupC/C");

    h5pp::File fileD(fileC);
    fileD.writeDataset("D", "groupD/D");

    h5pp::File fileE(h5pp::File("output/copySwapE.h5", h5pp::FileAccess::REPLACE, logLevel));
    fileE.writeDataset("E", "groupE/E");

    fileD = fileB;

    return 0;
}