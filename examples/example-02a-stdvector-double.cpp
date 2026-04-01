#include <h5pp/h5pp.h>
#include <vector>

namespace {
    h5pp::DatasetCreateOptions makeCreateOptions(size_t size) {
        h5pp::DatasetCreateOptions createOptions;
        createOptions.h5Layout    = H5D_CHUNKED;             // Use chunked layout for this dataset
        createOptions.dimsChunk   = {static_cast<hsize_t>(size)}; // Store all entries in one chunk
        return createOptions;
    }
}

void longForm(h5pp::File &file) {
    // Initialize a vector of doubles.
    std::vector<double> vWrite = {1.0, 2.0, 3.0, 4.0};
    auto                createOptions = makeCreateOptions(vWrite.size());

    // Write data using the long-form dataset handle and explicit creation options.
    file.dataset("longForm/myStdVectorDouble").ensure(createOptions).write(vWrite);
    auto vRead = file.dataset("longForm/myStdVectorDouble").read<std::vector<double>>();

    h5pp::print("Long form wrote dataset: {}\n", vWrite);
    h5pp::print("Long form read  dataset: {}\n", vRead);
}

void shortForm(h5pp::File &file) {
    // Initialize a vector of doubles.
    std::vector<double> vWrite = {1.0, 2.0, 3.0, 4.0};
    auto                createOptions = makeCreateOptions(vWrite.size());

    // Write data using the explicit File short form and the same creation options.
    file.writeDataset(vWrite, "shortForm/myStdVectorDouble", createOptions);
    auto vRead = file.readDataset<std::vector<double>>("shortForm/myStdVectorDouble");

    h5pp::print("Short form wrote dataset: {}\n", vWrite);
    h5pp::print("Short form read  dataset: {}\n", vRead);
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02a-stdvector-double.h5", h5pp::FileAccess::REPLACE);
    longForm(file);  // Show the canonical handle-based API
    shortForm(file); // Show the compatibility short form
    return 0;
}
