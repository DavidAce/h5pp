
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
    uint64_t             index;
    h5pp::varr_t<double> data;

    // This is a sentinel telling h5pp to expect a vlen type in this struct.
    // Note that "vlen_type" must be spelled exactly like this, and a single "using vlen_type" is enough,
    // even if there are multiple vlen_t members. The purpose of it is to disable any tracking of variable-length allocations
    // when reading this type of data. To disable tracking completely, use h5pp::File::vlenDisableReclaimsTracking() instead.
    using vlen_type = h5pp::varr_t<int>;
};

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-04e-tables-variable-length-arrays.h5", h5pp::FileAccess::REPLACE, 2);

    // Register the compound and the inner VLA data type
    h5pp::hid::h5t H5T_SCIENCE_TYPE  = H5Tcreate(H5T_COMPOUND, sizeof(ScienceEntry));
    H5Tinsert(H5T_SCIENCE_TYPE, "index", HOFFSET(ScienceEntry, index), H5T_NATIVE_UINT64);
    H5Tinsert(H5T_SCIENCE_TYPE, "data", HOFFSET(ScienceEntry, data), h5pp::varr_t<double>::get_h5type());

    // Create an empty table
    file.createTable(H5T_SCIENCE_TYPE, "somegroup/scienceTable", "ScienceTitle");

    // Create ragged data
    std::vector<ScienceEntry> scienceEntry = {{0, {1.0, 2.0}},
                                              {1, {3.5, 4.0, 4.5}},
                                              {2, {5.0}},
                                              {3, {6.2, 6.3, 6.4}},
                                              {4, {7.2, 7.6, 8.3, 8.55}}};


    file.appendTableRecords(scienceEntry, "somegroup/scienceTable");

    // Read back as std::vector<std::vector<ScienceHelper>>
    auto scienceReadEntry = file.readTableRecords<std::vector<ScienceEntry>>("somegroup/scienceTable", h5pp::TableSelection::ALL);

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
