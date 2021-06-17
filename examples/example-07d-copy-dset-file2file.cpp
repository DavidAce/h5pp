#include <h5pp/h5pp.h>

int main() {
    size_t logLevel = 2; // Set log level (default is 2: "info")

    // Initialize two files
    h5pp::File fileA("exampledir/example-07d-copy-dset-file2file-A.h5", h5pp::FilePermission::REPLACE, logLevel);
    h5pp::File fileB("exampledir/example-07d-copy-dset-file2file-B.h5", h5pp::FilePermission::REPLACE, logLevel);

    // Write a dummy dataset to fileA
    fileA.writeDataset("Data on file A", "data/in/fileB/datasetA");
    // Write a dummy dataset to fileB
    fileB.writeDataset("Data on file B", "data/in/fileB/datasetB");


    // Copy the dataset to fileB
    fileA.copyLinkToFile("data/in/fileB/datasetA",fileB.getFilePath(),  "data/from/fileA/datasetA");

    // Alternatively, copy a dataset on fileB to fileA
    fileA.copyLinkFromFile("data/from/fileB/datasetB", fileB.getFilePath(), "data/in/fileB/datasetB");

    return 0;
}