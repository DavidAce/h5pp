#include <h5pp/h5pp.h>
#include <iostream>

// This example shows how to write data into a portion of a dataset, a so-called "hyperslab".
// This time we will populate the h5pp::Options object to pass custom metadata information to .writeDataset(...)
// The main motivation for using h5pp::Options is that it allows fine-grained control with more explicit syntax.
// In addition, it allows reading/writing between hyperslab selections on both memory and dataset spaces.

/********************************************************************
   Note that the HDF5 C-API uses row-major layout!
*********************************************************************/

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-08d-write-hyperslab-using-options.h5", h5pp::FileAccess::REPLACE);

    // Initialize a vector with size 25 filled with zeros
    std::vector<double> data5x5(25, 0);

    // Write the data to a dataset, but interpret it as a 5x5 matrix (read about reinterpreting dimensions in example 08a)
    // 0  0  0  0  0
    // 0  0  0  0  0
    // 0  0  0  0  0
    // 0  0  0  0  0
    // 0  0  0  0  0
    file.writeDataset(data5x5, "data5x5", {5, 5});

    // In this example we would like to write a 2x2 matrix
    //
    // 1 2
    // 3 4
    //
    // into the larger 5x5 matrix, with the top left corner starting at position (1,2), so that we get:
    //
    // 0  0  0  0  0
    // 0  0  1  2  0
    // 0  0  3  4  0
    // 0  0  0  0  0
    // 0  0  0  0  0

    // Initialize the small vector with size 4 filled with 1,2,3,4 which will become our 2x2 matrix
    std::vector<double> data2x2 = {1, 2, 3, 4};

    // Now we need to select a 2x2 hyperslab in data5x5. There are three ways of doing this:
    // 1) Define a hyperslab and give it to .writeHyperslab(...). (simplest)
    // 2) Define a hyperslab in an instance of "h5pp::DsetInfo" corresponding to data5x5, and pass to .writeDataset(...) (see example 08d)
    // 3) Define a hyperslab in an instance of "h5pp::Options" and pass that to .writeDataset(...). (see example 08e)

    // Let's try 3) here:
    // NOTE: Internally h5pp populates instances of type h5pp::DsetInfo and h5pp::DataInfo with metadata about
    //       the dataset on file and the given data buffer, respectively. The h5pp::Options object let's the user
    //       override properties such as dimensions or hyperslab selection, during the process of
    //       populating h5pp::DsetInfo and h5pp::DataInfo. Most of the entries in h5pp::Options are only useful
    //       during dataset creation, but some, such as hyperslabs and dataDims can be used when updating a dataset.

    h5pp::Options options;
    options.linkPath = "data5x5"; // HDF5 path to the target dataset (Required)
    options.dataDims = {2, 2};    // Interpret the given data, i.e. data2x2 as a 2x2 matrix. Not strictly required as it can be guessed from
                                  // the shape of the hyperslab

    // The following three lines below can be replaced by options.dsetSlab = h5pp::Hyperslab({1,2},{2,2})
    options.dsetSlab         = h5pp::Hyperslab(); // options.dsetSlab is std::optional, so we initialize it first
    options.dsetSlab->offset = {1, 2};            // The starting point
    options.dsetSlab->extent = {2, 2};            // The dimensions of data2x2

    file.writeDataset(data2x2, options);

    // Print the result
    auto read5x5 = file.readDataset<std::vector<double>>("data5x5");
    h5pp::print("Read 5x5 matrix:\n");
    for(size_t row = 0; row < 5ul; row++) {
        for(size_t col = 0; col < 5; col++) {
            size_t idx = row * 5 + col;
            h5pp::print("{} ", read5x5[idx]);
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

    std::cout << "Read 5x5 Eigen matrix: \n" << file.readDataset<Eigen::MatrixXd>("data5x5") << std::endl;
#endif

    return 0;
}