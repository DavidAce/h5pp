#if defined(__GNUC__) || defined(__clang__)
    #define PACK(__Declaration__) __Declaration__ __attribute__((packed, aligned(1)))
#elif defined(_MSC_VER)
    #define PACK(__Declaration__) __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#else
    #define PACK(__Declaration__) __Declaration__
#endif

#include <catch2/catch_all.hpp>
#include <complex>
#include <h5pp/h5pp.h>

struct Particle {
    double               x = 0, y = 1, z = 2, t = 3;
    double               rho[3]   = {20, 3.13, 102.4};
    char                 name[10] = "some name";
    std::complex<double> cplx     = {1, 1};
    void                 dummy_function(int) {}
};

struct Rho {
    double rho[3];
};
struct Complex {
    std::complex<double> cplx;
};
struct Axis {
    double axis = 0;
};
struct Coords {
    double x = 0, y = 0, z = 0, t = 0;
};

PACK(struct RhoName {
    double rho[3];
    char   name[10];
});

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }

    h5pp::hid::h5t make_particle_type() {
        h5pp::hid::h5t name_type = H5Tcopy(H5T_C_S1);
        H5Tset_size(name_type, 10);
        H5Tset_strpad(name_type, H5T_STR_NULLTERM);

        std::vector<hsize_t> dims     = {3};
        h5pp::hid::h5t       rho_type = H5Tarray_create(H5T_NATIVE_DOUBLE, h5pp::type::safe_cast<unsigned int>(dims.size()), dims.data());

        h5pp::hid::h5t particle_type = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
        H5Tinsert(particle_type, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
        H5Tinsert(particle_type, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
        H5Tinsert(particle_type, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
        H5Tinsert(particle_type, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);
        H5Tinsert(particle_type, "rho", HOFFSET(Particle, rho), rho_type);
        H5Tinsert(particle_type, "name", HOFFSET(Particle, name), name_type);
        H5Tinsert(particle_type, "cplx", HOFFSET(Particle, cplx), h5pp::type::compound::H5T_COMPLEX<double>::h5type());
        return particle_type;
    }

    std::string make_table_file() {
        auto       path = make_path("readWriteTableFields");
        h5pp::File file(path, h5pp::FileAccess::REPLACE, 0);
        auto       type = make_particle_type();
        h5pp::TableCreateOptions create;
        create.dimsChunk   = std::vector<hsize_t>{5};
        create.compression = 1;
        auto table = file.table("somegroup/particleTable").create(type, "particleTable", create);
        table.appendRecords(std::vector<Particle>(10));
        return path;
    }
}

TEST_CASE("Single table fields can be addressed through all supported selector overloads", "[table-fields]") {
    h5pp::File          file(make_table_file(), h5pp::FileAccess::READWRITE, 0);
    auto                table = file.table("somegroup/particleTable");
    std::vector<Axis> axis_fields;

    axis_fields.emplace_back(table.readField<Axis>("y", h5pp::TableSelection::FIRST));
    axis_fields.emplace_back(table.readField<Axis>(std::string("y"), h5pp::TableSelection::LAST));
    axis_fields.emplace_back(table.readField<Axis>(std::string_view("y"), h5pp::TableSelection::LAST));
    axis_fields.emplace_back(table.readField<Axis>(std::initializer_list<std::string>{"y"}, h5pp::TableSelection::LAST));
    axis_fields.emplace_back(table.readField<Axis>(std::initializer_list<std::string_view>{"y"}, h5pp::TableSelection::LAST));
    axis_fields.emplace_back(table.readField<Axis>(std::vector<std::string>{"y"}, h5pp::TableSelection::LAST));
    axis_fields.emplace_back(table.readField<Axis>(std::vector<std::string_view>{"y"}, h5pp::TableSelection::LAST));
    axis_fields.emplace_back(table.readField<Axis>(std::array<std::string, 1>{"y"}, h5pp::TableSelection::LAST));
    axis_fields.emplace_back(table.readField<Axis>(std::array<std::string_view, 1>{"y"}, h5pp::TableSelection::LAST));
    axis_fields.emplace_back(table.readField<Axis>(1, h5pp::TableSelection::LAST));
    axis_fields.emplace_back(table.readField<Axis>(std::vector<size_t>{1}, h5pp::TableSelection::LAST));
    axis_fields.emplace_back(table.readField<Axis>(std::vector<long>{1}, h5pp::TableSelection::LAST));
    axis_fields.emplace_back(table.readField<Axis>(std::array<size_t, 1>{1}, h5pp::TableSelection::LAST));

    for(const auto &axis : axis_fields) CHECK(axis.axis == 1.0);
}

TEST_CASE("Compound sub-fields and mixed field selections round-trip correctly", "[table-fields]") {
    h5pp::File file(make_table_file(), h5pp::FileAccess::READWRITE, 0);
    auto       table = file.table("somegroup/particleTable");

    auto rho_first  = table.readField<Rho>("rho", 0, 1);
    auto rho_last   = table.readField<Rho>("rho", -1ul, 1);
    auto cplx_first = table.readField<Complex>("cplx", 0, 1);
    auto cplx_last  = table.readField<Complex>("cplx", -1ul, 1);

    CHECK(rho_first.rho[0] == 20);
    CHECK(rho_first.rho[1] == 3.13);
    CHECK(rho_first.rho[2] == 102.4);
    CHECK(rho_last.rho[0] == 20);
    CHECK(rho_last.rho[1] == 3.13);
    CHECK(rho_last.rho[2] == 102.4);
    CHECK(cplx_first.cplx == std::complex<double>(1, 1));
    CHECK(cplx_last.cplx == std::complex<double>(1, 1));

    std::vector<Coords> coords_fields;
    coords_fields.emplace_back(table.readField<Coords>(std::initializer_list<std::string>{"x", "y", "z", "t"}, h5pp::TableSelection::LAST));
    coords_fields.emplace_back(table.readField<Coords>(std::vector<std::string>{"x", "y", "z", "t"}, h5pp::TableSelection::LAST));
    coords_fields.emplace_back(table.readField<Coords>(std::vector<std::string_view>{"x", "y", "z", "t"}, h5pp::TableSelection::LAST));
    coords_fields.emplace_back(table.readField<Coords>(std::array<std::string, 4>{"x", "y", "z", "t"}, h5pp::TableSelection::LAST));
    coords_fields.emplace_back(table.readField<Coords>(std::initializer_list<size_t>{0, 1, 2, 3}, h5pp::TableSelection::LAST));
    coords_fields.emplace_back(table.readField<Coords>(std::vector<size_t>{0, 1, 2, 3}, h5pp::TableSelection::LAST));
    coords_fields.emplace_back(table.readField<Coords>(std::array<size_t, 4>{0, 1, 2, 3}, h5pp::TableSelection::LAST));

    for(const auto &coords : coords_fields) {
        CHECK(coords.x == 0.0);
        CHECK(coords.y == 1.0);
        CHECK(coords.z == 2.0);
        CHECK(coords.t == 3.0);
    }

    std::vector<RhoName> rho_name_fields;
    rho_name_fields.emplace_back(table.readField<RhoName>(std::initializer_list<std::string>{"rho", "name"}, h5pp::TableSelection::LAST));
    rho_name_fields.emplace_back(table.readField<RhoName>(std::vector<std::string>{"rho", "name"}, h5pp::TableSelection::LAST));
    rho_name_fields.emplace_back(table.readField<RhoName>(std::vector<std::string_view>{"rho", "name"}, h5pp::TableSelection::LAST));
    rho_name_fields.emplace_back(table.readField<RhoName>(std::array<std::string, 2>{"rho", "name"}, h5pp::TableSelection::LAST));
    rho_name_fields.emplace_back(table.readField<RhoName>(std::initializer_list<size_t>{4, 5}, h5pp::TableSelection::LAST));
    rho_name_fields.emplace_back(table.readField<RhoName>(std::vector<size_t>{4, 5}, h5pp::TableSelection::LAST));
    rho_name_fields.emplace_back(table.readField<RhoName>(std::array<size_t, 2>{4, 5}, h5pp::TableSelection::LAST));

    for(const auto &rho_name : rho_name_fields) {
        CHECK(rho_name.rho[0] == 20);
        CHECK(rho_name.rho[1] == 3.13);
        CHECK(rho_name.rho[2] == 102.4);
        CHECK(strncmp(rho_name.name, "some name", 10) == 0);
    }
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    session.configData().shouldDebugBreak = true;
    return session.run();
}
