#include <h5pp/h5pp.h>

int main() {
    size_t logLevel = 2; // Set log level (default is 2: "info")

    // Initialize two files.
    h5pp::File fileA(H5PP_EXAMPLE_DIR "example-07d-copy-dset-file2file-A.h5", h5pp::FileAccess::REPLACE, logLevel);
    h5pp::File fileB(H5PP_EXAMPLE_DIR "example-07d-copy-dset-file2file-B.h5", h5pp::FileAccess::REPLACE, logLevel);

    // Write one dummy dataset to each file.
    fileA.dataset("data/in/fileA/datasetA").write("Data on file A");
    fileB.dataset("data/in/fileB/datasetB").write("Data on file B");

    // Copy a dataset from fileA into fileB.
    fileA.copyLinkToFile("data/in/fileA/datasetA", fileB.getFilePath(), "data/from/fileA/datasetA");

    // And copy a dataset from fileB into fileA.
    fileA.copyLinkFromFile("data/from/fileB/datasetB", fileB.getFilePath(), "data/in/fileB/datasetB");

    return 0;
}
