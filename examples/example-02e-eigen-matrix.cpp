#include <h5pp/h5pp.h>
#include <iostream>

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02e-eigen-matrix.h5", h5pp::FileAccess::REPLACE, 0);

#ifdef H5PP_USE_EIGEN3

    // Initialize a 5x5 Eigen matrix with random complex entries.
    Eigen::MatrixXcd matrixWrite = Eigen::MatrixXcd::Random(5, 5);

    // Write the matrix using the v2 API.
    // Inside the file, the data will be stored in a dataset named "myEigenMatrix" under the group "myGroup".
    file.dataset("v2Api/myGroup/myEigenMatrix").write(matrixWrite);

    // Read it back in one line.
    auto matrixRead = file.dataset("v2Api/myGroup/myEigenMatrix").read<Eigen::MatrixXcd>();
    std::cout << "v2 API matrix:\n" << matrixRead << std::endl;
#endif
    return 0;
}
