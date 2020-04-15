
#include <complex>
#include <h5pp/h5pp.h>
#include <iostream>

int main() {
    h5pp::File file("output/findObjects.h5", h5pp::FilePermission::REPLACE, 0);

    file.writeDataset(0.0,"dsetA");
    file.writeDataset(0.1,"group1/dsetB2");
    file.writeDataset(0.1,"group1/dsetB1");
    file.writeDataset(0.1,"group1/dsetB0");
    file.writeDataset(0.2,"group1/group2/dsetC");
    file.writeDataset(0.2,"group3/group4/group5/dsetD");
    file.writeDataset(0.2,"group3/group4/group5/dsetE");


    for(auto & item : file.findDatasets("dset"))
        std::cout << "Found: " << item << std::endl;
    return 0;
}
