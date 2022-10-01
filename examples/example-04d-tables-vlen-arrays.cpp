
#include <h5pp/h5pp.h>

/*
 * In this example we treat a struct as a single record for an HDF5 table.
 * Tables can be suitable for time-series data or coordinate sets, for instance.
 * In this case we consider the case where a column in the table contains "ragged" data,
 * meaning that cells have varying length arrays (VLA).
 * We use the "hvl_t" struct, which describes the length of data and a pointer to it,
 * to pass data to and from HDF5.
 *
 */
struct ScienceEntry {
    uint64_t            index;
    std::vector<double> data;
};

struct ScienceHelper {
    uint64_t index;
    hvl_t    data;
};

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-04d-tables-vlen-arrays.h5", h5pp::FileAccess::REPLACE,0);

    // Register the compound and the inner VLA data type
    h5pp::hid::h5t H5T_SCIENCE_TYPE  = H5Tcreate(H5T_COMPOUND, sizeof(ScienceHelper));
    h5pp::hid::h5t H5T_VLA_DATA_TYPE = H5Tvlen_create(H5T_NATIVE_DOUBLE);
    H5Tinsert(H5T_SCIENCE_TYPE, "index", HOFFSET(ScienceHelper, index), H5T_NATIVE_UINT64);
    H5Tinsert(H5T_SCIENCE_TYPE, "data", HOFFSET(ScienceHelper, data), H5T_VLA_DATA_TYPE);

    // Create an empty table
    file.createTable(H5T_SCIENCE_TYPE, "somegroup/scienceTable", "ScienceTitle");

    // Create ragged data
    std::vector<ScienceEntry> scienceEntry = {{0, {1.0, 2.0}},
                                              {1, {3.5, 4.0, 4.5}},
                                              {2, {5.0}},
                                              {3, {6.2, 6.3, 6.4}},
                                              {4, {7.2, 7.6, 8.3, 8.55}}};

    // When writing VLA data to file, HDF5 expects the ragged data in a hvl_t struct.
    // We will use ScienceHelper to create pointers to ScienceEntry.
    // Remember, the data always lives in ScienceEntry!
    std::vector<ScienceHelper> scienceHelper;
    scienceHelper.reserve(scienceEntry.size());
    for(auto &entry : scienceEntry) scienceHelper.emplace_back(ScienceHelper{entry.index, hvl_t{entry.data.size(), entry.data.data()}});

    file.appendTableRecords(scienceHelper, "somegroup/scienceTable");

    // Read back as std::vector<std::vector<ScienceHelper>>, and free the data allocated by HDF5 when you are done with it.
//    h5pp::TableInfo tableInfo; // Keep track of this metadata, it will hold what we need to reclaim/free vla arrays later.
    auto            scienceReadHelper =
        file.readTableRecords<std::vector<ScienceHelper>>("somegroup/scienceTable", h5pp::TableSelection::ALL);
    auto scienceReadEntry = std::vector<ScienceEntry>();
    scienceReadEntry.reserve(scienceReadHelper.size());
    // Copy the data back from hvl_t
    for(auto &helper : scienceReadHelper) {
        auto data_begin = static_cast<double *>(helper.data.p);                   // Cast void* to double*
        auto data_end   = static_cast<double *>(helper.data.p) + helper.data.len; // Cast void* to double*
        scienceReadEntry.emplace_back(ScienceEntry{helper.index, std::vector<double>(data_begin, data_end)});
    }
    // Free hvl_t data! h5pp stores metadata when it detects H5T_VLEN data being read, so that we can reclaim (free) it later.
    file.vlenReclaim(); // Note that you can also reclaim from TableInfo objects

    // Print data
    h5pp::print("Wrote and read entry:\n");
    for(const auto &entry : scienceReadEntry) h5pp::print("  index {} \t data {}\n", entry.index, entry.data);

    return 0;
}

/* Output:

Wrote and read entry:
  index 0 	 data [1, 2]
  index 1 	 data [3.5, 4, 4.5]
  index 2 	 data [5]
  index 3 	 data [6.2, 6.3, 6.4]
  index 4 	 data [7.2, 7.6, 8.3, 8.55]

 */
