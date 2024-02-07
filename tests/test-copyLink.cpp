#include <h5pp/h5pp.h>

// Store some dummy data to an hdf5 file

int main() {
    static_assert(
        h5pp::type::sfinae::has_data_v<std::vector<double>> and
        "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    size_t     logLevel = 0;
    h5pp::File fileA("output/copyLinkA.h5", h5pp::FileAccess::REPLACE, logLevel);

    fileA.writeDataset("A", "groupA/A");
    fileA.copyLinkToFile("groupA/A", "output/copyLinkB.h5", "groupA_from_file_A/A", h5pp::FileAccess::REPLACE);
    fileA.copyLinkFromFile("groupA_from_file_B/A", "output/copyLinkB.h5", "groupA_from_file_A/A");
}