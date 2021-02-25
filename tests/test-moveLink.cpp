#include <h5pp/h5pp.h>
#include <iostream>

// Store some dummy data to an hdf5 file

int main() {
    size_t     logLevel = 0;
    h5pp::File fileA("output/moveLinkA.h5", h5pp::FilePermission::REPLACE, logLevel);
    // Same file
    fileA.writeDataset("A", "groupA/A");
    fileA.moveLinkToFile("groupA/A", "output/moveLinkA.h5", "groupA_from_file_A/A");
    fileA.moveLinkToFile("groupA_from_file_A/A", "output/moveLinkA.h5", "groupA/A");

    // Different file
    fileA.moveLinkToFile("groupA/A", "output/moveLinkB.h5", "groupA_from_file_A/A", h5pp::FilePermission::REPLACE);
    fileA.moveLinkFromFile("groupA_from_file_B/A", "output/moveLinkB.h5", "groupA_from_file_A/A");
}