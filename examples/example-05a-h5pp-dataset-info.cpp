#include <h5pp/h5pp.h>

/*
 * This example shows how to use a DsetInfo object.
 *
 * When transferring a dataset to/from file h5pp scans its type, shape, link path
 * and many other properties. The results from a scan populates a struct of type "DsetInfo".
 *
 * The DsetInfo of a dataset can be obtained with h5pp::File::getDatasetInfo(<dataset path>),
 * but is also returned from a h5pp::File::writeDataset(...) operation.
 *
 * The scanning process introduces some overhead, which is why reusing the
 * struct can be desirable, in particular to speed up repeated operations.
 *
 * Three different datasets are written to file and some DsetInfo fields are printed.
 *
 * The third dataset is written with a chunked layout, meaning that it can make use
 * of advanced operations such as resizing and compression.
 * The last part of this example reuses the info struct for a resize operation.
 *
 */

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-05a-dataset-info.h5", h5pp::FilePermission::REPLACE);

    // Write some datasets to file. Note the H5D_CHUNKED parameter on the third dataset.
    file.writeDataset(std::vector<int>{1, 2, 3, 4}, "group/intVector");
    file.writeDataset(std::vector<std::string>{"hello", "world"}, "group/stringVector");
    file.writeDataset(std::vector<double>(20, 3.14), "group/doubleVector", H5D_CHUNKED);

    // Get a struct populated with information about the dataset
    auto dsetInfo = file.getDatasetInfo("group/intVector");

    // Print the members of the info struct
    if(dsetInfo.dsetPath) h5pp::print("Int vector path : {}\n", dsetInfo.dsetPath.value());
    if(dsetInfo.dsetSize) h5pp::print("Int vector size : {}\n", dsetInfo.dsetSize.value());
    if(dsetInfo.dsetByte) h5pp::print("Int vector bytes: {}\n", dsetInfo.dsetByte.value());
    if(dsetInfo.dsetRank) h5pp::print("Int vector rank : {}\n", dsetInfo.dsetRank.value());
    if(dsetInfo.dsetDims) h5pp::print("Int vector dims : {}\n", dsetInfo.dsetDims.value());

    // And so on... OR, just use .string()
    h5pp::print("dsetInfo.string(): {}\n", dsetInfo.string());

    // Get a struct populated with information about the dataset
    dsetInfo = file.getDatasetInfo("group/stringVector");

    // Print the members of the info struct
    if(dsetInfo.dsetPath) h5pp::print("String vector path : {}\n", dsetInfo.dsetPath.value());
    if(dsetInfo.dsetSize) h5pp::print("String vector size : {}\n", dsetInfo.dsetSize.value());
    if(dsetInfo.dsetByte) h5pp::print("String vector bytes: {}\n", dsetInfo.dsetByte.value());
    if(dsetInfo.dsetRank) h5pp::print("String vector rank : {}\n", dsetInfo.dsetRank.value());
    if(dsetInfo.dsetDims) h5pp::print("String vector dims : {}\n", dsetInfo.dsetDims.value());

    // And so on... OR, just use .string()
    h5pp::print("dsetInfo.string(): {}\n", dsetInfo.string());

    // Compare the output this time with a chunked dataset
    // Get a struct populated with information about the dataset
    dsetInfo = file.getDatasetInfo("group/doubleVector");

    // Print the members of the info struct
    if(dsetInfo.dsetPath) h5pp::print("Double vector path : {}\n", dsetInfo.dsetPath.value());
    if(dsetInfo.dsetSize) h5pp::print("Double vector size : {}\n", dsetInfo.dsetSize.value());
    if(dsetInfo.dsetByte) h5pp::print("Double vector bytes: {}\n", dsetInfo.dsetByte.value());
    if(dsetInfo.dsetRank) h5pp::print("Double vector rank : {}\n", dsetInfo.dsetRank.value());
    if(dsetInfo.dsetDims) h5pp::print("Double vector dims : {}\n", dsetInfo.dsetDims.value());

    // And so on... OR, just use .string()
    h5pp::print("dsetInfo.string(): {}\n", dsetInfo.string());

    // Finally we resize the chunked dataset by simply overwriting it with a larger vector,
    // while reusing the metadata struct. Note that the rank and layout are static properties
    // and cannot be changed on an existing dataset. Therefore there is no need to specify H5D_CHUNKED again.

    file.writeDataset(std::vector<double>(150, 2.71), dsetInfo);

    // The fields in the info struct should have been updated during the previous overwrite
    h5pp::print("After resize\ndsetInfo.string(): {}\n", dsetInfo.string());

    return 0;
}