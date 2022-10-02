#include <h5pp/h5pp.h>

// In this example we want to treat a whole struct as a single writeable unit, a so-called compound data type.
// To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.

// This time we consider the case where the struct data members are variable-length arrays.
// Since the memory layout of this struct must be known at compile-time, the struct members need to be pointers to
// dynamically allocated data.

// for numeric types, h5pp provides h5pp::vlen_t<> which is a wrapper for HDF5's hvl_t with automatic memory management. It can be for
// writing variable-length elements in datasets or table fields.

struct Volcano {
    h5pp::vstr_t      name; // Name of the volcano
    h5pp::varr_t<int> year; // Year of eruption events. vlen_t is t with a wich a struct with void* p and size_t length to describe the data

    // This is a sentinel telling h5pp to expect a vlen type in this struct.
    // Note that "vlen_type" must be spelled exactly like this, and a single "using vlen_type" is enough,
    // even if there are multiple vlen_t members. The purpose of it is to disable any tracking of variable-length allocations
    // when reading this type of data. To disable tracking completely, use h5pp::File::vlenDisableReclaimsTracking() instead.
    using vlen_type = h5pp::varr_t<int>;
};

// Helper functions to print volcano events to terminal
void print_event(const Volcano &v, const std::string &msg = "") {
    if(not msg.empty()) h5pp::print("{}\n", msg);
    h5pp::print("-- {:<32}: {}\n", v.name, v.year);
}
void print_events(const std::vector<Volcano> &vs, const std::string &msg = "") {
    if(not msg.empty()) h5pp::print("{}\n", msg);
    for(const auto &v : vs) h5pp::print("-- {:<32}: {}\n", v.name, v.year);
}

int main() {
    size_t     logLevel = 2; // Default log level is 2: "info"
    h5pp::File file("exampledir/example-04c-compound-datatype-variable-length-arrays.h5", h5pp::FileAccess::REPLACE, logLevel);

    // Register the compound datatype and its members
    h5pp::hid::h5t H5_VOLCANO_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Volcano));
    H5Tinsert(H5_VOLCANO_TYPE, "name", HOFFSET(Volcano, name), h5pp::vstr_t::get_h5type());
    H5Tinsert(H5_VOLCANO_TYPE, "year", HOFFSET(Volcano, year), h5pp::varr_t<int>::get_h5type());

    // We can now write single volcano dataests ...

    Volcano volcano_single{"Mount Vesuvius", {1906, 1944}};
    print_event(volcano_single, "Writing to file:");
    file.writeDataset(volcano_single, "volcano_single", H5_VOLCANO_TYPE);

    // Or even containers of volcanos
    std::vector<Volcano> volcano_vector{{"Mount Vesuvius", {1906, 1944}},
                                        {"Mount Spurr", {1953, 1992}},
                                        {"Kelud", {1919, 1951, 1966, 1990}}};
    print_events(volcano_vector, "Writing to file:");
    file.writeDataset(volcano_vector, "volcano_vector", H5_VOLCANO_TYPE);

    // Now we can read the data back
    auto volcano_single_read = file.readDataset<Volcano>("volcano_single");
    print_event(volcano_single, "Read from file:");

    auto volcano_vector_read = file.readDataset<std::vector<Volcano>>("volcano_vector");
    print_events(volcano_vector, "Read from file:");

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