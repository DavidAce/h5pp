#include <h5pp/h5pp.h>

// In this example we want to treat a whole struct as a single writeable unit, a so-called compound data type.
// To achieve this, the memory layout of the struct has to be registered with HDF5 in advance.

// This time we consider the case where the struct data members are variable-length arrays.
// Since the memory layout of this struct must be known at compile-time, the struct members need to be pointers to
// dynamically allocated data.

struct Volcano {
    char *name; // Name of the volcano
    hvl_t year; // Year of eruption events. hvl_t is a struct with void* p and size_t length to describe the data

    // A helper function to cast years to int
    [[nodiscard]] int year_val(size_t i) const { return *(static_cast<int *>(year.p) + i); }

    // Constructors to make it easier to fill this struct with data and put it into containers liks std::vector<Volcano>
    Volcano() = default; // Must be defaulted since we have a custom destructor further down
    Volcano(const Volcano &v) {
        copy_name(v.name);
        copy_year(v.year.p, v.year.len);
    };

    Volcano(std::string_view name_, const std::vector<int> &year_) {
        copy_name(name_);
        copy_year(year_.data(), year_.size());
    }
    // Automatically deallocate when no longer needed
    ~Volcano() {
        free(name);
        free(year.p);
    }

    private:
    // Below are helper functions to copy data into this struct
    // Note that we must use C-style malloc/free here rather than C++-style new/delete, since that is what HDF5 uses internally.

    void copy_name(std::string_view name_) {
        name = static_cast<char *>(malloc((name_.size() + 1) * sizeof(char))); // Add +1 for null terminator
        strncpy(name, name_.data(), name_.size());
        name[name_.size()] = '\0'; // Make sure name is null-terminated!
    }
    void copy_year(const void *p, size_t len) {
        year.len = len;
        year.p   = static_cast<void *>(malloc(len * sizeof(int)));
        std::memcpy(year.p, p, len * sizeof(int));
    }
};

// Helper functions to print volcano events to terminal
void print_event(const Volcano &v, const std::string &msg = "") {
    if(not msg.empty()) h5pp::print("{}\n", msg);
    h5pp::print("-- {:<32}: ", v.name);
    for(size_t i = 0; i < v.year.len; i++) h5pp::print("{} ", v.year_val(i));
    h5pp::print("\n");
}
void print_events(const std::vector<Volcano> &vs, const std::string &msg = "") {
    if(not msg.empty()) h5pp::print("{}\n", msg);
    for(const auto &v : vs) print_event(v);
}

int main() {
    size_t     logLevel = 2; // Default log level is 2: "info"
    h5pp::File file("exampledir/example-04c-compound-datatype-vlen-arrays.h5", h5pp::FilePermission::REPLACE, logLevel);

    // Below we begin to define the memory layout of Volcano in a new compound HDF5 datatype.

    // Step 1:
    // We start by registering the string type for the field "name", which can be any length.

    h5pp::hid::h5t H5_VLEN_STR_TYPE = H5Tcopy(H5T_C_S1);
    // Now use H5Tset_size to set variable length
    H5Tset_size(H5_VLEN_STR_TYPE, H5T_VARIABLE);
    // Optionally set the null terminator or pad with '\0'
    H5Tset_strpad(H5_VLEN_STR_TYPE, H5T_STR_NULLTERM);
    // Optionally select a character set. Here we choose UTF8 but ASCII is also fine.
    H5Tset_cset(H5_VLEN_STR_TYPE, H5T_CSET_UTF8);

    // Step 2:
    // Next we register a variable-length array of type int
    h5pp::hid::h5t H5_VLEN_INT_TYPE = H5Tvlen_create(H5T_NATIVE_INT); // Note that H5Tvlen_create cannot be used for char/string data!

    // Step 3:
    // Finally we register the compound datatype and its members
    h5pp::hid::h5t H5_VOLCANO_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Volcano));
    H5Tinsert(H5_VOLCANO_TYPE, "name", HOFFSET(Volcano, name), H5_VLEN_STR_TYPE);
    H5Tinsert(H5_VOLCANO_TYPE, "year", HOFFSET(Volcano, year), H5_VLEN_INT_TYPE);

    // Now we can write single volcano dataests ...

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