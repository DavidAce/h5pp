#include <h5pp/h5pp.h>
#include <iostream>

#ifdef H5PP_USE_EIGEN3
void v2Api(h5pp::File &file) {
    // Initialize a 5x5 Eigen matrix with random complex entries.
    Eigen::MatrixXcd matrixWrite = Eigen::MatrixXcd::Random(5, 5);

    // Write the matrix using the v2 API.
    // Inside the file, the data will be stored in a dataset named "myEigenMatrix" under the group "myGroup".
    file.dataset("v2Api/myGroup/myEigenMatrix").write(matrixWrite);

    // Read it back in one line.
    auto matrixRead = file.dataset("v2Api/myGroup/myEigenMatrix").read<Eigen::MatrixXcd>();
    std::cout << "v2 API matrix:\n" << matrixRead << std::endl;
}

void v1Api(h5pp::File &file) {
    // Initialize a 5x5 Eigen matrix with random complex entries.
    Eigen::MatrixXcd matrixWrite = Eigen::MatrixXcd::Random(5, 5);

    // Write the matrix using the v1 API.
    file.writeDataset(matrixWrite, "v1Api/myGroup/myEigenMatrix");

    // Read it back in one line.
    auto matrixRead = file.readDataset<Eigen::MatrixXcd>("v1Api/myGroup/myEigenMatrix");
    std::cout << "v1 API matrix:\n" << matrixRead << std::endl;
}
#endif

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02e-eigen-matrix.h5", h5pp::FileAccess::REPLACE, 0);

#ifdef H5PP_USE_EIGEN3
    v2Api(file);  // Show the v2 API
    v1Api(file); // Show the v1 API
#endif
    return 0;
}
