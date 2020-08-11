#include <h5pp/h5pp.h>

// When interacting with tables h5pp scans the table object in file
// and populates a struct "TableInfo", which is returned to the user.
// The scanning process introduces some overhead, which is why reusing the
// info struct can be desirable to speed up repeated operations.

// This example shows how to extract metadata about tables.
// Tables are support chunking and compression by default.

// To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.
// First define typical struct.
// Note that it cannot have dynamic-size members.
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
    printf("%s", "Table info before appending records\n");
    if(tableInfo.tablePath) printf("Table path     : %s \n", tableInfo.tablePath->c_str());
    if(tableInfo.tableTitle) printf("Table title   : %s \n", tableInfo.tableTitle->c_str());
    if(tableInfo.numRecords) printf("Table records : %lu \n", tableInfo.numRecords.value());
    if(tableInfo.numFields and tableInfo.fieldNames and tableInfo.fieldSizes and tableInfo.cppTypeName)
        for(size_t idx = 0; idx < tableInfo.numFields.value(); idx++)
            printf("-- Field name [%s] | Size [%lu] bytes | Type [%s] \n", tableInfo.fieldNames.value()[idx].c_str(), tableInfo.fieldSizes.value()[idx], tableInfo.cppTypeName.value()[idx].c_str());

    // Or get a preformated string with .string()
    printf("%s\n", tableInfo.string().c_str());

    // Initialize a table in a vector
    std::vector<Stats> cityStats{Stats{"London", 9787426, 1737}, Stats{"Stockholm", 1605030, 382}, Stats{"Santiago", 5220161, 641}};

    // Write the table to file, which updates tableInfo
    tableInfo = file.appendTableEntries(cityStats, "tables/cityStats");

    // Compare the new output
    printf("%s", "Table info after appending records\n");
    if(tableInfo.tablePath) printf("Table path     : %s \n", tableInfo.tablePath->c_str());
    if(tableInfo.tableTitle) printf("Table title   : %s \n", tableInfo.tableTitle->c_str());
    if(tableInfo.numRecords) printf("Table records : %lu \n", tableInfo.numRecords.value());
    if(tableInfo.numFields and tableInfo.fieldNames and tableInfo.fieldSizes and tableInfo.cppTypeName)
        for(size_t idx = 0; idx < tableInfo.numFields.value(); idx++)
            printf("-- Field name [%s] | Size [%lu] bytes | Type [%s] \n", tableInfo.fieldNames.value()[idx].c_str(), tableInfo.fieldSizes.value()[idx], tableInfo.cppTypeName.value()[idx].c_str());

    printf("%s\n", tableInfo.string().c_str());

    return 0;
}