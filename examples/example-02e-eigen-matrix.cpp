#include <h5pp/h5pp.h>
#include <iostream>

#ifdef H5PP_USE_EIGEN3
void longForm(h5pp::File &file) {
    // Initialize a 5x5 Eigen matrix with random complex entries.
    Eigen::MatrixXcd matrixWrite = Eigen::MatrixXcd::Random(5, 5);

    // Write the matrix using the long-form dataset handle.
    // Inside the file, the data will be stored in a dataset named "myEigenMatrix" under the group "myGroup".
    file.dataset("longForm/myGroup/myEigenMatrix").write(matrixWrite);

    // Read it back in one line.
    auto matrixRead = file.dataset("longForm/myGroup/myEigenMatrix").read<Eigen::MatrixXcd>();
    std::cout << "Long form matrix:\n" << matrixRead << std::endl;
}

void shortForm(h5pp::File &file) {
    // Initialize a 5x5 Eigen matrix with random complex entries.
    Eigen::MatrixXcd matrixWrite = Eigen::MatrixXcd::Random(5, 5);

    // Write the matrix using the explicit File short form.
    file.writeDataset(matrixWrite, "shortForm/myGroup/myEigenMatrix");

    // Read it back in one line.
    auto matrixRead = file.readDataset<Eigen::MatrixXcd>("shortForm/myGroup/myEigenMatrix");
    std::cout << "Short form matrix:\n" << matrixRead << std::endl;
}
#endif

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02e-eigen-matrix.h5", h5pp::FileAccess::REPLACE, 0);

#ifdef H5PP_USE_EIGEN3
    longForm(file);  // Show the canonical handle-based API
    shortForm(file); // Show the compatibility short form
#endif
    return 0;
}
