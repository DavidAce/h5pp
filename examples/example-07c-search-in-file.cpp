#include <h5pp/h5pp.h>

int main() {
    size_t logLevel = 2; // Set log level (default is 2: "info")

    // Initialize a file.
    h5pp::File file(H5PP_EXAMPLE_DIR "example-07c-search-in-file.h5", h5pp::FileAccess::REPLACE, logLevel);

    // Write many dummy datasets.
    file.dataset("dataset0").write("0");
    file.dataset("set1/datasetA").write("A");
    file.dataset("set1/datasetB").write("B");
    file.dataset("set1/set2/datasetC").write("C");
    file.dataset("set1/set2/datasetD").write("D");
    file.dataset("set1/set2/set3/datasetE").write("E");
    file.dataset("set1/set2/set3/datasetF").write("F");

    // By default, the search starts from the root "/",
    // returns all matches in an std::vector<std::string>,
    // and searches recursively through the file hierarchy.

    // Search for datasets with "A" in their name with search depth 1.
    std::string key   = "A";
    std::string root  = "/"; // Search root path
    long        hits  = -1;  // Negative means "return all matches"
    long        depth = 1;   // Negative means "search the whole file recursively"
    h5pp::print("Searching for datasets with key [{}] in their name. Search root [{}] | Hits [{}] | Depth [{}]\n",
                key,
                root,
                hits,
                depth);
    for(const auto &result : file.findDatasets(key, root, hits, depth)) h5pp::print(" -- found: [{}]\n", result);

    // Search for datasets with "C" in their name with search depth 1. This should find nothing.
    key   = "C";
    root  = "/";
    hits  = -1;
    depth = 1;
    h5pp::print("Searching for datasets with key [{}] in their name. Search root [{}] | Hits [{}] | Depth [{}]\n",
                key,
                root,
                hits,
                depth);
    for(const auto &result : file.findDatasets(key, root, hits, depth)) h5pp::print(" -- found: [{}]\n", result);

    // Search for datasets with "C" in their name with search depth 2.
    key   = "C";
    root  = "/";
    hits  = -1;
    depth = 2;
    h5pp::print("Searching for datasets with key [{}] in their name. Search root [{}] | Hits [{}] | Depth [{}]\n",
                key,
                root,
                hits,
                depth);
    for(const auto &result : file.findDatasets(key, root, hits, depth)) h5pp::print(" -- found: [{}]\n", result);

    // Search for any link type with "e" in its name from "/set1/set2" down to depth 2.
    key   = "e";
    root  = "/set1/set2";
    hits  = -1;
    depth = 2;
    h5pp::print("Searching for links with key [{}] in their name. Search root [{}] | Hits [{}] | Depth [{}]\n",
                key,
                root,
                hits,
                depth);
    for(const auto &result : file.findLinks(key, root, hits, depth)) h5pp::print(" -- found: [{}]\n", result);

    // Search for the first two groups with "e" in their name from "/set1" down to depth 2.
    key   = "e";
    root  = "/set1";
    hits  = 2;
    depth = 2;
    h5pp::print("Searching for groups with key [{}] in their name. Search root [{}] | Hits [{}] | Depth [{}]\n",
                key,
                root,
                hits,
                depth);
    for(const auto &result : file.findGroups(key, root, hits, depth)) h5pp::print(" -- found: [{}]\n", result);
}
