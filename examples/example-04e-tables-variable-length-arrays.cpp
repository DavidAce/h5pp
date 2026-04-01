#include <h5pp/h5pp.h>
#include <vector>

/*
 * This example shows a table whose records contain variable-length arrays.
 * Such "ragged" table columns are represented with h5pp::varr_t<T>.
 */
struct ScienceEntry {
    uint64_t             index;
    h5pp::varr_t<double> data;

    // This sentinel tells h5pp to expect variable-length members in this record type.
    using vlen_type = h5pp::varr_t<int>;
};

int main() {
    // Initialize a file.
    h5pp::File file(H5PP_EXAMPLE_DIR "example-04e-tables-variable-length-arrays.h5", h5pp::FileAccess::REPLACE, 2);

    // Register the compound type and the inner variable-length array type.
    h5pp::hid::h5t H5_SCIENCE_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(ScienceEntry));
    H5Tinsert(H5_SCIENCE_TYPE, "index", HOFFSET(ScienceEntry, index), H5T_NATIVE_UINT64);
    H5Tinsert(H5_SCIENCE_TYPE, "data", HOFFSET(ScienceEntry, data), h5pp::varr_t<double>::get_h5type());

    // Create the table up front.
    auto table = file.table("somegroup/scienceTable").create(H5_SCIENCE_TYPE, "ScienceTitle");

    // Prepare a few ragged records.
    std::vector<ScienceEntry> scienceEntry = {
        {0, {1.0, 2.0}},
        {1, {3.5, 4.0, 4.5}},
        {2, {5.0}},
        {3, {6.2, 6.3, 6.4}},
        {4, {7.2, 7.6, 8.3, 8.55}},
    };

    // Append the records to the table.
    table.appendRecords(scienceEntry);

    // Read them all back.
    auto scienceReadEntry = table.readRecords<std::vector<ScienceEntry>>(h5pp::TableSelection::ALL);

    // Print the result.
    h5pp::print("Wrote and read entry:\n");
    for(const auto &entry : scienceReadEntry) h5pp::print("  index {} \t data {}\n", entry.index, entry.data);

    return 0;
}
