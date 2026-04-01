#include <h5pp/h5pp.h>
#include <iostream>
#include <vector>

// This example shows how to specify dimensions to reinterpret
// the shape of the given data when writing it to a new dataset.

/********************************************************************
   Note that the HDF5 C API uses row-major layout.
*********************************************************************/

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-08a-data-dimensions.h5", h5pp::FileAccess::REPLACE);

    // Initialize a vector with size 12, i.e. a 1-dimensional layout "{12}".
    std::vector<double> vec(12);
    for(size_t i = 0; i < vec.size(); i++) vec[i] = static_cast<double>(i); // Populate the vector with 0,1,2,3...11

    // Let's write the data in a few different shapes.
    h5pp::DatasetWriteOptions dim12;
    dim12.dims = {12}; // Writes 0,1,2,3....11
    file.dataset("dim12").write(vec, dim12);

    // Write the data as a 3 x 4 matrix.
    // 0  1  2  3
    // 4  5  6  7
    // 8  9  10 11
    h5pp::DatasetWriteOptions dim3x4;
    dim3x4.dims = {3, 4};
    file.dataset("dim3x4").write(vec, dim3x4);

    // Write the data as a 3 x 2 x 2 tensor or multidimensional array.
    // 0  1  |  4  5  |  8  9
    // 2  3  |  6  7  |  10 11
    h5pp::DatasetWriteOptions dim3x2x2;
    dim3x2x2.dims = {3, 2, 2};
    file.dataset("dim3x2x2").write(vec, dim3x2x2);

    // One can read any shape into an std::vector.
    auto stdVec = file.dataset("dim12").read<std::vector<double>>();
    auto stdMat = file.dataset("dim3x4").read<std::vector<double>>();
    auto stdTen = file.dataset("dim3x2x2").read<std::vector<double>>();
    h5pp::print("std::vector vec {}\n", stdVec);
    h5pp::print("std::vector mat {}\n", stdMat);
    h5pp::print("std::vector ten {}\n", stdTen);

#ifdef H5PP_USE_EIGEN3
    // Eigen comes in handy when reading multidimensional data.
    // Note 1: h5pp resizes the Eigen container as indicated by the dataset dimensions.
    // Note 2: The rank (number of dimensions) of the Eigen container must agree with the rank of the dataset.
    // Note 3: Eigen uses column-major storage. Internally, h5pp needs to make a transposed copy to transform
    //         the data from row-major to column-major. For very large datasets this operation can be expensive.
    //         In that case consider using row-major Eigen containers, such as
    //              Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor>

    auto eigenVec = file.dataset("dim12").read<Eigen::VectorXd>();
    auto eigenMat = file.dataset("dim3x4").read<Eigen::MatrixXd>();
    auto eigenTen = file.dataset("dim3x2x2").read<Eigen::Tensor<double, 3>>();

    std::cout << "Eigen vector: \n" << eigenVec << std::endl;
    std::cout << "Eigen matrix: \n" << eigenMat << std::endl;
    std::cout << "Eigen tensor: \n" << eigenTen << std::endl;
#endif

    return 0;
}
