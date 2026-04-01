#include <h5pp/h5pp.h>
#include <vector>

// In this example we want to treat a whole struct as a single writable unit, a so-called compound data type.
// To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.

// This time we consider the case where the struct data members are variable-length arrays.
// Since the memory layout of this struct must be known at compile-time, the struct members need to be wrappers
// over dynamically allocated data.

// For numeric types, h5pp provides h5pp::varr_t<> which is a wrapper for HDF5's hvl_t with automatic memory management.
// It can be used for writing variable-length elements in datasets or table fields.
struct Volcano {
    h5pp::vstr_t      name; // Name of the volcano
    h5pp::varr_t<int> year; // Year of eruption events

    // This is a sentinel telling h5pp to expect a variable-length type in this struct.
    // Note that "vlen_type" must be spelled exactly like this, and a single "using vlen_type" is enough,
    // even if there are multiple variable-length members. The purpose of it is to disable tracking of
    // variable-length allocations when reading this type of data. To disable reclaim tracking completely,
    // use file.advanced().vlenDisableReclaimsTracking().
    using vlen_type = h5pp::varr_t<int>;
};

void printEvent(const Volcano &volcano, const std::string &message = "") {
    if(not message.empty()) h5pp::print("{}\n", message);
    h5pp::print("-- {:<32}: {}\n", volcano.name, volcano.year);
}

void printEvents(const std::vector<Volcano> &volcanoes, const std::string &message = "") {
    if(not message.empty()) h5pp::print("{}\n", message);
    for(const auto &volcano : volcanoes) h5pp::print("-- {:<32}: {}\n", volcano.name, volcano.year);
}

int main() {
    size_t     logLevel = 2; // Default log level is 2: "info"
    h5pp::File file(H5PP_EXAMPLE_DIR "example-04c-compound-datatype-variable-length-arrays.h5", h5pp::FileAccess::REPLACE, logLevel);

    // Register the compound datatype and its members
    h5pp::hid::h5t H5_VOLCANO_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Volcano));
    H5Tinsert(H5_VOLCANO_TYPE, "name", HOFFSET(Volcano, name), h5pp::vstr_t::get_h5type());
    H5Tinsert(H5_VOLCANO_TYPE, "year", HOFFSET(Volcano, year), h5pp::varr_t<int>::get_h5type());

    // Tell h5pp to use the registered HDF5 type when creating the dataset.
    h5pp::DatasetCreateOptions createOptions;
    createOptions.h5Type = H5_VOLCANO_TYPE;

    // We can now write single volcano datasets ...
    Volcano volcanoSingle{"Mount Vesuvius", {1906, 1944}};
    printEvent(volcanoSingle, "Writing to file:");
    file.dataset("volcano_single").ensure(createOptions).write(volcanoSingle);

    // ... or even containers of volcanoes.
    std::vector<Volcano> volcanoVector{{"Mount Vesuvius", {1906, 1944}},
                                       {"Mount Spurr", {1953, 1992}},
                                       {"Kelud", {1919, 1951, 1966, 1990}}};
    printEvents(volcanoVector, "Writing to file:");
    file.dataset("volcano_vector").ensure(createOptions).write(volcanoVector);

    // Now we can read the data back
    auto volcanoSingleRead = file.dataset("volcano_single").read<Volcano>();
    printEvent(volcanoSingleRead, "Read from file:");

    auto volcanoVectorRead = file.dataset("volcano_vector").read<std::vector<Volcano>>();
    printEvents(volcanoVectorRead, "Read from file:");

    return 0;
}

/* Console output:

Writing to file:
-- Mount Vesuvius                  : 1906 1944
Writing to file:
-- Mount Vesuvius                  : 1906 1944
-- Mount Spurr                     : 1953 1992
-- Kelud                           : 1919 1951 1966 1990
Read from file:
-- Mount Vesuvius                  : 1906 1944
Read from file:
-- Mount Vesuvius                  : 1906 1944
-- Mount Spurr                     : 1953 1992
-- Kelud                           : 1919 1951 1966 1990

*/
