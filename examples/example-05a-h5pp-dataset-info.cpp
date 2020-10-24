#include <h5pp/h5pp.h>

/*
 * When reading and writing to datasets to file, h5pp scans HDF5 and C++ objects,
 * and populates a struct "DsetInfo", which is returned to the user.
 * The scanning process introduces some overhead, which is why reusing the
 * struct can be desirable to speed up repeated operations.
 *
 * This example shows how to use dataset metadata.
 * Three different datasets are written to file and their metadata
 * is obtained with file.getDatasetInfo(<dset path>).
 *
 * The third dataset is written with a chunked layout, meaning it can make use
 * of advanced operations such as resizing and compression.
 * The last part of this example reuses the info struct for a resize operation.
 */

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-05a-dataset-info.h5", h5pp::FilePermission::REPLACE);

    // Write some datasets to file
    file.writeDataset(std::vector<int>{1, 2, 3, 4}, "dummyGroup/intVector");
    file.writeDataset(std::vector<std::string>{"hello", "world"}, "dummyGroup/stringVector");
    file.writeDataset(std::vector<double>(20, 3.14), "dummyGroup/doubleVector", H5D_CHUNKED);

    // Get a struct populated with information about the dataset
    auto intVectorInfo = file.getDatasetInfo("dummyGroup/intVector");

    // Print the members of the info struct
    if(intVectorInfo.dsetPath) h5pp::print("Int vector path : {}\n", intVectorInfo.dsetPath.value());
    if(intVectorInfo.dsetSize) h5pp::print("Int vector size : {}\n", intVectorInfo.dsetSize.value());
    if(intVectorInfo.dsetByte) h5pp::print("Int vector bytes: {}\n", intVectorInfo.dsetByte.value());
    if(intVectorInfo.dsetRank) h5pp::print("Int vector rank : {}\n", intVectorInfo.dsetRank.value());
    if(intVectorInfo.dsetDims) h5pp::print("Int vector dims : {}\n", intVectorInfo.dsetDims.value());

    // And so on... OR, just use .string()
    h5pp::print("DsetInfo::string(): {}\n", intVectorInfo.string());

    // Get a struct populated with information about the dataset
    auto stringVectorInfo = file.getDatasetInfo("dummyGroup/stringVector");

    // Print the members of the info struct
    if(stringVectorInfo.dsetPath) h5pp::print("String vector path : {}\n", stringVectorInfo.dsetPath.value());
    if(stringVectorInfo.dsetSize) h5pp::print("String vector size : {}\n", stringVectorInfo.dsetSize.value());
    if(stringVectorInfo.dsetByte) h5pp::print("String vector bytes: {}\n", stringVectorInfo.dsetByte.value());
    if(stringVectorInfo.dsetRank) h5pp::print("String vector rank : {}\n", stringVectorInfo.dsetRank.value());
    if(stringVectorInfo.dsetDims) h5pp::print("String vector dims : {}\n", stringVectorInfo.dsetDims.value());

    // And so on... OR, just use .string()
    h5pp::print("DsetInfo::string(): {}\n", stringVectorInfo.string());

    // Compare the output this time with a chunked dataset
    // Get a struct populated with information about the dataset
    auto doubleVectorInfo = file.getDatasetInfo("dummyGroup/doubleVector");

    // Print the members of the info struct
    if(doubleVectorInfo.dsetPath) h5pp::print("Double vector path : {}\n", doubleVectorInfo.dsetPath.value());
    if(doubleVectorInfo.dsetSize) h5pp::print("Double vector size : {}\n", doubleVectorInfo.dsetSize.value());
    if(doubleVectorInfo.dsetByte) h5pp::print("Double vector bytes: {}\n", doubleVectorInfo.dsetByte.value());
    if(doubleVectorInfo.dsetRank) h5pp::print("Double vector rank : {}\n", doubleVectorInfo.dsetRank.value());
    if(doubleVectorInfo.dsetDims) h5pp::print("Double vector dims : {}\n", doubleVectorInfo.dsetDims.value());

    // And so on... OR, just use .string()
    h5pp::print("DsetInfo::string(): {}\n", doubleVectorInfo.string());

    // Finally we resize the chunked dataset by simply overwriting it with a larger vector,
    // while reusing the metadata struct. Note that the rank and layout are static properties
    // and cannot be changed on an existing dataset. Therefore there is no need to specify H5D_CHUNKED again.

    file.writeDataset(std::vector<double>(150, 2.71), doubleVectorInfo);

    // The fields in the info struct should have been updated during the previous overwrite
    h5pp::print("After resize\nDsetInfo::string(): {}\n", doubleVectorInfo.string());

    return 0;
}