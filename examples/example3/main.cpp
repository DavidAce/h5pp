#include <h5pp/h5pp.h>

int main() {
#if __has_include(<Eigen/Core>)
    // Initialize a file
    h5pp::File file("exampledir/example3.h5", h5pp::FilePermission::REPLACE);

    // Initialize a 10x10 Eigen matrix with random complex entries
    Eigen::MatrixXcd m1 = Eigen::MatrixXcd::Random(5, 5);

    // Write the matrix
    // Inside the file, the data will be stored in a dataset named "myEigenMatrix" under the group "myMatrixCollection"
    file.writeDataset(m1, "myGroup/myEigenMatrix");

    // Read it back in one line. Note that we pass the type as a template parameter
    auto m2 = file.readDataset<Eigen::MatrixXcd>("myGroup/myEigenMatrix");

    std::cout << m2 << std::endl;
#else
    std::cout << "Example 3 skipped: Eigen 3 is disabled" << std::endl;
#endif
    return 0;
}