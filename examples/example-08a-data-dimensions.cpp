#include <h5pp/h5pp.h>
#include <iostream>

// This example shows how to specify dimensions to reinterpret
// the shape of the given data when writing it to a new dataset

/********************************************************************
   Note that the HDF5 C-API uses row-major layout!
*********************************************************************/

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-08a-data-dimensions.h5", h5pp::FilePermission::REPLACE);

    // Initialize a vector with size 12, i.e. a 1-dimensional layout "{12}"
    std::vector<double> vec(12);
    for(size_t i = 0; i < vec.size(); i++) vec[i] = static_cast<double>(i); // Populate the vector with 0,1,2,3...11

    // Let's write the data in a few different shapes
    file.writeDataset(vec, "dim12", {12}); // Writes 0,1,2,3....11

    // Write the data as a 3 x 4 matrix
    // 0  1  2  3
    // 4  5  6  7
    // 8  9  10 11
    file.writeDataset(vec, "dim3x4", {3, 4});

    // Write the data as a 3 x 2 x 2 tensor or multidimensional array
    // 0  1  |  4  5  |  8  9
    // 2  3  |  6  7  |  10 11
    file.writeDataset(vec, "dim3x2x2", {3, 2, 2});

    // One can read any shape into an std::vector
    auto std_vec = file.readDataset<std::vector<double>>("dim12");
    auto std_mat = file.readDataset<std::vector<double>>("dim3x4");
    auto std_ten = file.readDataset<std::vector<double>>("dim3x2x2");
    h5pp::print("std::vector vec {}\n", std_vec);
    h5pp::print("std::vector mat {}\n", std_mat);
    h5pp::print("std::vector ten {}\n", std_ten);

#ifdef H5PP_EIGEN3
    // Eigen comes in handy when reading multidimensional data
    // Note 1: h5pp resizes the Eigen container as indicated by the dataset dimensions
    // Note 2: The rank (number of dimensions) of the Eigen container must agree with the rank of the dataset
    // Note 3: Eigen uses column-major storage. Internally, h5pp needs to make a transposed copy to transform
    //         the data from row-major to column-major. For very large datasets this operation can be expensive.
    //         In that case consider using row-major Eigen containers, such as
    //              Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor>
    //

    auto eigen_vec = file.readDataset<Eigen::VectorXd>("dim12");
    auto eigen_mat = file.readDataset<Eigen::MatrixXd>("dim3x4");
    auto eigen_ten = file.readDataset<Eigen::Tensor<double, 3>>("dim3x2x2");

    std::cout << "Eigen vector: \n" << eigen_vec << std::endl;
    std::cout << "Eigen matrix: \n" << eigen_mat << std::endl;
    std::cout << "Eigen tensor: \n" << eigen_ten << std::endl;
#endif

    return 0;
}