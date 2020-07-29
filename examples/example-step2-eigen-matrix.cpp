#include <h5pp/h5pp.h>
#include <iostream>
int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-step2-eigen.h5", h5pp::FilePermission::REPLACE, 0);

#ifdef H5PP_EIGEN3
    // Initialize a 10x10 Eigen matrix with random complex entries
    Eigen::MatrixXcd m1 = Eigen::MatrixXcd::Random(5, 5);

    // Write the matrix
    // Inside the file, the data will be stored in a dataset named "myEigenMatrix" under the group "myMatrixCollection"
    file.writeDataset(m1, "myGroup/myEigenMatrix");

    // Read it back in one line. Note that we pass the type as a template parameter
    auto m2 = file.readDataset<Eigen::MatrixXcd>("myGroup/myEigenMatrix");

    std::cout << m2 << std::endl;
#endif
    return 0;
}
