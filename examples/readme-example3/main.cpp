#include <h5pp/h5pp.h>

int main() {

    // Initialize a file
    h5pp::File file("myDir/someFile.h5", h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE);

    // Initialize a 10x10 Eigen matrix with random complex entries
    Eigen::MatrixXcd m1 = Eigen::MatrixXcd::Random(10, 10);

    // Write the matrix
    // Inside the file, the data will be stored in a dataset named "myEigenMatrix" under the group "myMatrixCollection"
    file.writeDataset(m1, "myMatrixCollection/myEigenMatrix");


    // Read it back in one line. Note that we pass the type as a template parameter
    auto m2 = file.readDataset<Eigen::MatrixXcd> ("myMatrixCollection/myEigenMatrix");

    return 0;
}