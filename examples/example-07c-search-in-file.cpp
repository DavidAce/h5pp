#include <h5pp/h5pp.h>

int main() {
    size_t logLevel = 2; // Set log level (default is 2: "info")

    // Initialize a file
    h5pp::File file("exampledir/example-07c-search-in-file.h5", h5pp::FilePermission::REPLACE, logLevel);

    // Write many dummy datasets
    file.writeDataset("0", "dataset0");
    file.writeDataset("A", "set1/datasetA");
    file.writeDataset("B", "set1/datasetB");
    file.writeDataset("C", "set1/set2/datasetC");
    file.writeDataset("D", "set1/set2/datasetD");
    file.writeDataset("E", "set1/set2/set3/datasetE");
    file.writeDataset("F", "set1/set2/set3/datasetF");

    // By default, the search starts from the root "/", returns all matches in an std::vector<std::string>
    // and searches through all groups in the file recursively.

    // Search for datasets with "A" in their name with search depth 1.
    std::string key  = "A";
    std::string root = "/"; // Search root path
    long        hits = -1;  // A negative value returns all matches. Positive stops searching after finding "hits" number of results
    long depth       = 1;   // A negative value searches the whole file recursively. Positive searches no further than "depth" number of groups
                            // below root.
    h5pp::print("Searching for datasets with key [{}] in their name. Search root [{}] | Hits [{}] | Depth [{}] \n", key, root, hits, depth);
    for(auto &res : file.findDatasets(key, root, hits, depth)) h5pp::print(" -- found: [{}]\n", res);

    // Search for datasets with "C" in their name with search depth 1. (Should find nothing)
    key   = "C";
    root  = "/";
    hits  = -1;
    depth = 1;
    h5pp::print("Searching for datasets with key [{}] in their name. Search root [{}] | Hits [{}] | Depth [{}] \n", key, root, hits, depth);
    for(auto &res : file.findDatasets(key, root, hits, depth)) h5pp::print(" -- found: [{}]\n", res);

    // Search for datasets with "C" in their name with search depth 2. (Should find set1/set2/datasetC)
    key   = "C";
    root  = "/";
    hits  = -1;
    depth = 2;
    h5pp::print("Searching for datasets with key [{}] in their name. Search root [{}] | Hits [{}] | Depth [{}] \n", key, root, hits, depth);
    for(auto &res : file.findDatasets("C", root, hits, depth)) h5pp::print(" -- found: [{}]\n", res);

    // Search for any type of object (group or dataset) with "e" in its name with root "/set1/set2" and search depth 2.
    key   = "e";
    root  = "/set1/set2";
    hits  = -1;
    depth = 2;
    h5pp::print("Searching for links with key [{}] in their name. Search root [{}] | Hits [{}] | Depth [{}] \n", key, root, hits, depth);
    for(auto &res : file.findLinks("e", root, hits, depth)) h5pp::print(" -- found: [{}]\n", res);

    // Search for the first 2 groups with "e" in their name with root "/set1" and search depth 2.
    key   = "e";
    root  = "/set1";
    hits  = 2;
    depth = 2;
    h5pp::print("Searching for groups with key [{}] in their name. Search root [{}] | Hits [{}] | Depth [{}] \n", key, root, hits, depth);
    for(auto &res : file.findGroups("e", root, hits, depth)) h5pp::print(" -- found: [{}]\n", res);
}