#include <catch2/catch_all.hpp>
#include <h5pp/v1/h5pp.h>

struct Particle {
    double x = 0, y = 0, z = 0, t = 0;
    double rho[3]   = {20, 3.13, 102.4};
    char   name[10] = "some name";
    void   dummy_function(int) {}
};

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }

    void require_particle_equal(const Particle &lhs, const Particle &rhs) {
        CHECK(lhs.x == rhs.x);
        CHECK(lhs.y == rhs.y);
        CHECK(lhs.z == rhs.z);
        CHECK(lhs.t == rhs.t);
        CHECK(lhs.rho[0] == rhs.rho[0]);
        CHECK(lhs.rho[1] == rhs.rho[1]);
        CHECK(lhs.rho[2] == rhs.rho[2]);
        CHECK(strncmp(lhs.name, rhs.name, 10) == 0);
    }

    h5pp::hid::h5t make_particle_type() {
        h5pp::hid::h5t name_type = H5Tcopy(H5T_C_S1);
        H5Tset_size(name_type, 10);
        H5Tset_strpad(name_type, H5T_STR_NULLTERM);

        std::vector<hsize_t> rho_dims = {3};
        h5pp::hid::h5t       rho_type = H5Tarray_create(H5T_NATIVE_DOUBLE, static_cast<unsigned int>(rho_dims.size()), rho_dims.data());

        h5pp::hid::h5t particle_type = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
        H5Tinsert(particle_type, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
        H5Tinsert(particle_type, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
        H5Tinsert(particle_type, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
        H5Tinsert(particle_type, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);
        H5Tinsert(particle_type, "rho", HOFFSET(Particle, rho), rho_type);
        H5Tinsert(particle_type, "name", HOFFSET(Particle, name), name_type);
        return particle_type;
    }

    std::string make_table_file() {
        auto       path = make_path("readWriteTables");
        h5pp::v1::File file(path, h5pp::FileAccess::REPLACE, 0);
        file.setCompressionLevel(6);
        auto particle_type = make_particle_type();
        file.createTable(particle_type, "somegroup/particleTable", "particleTable", std::nullopt, 6);
        file.appendTableRecords(std::vector<Particle>(10), "somegroup/particleTable");
        return path;
    }
}

TEST_CASE("Tables can be created, appended, inspected and partially read through multiple overloads", "[tables]") {
    h5pp::v1::File file(make_table_file(), h5pp::FileAccess::READWRITE, 0);

    auto info = file.getTableInfo("somegroup/particleTable");
    REQUIRE(info.tableTitle.value() == "particleTable");
    REQUIRE(info.numRecords.value() == 10);
    REQUIRE(info.recordBytes.value() == sizeof(Particle));
    REQUIRE_THAT(info.fieldSizes.value(), Catch::Matchers::Equals(std::vector<size_t>{8, 8, 8, 8, 24, 10}));

    Particle              particle_default;
    std::vector<Particle> particle_read;
    particle_read.emplace_back(file.readTableRecords<Particle>("somegroup/particleTable"));
    particle_read.emplace_back(file.readTableRecords<Particle>(std::string("somegroup/particl\0eTable", 24)));
    particle_read.emplace_back(file.readTableRecords<Particle>(std::string("somegroup/particleTable")));
    particle_read.emplace_back(file.readTableRecords<Particle>(std::string_view("somegroup/particleTable")));
    for(const auto &particle : particle_read) require_particle_equal(particle, particle_default);

    std::vector<std::vector<Particle>> result_container;
    result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable"));
    result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", 0));
    result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", 5));
    result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", std::nullopt, 5));
    result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", 0, std::nullopt));
    result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", 0, 5));
    result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", h5pp::TableSelection::ALL));
    result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", h5pp::TableSelection::FIRST));
    result_container.emplace_back(file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", h5pp::TableSelection::LAST));

    REQUIRE(result_container[0].size() == 10);
    REQUIRE(result_container[1].size() == 10);
    REQUIRE(result_container[2].size() == 5);
    REQUIRE(result_container[3].size() == 5);
    REQUIRE(result_container[4].size() == 10);
    REQUIRE(result_container[5].size() == 5);
    REQUIRE(result_container[6].size() == 10);
    REQUIRE(result_container[7].size() == 1);
    REQUIRE(result_container[8].size() == 1);
    for(const auto &particles : result_container)
        for(const auto &particle : particles) require_particle_equal(particle, particle_default);
}

TEST_CASE("Appending and copying table records preserves metadata and selected rows", "[tables]") {
    h5pp::v1::File file(make_table_file(), h5pp::FileAccess::READWRITE, 0);

    std::vector<Particle> extra_particles(3);
    for(size_t idx = 0; idx < extra_particles.size(); ++idx) extra_particles[idx].x = 100.0 + static_cast<double>(idx);
    auto appended_info = file.appendTableRecords(extra_particles, "somegroup/particleTable");
    REQUIRE(appended_info.numRecords.value() == 13);

    auto last_three = file.readTableRecords<std::vector<Particle>>("somegroup/particleTable", 10, 3);
    REQUIRE(last_three.size() == 3);
    for(size_t idx = 0; idx < last_three.size(); ++idx) {
        CHECK(last_three[idx].x == 100.0 + static_cast<double>(idx));
        CHECK(last_three[idx].y == 0.0);
    }

    auto       info1 = file.getTableInfo("somegroup/particleTable");
    h5pp::v1::File copy_target(make_path("readWriteTablesCopy"), h5pp::FileAccess::REPLACE, 0);
    auto       info2 = copy_target.appendTableRecords(file.openFileHandle(),
                                                "somegroup/particleTable",
                                                "somegroup/particleTable",
                                                h5pp::TableSelection::LAST);

    CHECK(info2.tableTitle.value() == info1.tableTitle.value());
    CHECK(info2.numRecords.value() == 1);
    CHECK(info2.recordBytes.value() == info1.recordBytes.value());
    CHECK_THAT(info2.fieldSizes.value(), Catch::Matchers::Equals(info1.fieldSizes.value()));

    auto copied_last = copy_target.readTableRecords<Particle>("somegroup/particleTable");
    CHECK(copied_last.x == 102.0);
    CHECK(copied_last.y == 0.0);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    session.configData().shouldDebugBreak = true;
    return session.run();
}
