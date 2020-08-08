#include <h5pp/h5pp.h>

int main() {
    size_t logLevel = 2; // Set log level (default is 2: "info")

    // Initialize a file
    h5pp::File file("exampledir/example-07a-copy-file.h5", h5pp::FilePermission::REPLACE, logLevel);

    // Write a dummy dataset
    file.writeDataset("A", "groupA/A");

    // Print the current location
    printf("File is currently in path: %s\n", file.getFilePath().c_str());

    // Copy the file to another path
    file.copyFileTo("exampledir/subdir/example-step7-copy-file.h5", h5pp::FilePermission::REPLACE);

    // Print the current location (should not change!)
    printf("File remains in old path: %s\n", file.getFilePath().c_str());
}