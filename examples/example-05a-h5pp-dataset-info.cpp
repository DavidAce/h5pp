#include <h5pp/h5pp.h>
#include <vector>

/*
 * This example shows how to inspect datasets through the rich DsetInfo returned by getInfo().
 *
 * In v2, dataset metadata is no longer the main write path.
 * Instead, you keep reusing the dataset handle itself and ask it for fresh DsetInfo snapshots when needed.
 */
int main() {
    // Initialize a file.
    h5pp::File file(H5PP_EXAMPLE_DIR "example-05a-dataset-info.h5", h5pp::FileAccess::REPLACE);

    // Write two simple datasets.
    file.dataset("group/intVector").write(std::vector<int>{1, 2, 3, 4});
    file.dataset("group/stringVector").write(std::vector<std::string>{"hello", "world"});

    // Create a chunked dataset handle that we can reuse later.
    h5pp::DatasetCreateOptions createOptions;
    createOptions.h5Layout  = H5D_CHUNKED; // Chunked layout enables resizing
    createOptions.dimsChunk = {20};        // One chunk with 20 elements

    auto doubleVector = file.dataset("group/doubleVector").ensure(createOptions);
    doubleVector.write(std::vector<double>(20, 3.14));

    // Get a DsetInfo object with information about the integer dataset.
    auto datasetInfo = file.dataset("group/intVector").getInfo();
    if(datasetInfo.dsetPath) h5pp::print("Int vector path : {}\n", datasetInfo.dsetPath.value());
    if(datasetInfo.dsetSize) h5pp::print("Int vector size : {}\n", datasetInfo.dsetSize.value());
    if(datasetInfo.dsetByte) h5pp::print("Int vector bytes: {}\n", datasetInfo.dsetByte.value());
    if(datasetInfo.dsetRank) h5pp::print("Int vector rank : {}\n", datasetInfo.dsetRank.value());
    if(datasetInfo.dsetDims) h5pp::print("Int vector dims : {}\n", datasetInfo.dsetDims.value());

    // Inspect the string dataset in the same way.
    datasetInfo = file.dataset("group/stringVector").getInfo();
    if(datasetInfo.dsetPath) h5pp::print("String vector path : {}\n", datasetInfo.dsetPath.value());
    if(datasetInfo.dsetSize) h5pp::print("String vector size : {}\n", datasetInfo.dsetSize.value());
    if(datasetInfo.dsetByte) h5pp::print("String vector bytes: {}\n", datasetInfo.dsetByte.value());
    if(datasetInfo.dsetRank) h5pp::print("String vector rank : {}\n", datasetInfo.dsetRank.value());
    if(datasetInfo.dsetDims) h5pp::print("String vector dims : {}\n", datasetInfo.dsetDims.value());

    // Compare that with the chunked dataset.
    datasetInfo = doubleVector.getInfo();
    if(datasetInfo.dsetPath) h5pp::print("Double vector path : {}\n", datasetInfo.dsetPath.value());
    if(datasetInfo.dsetSize) h5pp::print("Double vector size : {}\n", datasetInfo.dsetSize.value());
    if(datasetInfo.dsetByte) h5pp::print("Double vector bytes: {}\n", datasetInfo.dsetByte.value());
    if(datasetInfo.dsetRank) h5pp::print("Double vector rank : {}\n", datasetInfo.dsetRank.value());
    if(datasetInfo.dsetDims) h5pp::print("Double vector dims : {}\n", datasetInfo.dsetDims.value());
    if(datasetInfo.dsetChunk) h5pp::print("Double vector chunk: {}\n", datasetInfo.dsetChunk.value());

    // Resize the chunked dataset by overwriting it with a larger vector through the same handle.
    doubleVector.write(std::vector<double>(150, 2.71));

    // Then fetch a fresh DsetInfo object to inspect the updated metadata.
    datasetInfo = doubleVector.getInfo();
    h5pp::print("After resize\n");
    if(datasetInfo.dsetSize) h5pp::print("Double vector size : {}\n", datasetInfo.dsetSize.value());
    if(datasetInfo.dsetByte) h5pp::print("Double vector bytes: {}\n", datasetInfo.dsetByte.value());
    if(datasetInfo.dsetRank) h5pp::print("Double vector rank : {}\n", datasetInfo.dsetRank.value());
    if(datasetInfo.dsetDims) h5pp::print("Double vector dims : {}\n", datasetInfo.dsetDims.value());

    return 0;
}
