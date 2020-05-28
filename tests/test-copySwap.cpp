
#include <complex>
#include <h5pp/h5pp.h>
#include <iostream>

/*! \brief Prints the content of a vector nicely */
template<typename T> std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
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
    static_assert(h5pp::type::sfinae::has_data<std::vector<double>>() and
                  "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    size_t     logLevel = 0;
    h5pp::File fileA("output/copySwapA.h5", h5pp::FilePermission::REPLACE, logLevel);
    h5pp::File fileB("output/copySwapB.h5", h5pp::FilePermission::REPLACE, logLevel);

    fileA.writeDataset("A", "groupA/A");
    fileB.writeDataset("B", "groupB/B");

    h5pp::File fileC;

    fileC = fileB;
    fileC.writeDataset("C", "groupC/C");

    h5pp::File fileD(fileC);
    fileD.writeDataset("D", "groupD/D");

    h5pp::File fileE(h5pp::File("output/copySwapE.h5", h5pp::FilePermission::REPLACE, logLevel));
    fileE.writeDataset("E", "groupE/E");

    fileD = fileB;

    // Try copying the file
    fileA.writeAttribute("hidden_attr","sneaky_attribute","/"); // <-- unable to copy this
    fileA.copy_file("output/copySwapA_copy.h5",h5pp::FilePermission::REPLACE);
    h5pp::File fileA_copy("output/copySwapA_copy.h5", h5pp::FilePermission::READONLY, logLevel);
    auto fileA_copy_read = fileA_copy.readDataset<std::string>("groupA/A");
    if(fileA_copy_read  != "A") throw std::runtime_error(h5pp::format("Copy failed: [{}] != A",fileA_copy_read));
    // Try moving the file
    fileA.move_file("output/copySwapA_move.h5",h5pp::FilePermission::REPLACE);
    h5pp::File fileA_move("output/copySwapA_move.h5", h5pp::FilePermission::READONLY, logLevel);
    auto fileA_move_read = fileA_move.readDataset<std::string>("groupA/A");
    if(fileA_move_read  != "A") throw std::runtime_error(h5pp::format("Move failed: [{}] != A",fileA_move_read));



    return 0;
}