#include <h5pp/h5pp.h>
#include <vector>

namespace {
    h5pp::DatasetCreateOptions makeCreateOptions(size_t size) {
        h5pp::DatasetCreateOptions createOptions;
        createOptions.h5Layout  = H5D_CHUNKED;                  // Use chunked layout for this dataset
        createOptions.dimsChunk = {static_cast<hsize_t>(size)}; // Store all entries in one chunk
        return createOptions;
    }
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02a-stdvector-double.h5", h5pp::FileAccess::REPLACE);

    // Initialize a vector of doubles.
    std::vector<double> vWrite        = {1.0, 2.0, 3.0, 4.0};
    auto                createOptions = makeCreateOptions(vWrite.size());

    // Write data using the v2 API and explicit creation options.
    file.dataset("v2Api/myStdVectorDouble").ensure(createOptions).write(vWrite);
    auto vRead = file.dataset("v2Api/myStdVectorDouble").read<std::vector<double>>();

    h5pp::print("v2 API wrote dataset: {}\n", vWrite);
    h5pp::print("v2 API read  dataset: {}\n", vRead);
    return 0;
}
