#include <h5pp/h5pp.h>

/*
 * This example shows how to use a TableInfo object.
 *
 * When creating a table or transferring a table record to/from file, h5pp scans its type, shape, link path
 * and many other properties. The results from a scan populates a struct of type "TableInfo".
 *
 * The TableInfo of a dataset can be obtained with h5pp::File::getTableInfo(<table path>),
 * but is also returned from a h5pp::File::createTable(...) operation, or a
 * h5pp::File::appendTableRecords operations
 *
 * The scanning process introduces some overhead, which is why reusing the
 * struct can be desirable, in particular to speed up repeated operations.
 *
 *
 */

// First define a typical struct.
// Note that it cannot have dynamically sized members (such as std::vector or std::string)

struct Stats {
    char   city[32];
    int    population;
    double area;
};

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-05c-table-info.h5", h5pp::FilePermission::REPLACE);

    // Create a type for the char array from the template H5T_C_S1
    // The template describes a string with a single char.
    h5pp::hid::h5t H5_CITY_TYPE = H5Tcopy(H5T_C_S1);
    // Set the size with H5Tset_size.
    // Remember to add at least 1 extra char to leave space for the null terminator '\0'
    H5Tset_size(H5_CITY_TYPE, 32);
    // Optionally set the null terminator '\0' and possibly padding.
    H5Tset_strpad(H5_CITY_TYPE, H5T_STR_NULLTERM);

    // Register the compound type
    h5pp::hid::h5t H5_STATS_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Stats));
    H5Tinsert(H5_STATS_TYPE, "city", HOFFSET(Stats, city), H5_CITY_TYPE);
    H5Tinsert(H5_STATS_TYPE, "population", HOFFSET(Stats, population), H5T_NATIVE_INT);
    H5Tinsert(H5_STATS_TYPE, "area [km²]", HOFFSET(Stats, area), H5T_NATIVE_DOUBLE);

    // Create an empty table
    auto tableInfo = file.createTable(H5_STATS_TYPE, "tables/cityStats", "City Stats");

    // We now have a tableInfo object with meta information about our table
    h5pp::print("Table info before appending records\n");
    if(tableInfo.tablePath) h5pp::print("Table path    : {}\n", tableInfo.tablePath.value());
    if(tableInfo.tableTitle) h5pp::print("Table title   : {}\n", tableInfo.tableTitle.value());
    if(tableInfo.numRecords) h5pp::print("Table records : {}\n", tableInfo.numRecords.value());
    if(tableInfo.numFields and tableInfo.fieldNames and tableInfo.fieldSizes and tableInfo.cppTypeName)
        for(size_t idx = 0; idx < tableInfo.numFields.value(); idx++)
            h5pp::print("-- Field name [{}] | Size [{}] bytes | Type [{}]\n",
                        tableInfo.fieldNames.value()[idx],
                        tableInfo.fieldSizes.value()[idx],
                        tableInfo.cppTypeName.value()[idx]);

    // Or get a preformated string with .string()
    h5pp::print("TableInfo::string(): {}\n", tableInfo.string());

    // Initialize a table in a vector
    std::vector<Stats> cityStats{Stats{"London", 9787426, 1737}, Stats{"Stockholm", 1605030, 382}, Stats{"Santiago", 5220161, 641}};

    // Write the table to file, which updates tableInfo
    tableInfo = file.appendTableRecords(cityStats, "tables/cityStats");

    // Compare the new output
    h5pp::print("Table info after appending records\n");
    if(tableInfo.tablePath) h5pp::print("Table path    : {}\n", tableInfo.tablePath.value());
    if(tableInfo.tableTitle) h5pp::print("Table title   : {}\n", tableInfo.tableTitle.value());
    if(tableInfo.numRecords) h5pp::print("Table records : {}\n", tableInfo.numRecords.value());
    if(tableInfo.numFields and tableInfo.fieldNames and tableInfo.fieldSizes and tableInfo.cppTypeName)
        for(size_t idx = 0; idx < tableInfo.numFields.value(); idx++)
            h5pp::print("-- Field name [{}] | Size [{}] bytes | Type [{}]\n",
                        tableInfo.fieldNames.value()[idx],
                        tableInfo.fieldSizes.value()[idx],
                        tableInfo.cppTypeName.value()[idx]);
    h5pp::print("TableInfo::string(): {}\n", tableInfo.string());
    return 0;
}