#include <h5pp/h5pp.h>
#include <iostream>

// This example shows how to use to write data into a portion of a dataset, a so-called "hyperslab", using h5pp::Options

/********************************************************************
   Note that the HDF5 C-API uses row-major layout!
*********************************************************************/

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-08c-read-hyperslab.h5", h5pp::FilePermission::REPLACE);

    // Initialize a vector with size 25
    std::vector<double> data5x5(25);
    for(size_t i = 0; i < data5x5.size(); i++) data5x5[i] = static_cast<double>(i); // Populate the vector with 0,1,2,3...24

    // Write the data to a dataset, but interpret it as 5x5 matrix (see example 08a to read more about reinterpreting dimensions)
    // 0  1  2  3  4
    // 5  6  7  8  9
    // 10 11 12 13 14
    // 15 16 17 18 19
    // 20 21 22 23 24
    file.writeDataset(data5x5, "data5x5", {5, 5});

    // In this example we would like to read a small portion of data5x5, a 2x4 matrix with top corner at position (3,1)
    // 16 17 18 19
    // 21 22 23 24

    // The 4 lines below can be replaced by auto read2x4 = file.readHyperslab<std::vector<double>>("data5x5", h5pp::Hyperslab({3,1},{2,4}));
    auto hyperslab   = h5pp::Hyperslab();
    hyperslab.offset = {3, 1};
    hyperslab.extent = {2, 4};
    auto read2x4     = file.readHyperslab<std::vector<double>>("data5x5", hyperslab);

    // Print the result
    h5pp::print("Read 2x4 matrix:\n");
    for(size_t row = 0; row < 2ul; row++) {
        for(size_t col = 0; col < 4ul; col++) {
            size_t idx = row * 2 + col;
            h5pp::print("{} ", read2x4[idx]);
        }
        h5pp::print("\n");
    }

#ifdef H5PP_EIGEN3
        // Eigen comes in handy when dealing with matrices
        // Note 1: h5pp resizes the Eigen container as indicated by the dataset dimensions
        // Note 2: The rank (number of dimensions) of the Eigen container must agree with the rank of the dataset
        // Note 3: Eigen uses column-major storage. Internally, h5pp needs to make a transposed copy to transform
        //         the data from row-major to column-major. For very large datasets this operation can be expensive.
        //         In that case consider using row-major Eigen containers, such as
        //              Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor>
        //

    std::cout << "Read 2x4 Eigen matrix: \n" << file.readHyperslab<Eigen::MatrixXd>("data5x5",h5pp::Hyperslab({3,1},{2,4})) << std::endl;
#endif

    return 0;
}