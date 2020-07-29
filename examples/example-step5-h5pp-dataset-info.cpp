#include <h5pp/h5pp.h>


// When reading and writing to file, h5pp scans datasets/c++ objects
// and populates a struct "DatasetInfo", which is returned to the user.
// The scanning process introduces some overhead, which is why reusing the
// info struct can be desirable to speed up repeated operations.

// This example shows how to extract metadata about datasets.
// Three different datasets are written to file and their metadata
// is obtained with file.getDatasetInfo(<dset path>).
//
// The third dataset is written with a chunked layout, meaning it can make use
// of advanced operations such as resizing and compression.
// The last part of this example reuses the info struct for a resize operation.



int main() {

    // Initialize a file
    h5pp::File file("exampledir/example-step5-dataset-info.h5", h5pp::FilePermission::REPLACE);

    // Write some datasets to file
    file.writeDataset(std::vector<int>{1,2,3,4}, "dummyGroup/intVector");
    file.writeDataset(std::vector<std::string>{"hello","world"}, "dummyGroup/stringVector");
    file.writeDataset(std::vector<double>(20,3.14), "dummyGroup/doubleVector", H5D_CHUNKED);

    // Get a struct populated with information about the dataset
    auto intVectorInfo = file.getDatasetInfo("dummyGroup/intVector");

    // Print the members of the info struct
    if(intVectorInfo.dsetPath) printf("Int vector name : %s \n", intVectorInfo.dsetPath->c_str());
    if(intVectorInfo.dsetSize) printf("Int vector size : %llu \n", intVectorInfo.dsetSize.value());
    if(intVectorInfo.dsetByte) printf("Int vector bytes: %lu \n", intVectorInfo.dsetByte.value());
    if(intVectorInfo.dsetRank) printf("Int vector rank : %d \n", intVectorInfo.dsetRank.value());
    if(intVectorInfo.dsetDims) {
        printf("Int vector dims : {");
        // Note that a single string is a scalar entry, so it has no extents and the following
        // should print nothing
        for(auto & dim: intVectorInfo.dsetDims.value()) printf("%llu,", dim);
        printf("}\n");
    }

    // And so on... OR, just use .string() to get a pre-formatted string
    printf("info:  %s\n", intVectorInfo.string().c_str());


    // Get a struct populated with information about the dataset
    auto stringVectorInfo = file.getDatasetInfo("dummyGroup/stringVector");


    // Print the members of the info struct
    if(stringVectorInfo.dsetPath) printf("String vector name : %s \n", stringVectorInfo.dsetPath->c_str());
    if(stringVectorInfo.dsetSize) printf("String vector size : %llu \n", stringVectorInfo.dsetSize.value());
    if(stringVectorInfo.dsetByte) printf("String vector bytes: %lu \n", stringVectorInfo.dsetByte.value());
    if(stringVectorInfo.dsetRank) printf("String vector rank : %d \n", stringVectorInfo.dsetRank.value());
    if(stringVectorInfo.dsetDims) {
        printf("String vector dims : {");
        // Note that a single string is a scalar entry, so it has no extents and the following
        // should print nothing
        for(auto & dim: stringVectorInfo.dsetDims.value()) printf("%llu,", dim);
        printf("}\n");
    }

    // And so on... OR, just use .string() to get a pre-formatted string
    printf("info:  %s\n", stringVectorInfo.string().c_str());



    // Compare the output this time with a chunked dataset
    // Get a struct populated with information about the dataset
    auto doubleVectorInfo = file.getDatasetInfo("dummyGroup/doubleVector");

    // Print the members of the info struct
    if(doubleVectorInfo.dsetPath) printf("Double vector name : %s \n", doubleVectorInfo.dsetPath->c_str());
    if(doubleVectorInfo.dsetSize) printf("Double vector size : %llu \n", doubleVectorInfo.dsetSize.value());
    if(doubleVectorInfo.dsetByte) printf("Double vector bytes: %lu \n", doubleVectorInfo.dsetByte.value());
    if(doubleVectorInfo.dsetRank) printf("Double vector rank : %d \n", doubleVectorInfo.dsetRank.value());
    if(doubleVectorInfo.dsetDims) {
        printf("Double vector dims : {");
        // Note that a single string is a scalar entry, so it has no extents and the following
        // should print nothing
        for(auto & dim: doubleVectorInfo.dsetDims.value()) printf("%llu,", dim);
        printf("}\n");
    }

    // And so on... OR, just use .string() to get a pre-formatted string
    printf("info before resize:  %s\n", doubleVectorInfo.string().c_str());



    // Finally we resize the chunked dataset by simply overwriting it with a larger vector,
    // while reusing the metadata struct. Note that the rank and layout are static properties
    // and cannot be changed on an existing dataset. Therefore there is no need to specify H5D_CHUNKED again.

    file.writeDataset(std::vector<double>(150,2.71), doubleVectorInfo);

    // The fields in the info struct should have been updated during the previous overwrite
    printf("info after resize:  %s\n", doubleVectorInfo.string().c_str());

    return 0;
}