#include <h5pp/h5pp.h>

int main() {
    size_t logLevel = 2; // Set log level (default is 2: "info")

    // Initialize a file
    h5pp::File file("exampledir/example-07c-search-in-file.h5", h5pp::FilePermission::REPLACE, logLevel);

    // Write many dummy datasets
    file.writeDataset("0", "dset0");
    file.writeDataset("A", "level1/dsetA");
    file.writeDataset("B", "level1/dsetB");
    file.writeDataset("C", "level1/level2/dsetC");
    file.writeDataset("D", "level1/level2/dsetD");
    file.writeDataset("E", "level1/level2/level3/dsetE");
    file.writeDataset("F", "level1/level2/level3/dsetF");

    // By default, the search starts from the root "/", returns all matches in an std::vector<std::string>
    // and searches through all groups in the file recursively.

    // Search for datasets with "e" in their path with search depth 0.
    std::string key  = "e";
    std::string root = "/"; // Search root path
    long        hits = -1;  // A negative value returns all matches. Positive stops searching after finding "hits" number of results
    long depth       = 0; // A negative value searches the whole file recursively. Positive searches no further than "depth" number of group
                          // levels below root.
    h5pp::print("Searching for datasets with key [{}] in their path. Search root [{}] | Hits [{}] | Depth [{}] \n", key, root, hits, depth);
    for(auto &res : file.findDatasets(key, root, hits, depth)) h5pp::print(" -- found: [{}]\n", res);

    // Search for datasets with "C" in their path with search depth 1.
    key   = "C";
    root  = "/";
    hits  = -1;
    depth = 1;
    h5pp::print("Searching for datasets with key [{}] in their path. Search root [{}] | Hits [{}] | Depth [{}] \n", key, root, hits, depth);
    for(auto &res : file.findDatasets(key, root, hits, depth)) h5pp::print(" -- found: [{}]\n", res);

    // Search for datasets with "C" in their path with search depth 2.
    key   = "C";
    root  = "/";
    hits  = -1;
    depth = 2;
    h5pp::print("Searching for datasets with key [{}] in their path. Search root [{}] | Hits [{}] | Depth [{}] \n", key, root, hits, depth);
    for(auto &res : file.findDatasets("C", root, hits, depth)) h5pp::print(" -- found: [{}]\n", res);

    // Search for any type of object (group or dataset) with "e" in its path with root "/level1/level2" and search depth 2.
    key   = "e";
    root  = "/level1/level2";
    hits  = -1;
    depth = 2;
    h5pp::print("Searching for links with key [{}] in their path. Search root [{}] | Hits [{}] | Depth [{}] \n", key, root, hits, depth);
    for(auto &res : file.findLinks("e", root, hits, depth)) h5pp::print(" -- found: [{}]\n", res);

    // Search for the first 2 groups with "e" in their path with root "/level1" and search depth 2.
    key   = "e";
    root  = "/level1";
    hits  = 2;
    depth = 2;
    h5pp::print("Searching for groups with key [{}] in their path. Search root [{}] | Hits [{}] | Depth [{}] \n", key, root, hits, depth);
    for(auto &res : file.findGroups("e", root, hits, depth)) h5pp::print(" -- found: [{}]\n", res);
}