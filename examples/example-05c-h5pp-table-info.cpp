#include <h5pp/h5pp.h>
#include <vector>

/*
 * This example shows how to inspect a table through the rich TableInfo returned by getInfo().
 *
 * In v2, you keep reusing the table handle itself and ask it for updated TableInfo objects after each operation.
 */
struct Stats {
    char   city[32];
    int    population;
    double area;
};

int main() {
    // Initialize a file.
    h5pp::File file(H5PP_EXAMPLE_DIR "example-05c-table-info.h5", h5pp::FileAccess::REPLACE);

    // Register a fixed-length string type for the city field.
    h5pp::hid::h5t H5_CITY_TYPE = H5Tcopy(H5T_C_S1);
    H5Tset_size(H5_CITY_TYPE, 32);             // Leave room for the null terminator
    H5Tset_strpad(H5_CITY_TYPE, H5T_STR_NULLTERM);

    // Register the compound type used by each table record.
    h5pp::hid::h5t H5_STATS_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Stats));
    H5Tinsert(H5_STATS_TYPE, "city", HOFFSET(Stats, city), H5_CITY_TYPE);
    H5Tinsert(H5_STATS_TYPE, "population", HOFFSET(Stats, population), H5T_NATIVE_INT);
    H5Tinsert(H5_STATS_TYPE, "area [km^2]", HOFFSET(Stats, area), H5T_NATIVE_DOUBLE);

    // Create an empty table and query its metadata.
    auto table     = file.table("tables/cityStats").create(H5_STATS_TYPE, "City Stats");
    auto tableInfo = table.getInfo();

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

    // Prepare a few table records.
    std::vector<Stats> cityStats = {
        Stats{"London", 9787426, 1737},
        Stats{"Stockholm", 1605030, 382},
        Stats{"Santiago", 5220161, 641},
    };

    // Append the records, then ask the same handle for a fresh metadata snapshot.
    table.appendRecords(cityStats);
    tableInfo = table.getInfo();

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

    return 0;
}
