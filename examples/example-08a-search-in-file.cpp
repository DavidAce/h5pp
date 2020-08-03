#include <h5pp/h5pp.h>

int main() {
    size_t logLevel = 2; // Set log level (default is 2: "info")

    // Initialize a file
    h5pp::File file("exampledir/example-08a-search-in-file.h5", h5pp::FilePermission::REPLACE, logLevel);

    // Write many dummy datasets
    file.writeDataset("0", "dset0");
    file.writeDataset("A", "level1/dsetA");
    file.writeDataset("B", "level1/dsetB");
    file.writeDataset("C", "level1/level2/dsetC");
    file.writeDataset("D", "level1/level2/dsetD");
    file.writeDataset("E", "level1/level2/level3/dsetE");
    file.writeDataset("F", "level1/level2/level3/dsetF");

    // Search for datasets with "A" in their path.
    // By default, the search starts from the root "/", returns all matches in an std::vector<std::string>
    // and searches through all groups in the file recursively.
//    printf("Searching for datasets with key [A] in their path \n");
//    for(auto &res : file.findDatasets("A")) printf(" -- found: %s \n", res.c_str());

    // Search for datasets with "e" in their path with search depth 0.
    std::string key   = "e";
    std::string root  = "/";
    long        hits  = -1;
    long        depth = 0;
    printf("Searching for datasets with key %s in their path. Root [%s] | Hits [%ld] | Depth [%ld] \n", key.c_str(), root.c_str(), hits, depth);
    for(auto &res : file.findDatasets(key, root, hits, depth)) printf(" -- found: %s \n", res.c_str());
    // Search for datasets with "C" in their path with search depth 1.
    key   = "C";
    root  = "/"; // Search root path
    hits  = -1;  // A negative value returns all matches. Positive stops searching after finding "hits" number of results
    depth = 1;   // A negative value searches the whole file recursively. Positive searches no further than "depth" number of group levels below root.
    printf("Searching for datasets with key %s in their path. Root [%s] | Hits [%ld] | Depth [%ld] \n", key.c_str(), root.c_str(), hits, depth);
    for(auto &res : file.findDatasets(key, root, hits, depth)) printf(" -- found: %s \n", res.c_str());

    // Search for datasets with "C" in their path with search depth 2.
    key   = "C";
    root  = "/";
    hits  = -1;
    depth = 2;
    printf("Searching for datasets with key %s in their path. Root [%s] | Hits [%ld] | Depth [%ld] \n", key.c_str(), root.c_str(), hits, depth);
    for(auto &res : file.findDatasets("C", root, hits, depth)) printf(" -- found: %s \n", res.c_str());

    // Search for any type of object (group or dataset) with "e" in its path with root "/level1/level2" and search depth 2.
    key   = "e";
    root  = "/level1/level2";
    hits  = -1;
    depth = 2;
    printf("Searching for links with key %s in their path. Root [%s] | Hits [%ld] | Depth [%ld] \n", key.c_str(), root.c_str(), hits, depth);
    for(auto &res : file.findLinks("e", root, hits, depth)) printf(" -- found: %s \n", res.c_str());

    // Search for the first 2 groups with "e" in their path with root "/level1" and search depth 2.
    key   = "e";
    root  = "/level1";
    hits  = 2;
    depth = 2;
    printf("Searching for groups with key %s in their path. Root [%s] | Hits [%ld] | Depth [%ld] \n", key.c_str(), root.c_str(), hits, depth);
    for(auto &res : file.findGroups("e", root, hits, depth)) printf(" -- found: %s \n", res.c_str());
}