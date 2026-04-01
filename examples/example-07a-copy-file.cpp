#include <h5pp/h5pp.h>

int main() {
    size_t logLevel = 2; // Set log level (default is 2: "info")

    // Initialize a file.
    h5pp::File file(H5PP_EXAMPLE_DIR "example-07a-copy-file.h5", h5pp::FileAccess::REPLACE, logLevel);

    // Write a dummy dataset.
    file.dataset("groupA/A").write("A");

    // Print the current location.
    h5pp::print("File is currently in path: {}\n", file.getFilePath());

    // Copy the file to another path.
    [[maybe_unused]] auto copiedPath = file.copyFileTo(H5PP_EXAMPLE_DIR "subdir/example-step7-copy-file.h5", h5pp::FileAccess::REPLACE);

    // Print the current location again. The original file stays where it was.
    h5pp::print("File remains in old path: {}\n", file.getFilePath());
}
