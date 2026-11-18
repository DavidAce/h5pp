#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>

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
        h5pp::File file(path, h5pp::FileAccess::REPLACE, 0);
        file.setCompressionLevel(6);
        auto table = file.table("somegroup/particleTable").create(make_particle_type(), "particleTable", {.compression = 6});
        table.appendRecords(std::vector<Particle>(10));
        return path;
    }
}

TEST_CASE("Tables can be created, appended, inspected and partially read through multiple selectors", "[tables]") {
    h5pp::File file(make_table_file(), h5pp::FileAccess::READWRITE, 0);
    auto       table = file.table("somegroup/particleTable");

    auto info = table.getInfo();
    auto fieldInfo = table.getFieldInfo();
    REQUIRE(info.tableTitle.value() == "particleTable");
    REQUIRE(info.numRecords.value() == 10);
    REQUIRE(info.numFields.value() == 6);
    REQUIRE(info.recordBytes.value() == sizeof(Particle));
    REQUIRE_THAT(info.fieldSizes.value(), Catch::Matchers::Equals(std::vector<size_t>{8, 8, 8, 8, 24, 10}));
    REQUIRE(fieldInfo.fieldNames.has_value());
    REQUIRE(fieldInfo.fieldNames.value().size() == 6);
    REQUIRE(fieldInfo.fieldNames.value()[0] == "x");
    REQUIRE(fieldInfo.fieldNames.value()[1] == "y");
    REQUIRE(fieldInfo.fieldNames.value()[2] == "z");
    REQUIRE(fieldInfo.fieldNames.value()[3] == "t");
    REQUIRE(fieldInfo.fieldNames.value()[4] == "rho");
    REQUIRE(fieldInfo.fieldNames.value()[5] == "name");
    REQUIRE(table.fieldExists(std::vector<std::string>{"x"}));
    REQUIRE(table.fieldExists(std::vector<std::string>{"rho"}));
    REQUIRE_FALSE(table.fieldExists(std::vector<std::string>{"missing"}));

    Particle              particle_default;
    std::vector<Particle> particle_read;
    particle_read.emplace_back(table.readRecords<Particle>());
    particle_read.emplace_back(file.table(std::string("somegroup/particl\0eTable", 24)).readRecords<Particle>());
    particle_read.emplace_back(file.table(std::string("somegroup/particleTable")).readRecords<Particle>());
    particle_read.emplace_back(file.table(std::string_view("somegroup/particleTable")).readRecords<Particle>());
    for(const auto &particle : particle_read) require_particle_equal(particle, particle_default);

    std::vector<std::vector<Particle>> result_container;
    result_container.emplace_back(table.readRecords<std::vector<Particle>>());
    result_container.emplace_back(table.readRecords<std::vector<Particle>>(0));
    result_container.emplace_back(table.readRecords<std::vector<Particle>>(5));
    result_container.emplace_back(table.readRecords<std::vector<Particle>>(std::nullopt, 5));
    result_container.emplace_back(table.readRecords<std::vector<Particle>>(0, std::nullopt));
    result_container.emplace_back(table.readRecords<std::vector<Particle>>(0, 5));
    result_container.emplace_back(table.readRecords<std::vector<Particle>>(h5pp::TableSelection::ALL));
    result_container.emplace_back(table.readRecords<std::vector<Particle>>(h5pp::TableSelection::FIRST));
    result_container.emplace_back(table.readRecords<std::vector<Particle>>(h5pp::TableSelection::LAST));

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
    h5pp::File file(make_table_file(), h5pp::FileAccess::READWRITE, 0);
    auto       table = file.table("somegroup/particleTable");

    std::vector<Particle> extra_particles(3);
    for(size_t idx = 0; idx < extra_particles.size(); ++idx) extra_particles[idx].x = 100.0 + static_cast<double>(idx);
    table.appendRecords(extra_particles);
    auto appended_info = table.getInfo();
    REQUIRE(appended_info.numRecords.value() == 13);

    auto last_three = table.readRecords<std::vector<Particle>>(10, 3);
    REQUIRE(last_three.size() == 3);
    for(size_t idx = 0; idx < last_three.size(); ++idx) {
        CHECK(last_three[idx].x == 100.0 + static_cast<double>(idx));
        CHECK(last_three[idx].y == 0.0);
    }

    auto       info1 = table.getInfo();
    h5pp::File copy_target(make_path("readWriteTablesCopy"), h5pp::FileAccess::REPLACE, 0);
    auto       info2 = copy_target.table("somegroup/particleTable")
                            .appendRecordsFrom(file.advanced().openFileHandle(), "somegroup/particleTable", h5pp::TableSelection::LAST);

    CHECK(info2.tableTitle.value() == info1.tableTitle.value());
    CHECK(info2.numRecords.value() == 1);
    CHECK(info2.recordBytes.value() == info1.recordBytes.value());
    CHECK_THAT(info2.fieldSizes.value(), Catch::Matchers::Equals(info1.fieldSizes.value()));

    auto copied_last = copy_target.table("somegroup/particleTable").readRecords<Particle>();
    CHECK(copied_last.x == 102.0);
    CHECK(copied_last.y == 0.0);
}

TEST_CASE("Stored table creation options are used when records are first appended", "[tables]") {
    h5pp::File               file(make_path("tableEnsure"), h5pp::FileAccess::REPLACE, 0);
    h5pp::TableCreateOptions options;
    options.title       = "ensured table";
    options.h5Type      = make_particle_type();
    options.dimsChunk   = {2};
    options.compression = 1;
    auto table          = file.table("particles").ensure(options);
    REQUIRE_FALSE(table.exists());

    std::vector<Particle> records(3);
    for(size_t idx = 0; idx < records.size(); ++idx) records[idx].x = static_cast<double>(idx + 1);
    table.appendRecords(records);
    table.appendRecords(records);
    auto info = table.getInfo();
    CHECK(info.tableTitle == options.title);
    CHECK(info.numRecords == 6);
    REQUIRE(info.h5Type.has_value());
    CHECK(H5Tequal(info.h5Type.value(), options.h5Type.value()) > 0);
    REQUIRE(info.chunkDims.has_value());
    CHECK(info.chunkDims.value() == std::vector<hsize_t>{2});
    REQUIRE(info.h5DsetCreate.has_value());
    CHECK(h5pp::hdf5::getDeflateLevel(info.h5DsetCreate.value()) > 0);
    auto read = table.readRecords<std::vector<Particle>>();
    REQUIRE(read.size() == 6);
    for(size_t idx = 0; idx < read.size(); ++idx) require_particle_equal(read[idx], records[idx % records.size()]);

    auto copied     = file.table("copied").ensure(options);
    auto copiedInfo = copied.appendRecordsFrom(info);
    REQUIRE(copiedInfo.chunkDims.has_value());
    CHECK(copiedInfo.chunkDims.value() == std::vector<hsize_t>{2});
    REQUIRE(copiedInfo.h5DsetCreate.has_value());
    CHECK(h5pp::hdf5::getDeflateLevel(copiedInfo.h5DsetCreate.value()) > 0);
    auto copiedRecords = copied.readRecords<std::vector<Particle>>();
    REQUIRE(copiedRecords.size() == read.size());
    for(size_t idx = 0; idx < read.size(); ++idx) require_particle_equal(copiedRecords[idx], read[idx]);
}

TEST_CASE("Appending to a missing table requires complete creation options", "[tables][validation]") {
    h5pp::File                  file(make_path("tableInvalidCreate"), h5pp::FileAccess::REPLACE, 0);
    auto                        table = file.table("missing");
    const std::vector<Particle> records(1);
    SECTION("no creation options") {
        REQUIRE_THROWS_WITH(table.appendRecords(records), Catch::Matchers::ContainsSubstring("no creation options"));
    }
    SECTION("missing datatype") {
        h5pp::TableCreateOptions options;
        options.title = "missing datatype";
        REQUIRE_THROWS_WITH(table.ensure(options).appendRecords(records), Catch::Matchers::ContainsSubstring("h5Type is not set"));
    }
    SECTION("missing title") {
        h5pp::TableCreateOptions options;
        options.h5Type = make_particle_type();
        REQUIRE_THROWS_WITH(table.ensure(options).appendRecords(records), Catch::Matchers::ContainsSubstring("title is not set"));
    }
    CHECK_FALSE(table.exists());
}

TEST_CASE("Table descriptions include their path, title and field layout", "[tables][metadata]") {
    h5pp::File file(make_table_file(), h5pp::FileAccess::READONLY, 0);
    auto       table = file.table("somegroup/particleTable");
    auto       info  = table.getInfo();
    CHECK_THAT(info.string(), Catch::Matchers::ContainsSubstring("somegroup/particleTable"));
    CHECK_THAT(info.string(), Catch::Matchers::ContainsSubstring("title [particleTable]"));
    CHECK_THAT(info.string(), Catch::Matchers::ContainsSubstring("num records [10]"));
    CHECK(info.string(false).empty());

    auto                      fields = table.getFieldInfo();
    const std::vector<size_t> sizes =
        {sizeof(Particle::x), sizeof(Particle::y), sizeof(Particle::z), sizeof(Particle::t), sizeof(Particle::rho), sizeof(Particle::name)};
    const std::vector<size_t> offsets = {HOFFSET(Particle, x),
                                         HOFFSET(Particle, y),
                                         HOFFSET(Particle, z),
                                         HOFFSET(Particle, t),
                                         HOFFSET(Particle, rho),
                                         HOFFSET(Particle, name)};
    CHECK_THAT(fields.string(), Catch::Matchers::ContainsSubstring(h5pp::format("sizes [{}]", sizes)));
    CHECK_THAT(fields.string(), Catch::Matchers::ContainsSubstring(h5pp::format("offsets [{}]", offsets)));
    CHECK_THAT(fields.string_fields(), Catch::Matchers::ContainsSubstring("rho"));
    CHECK_THAT(fields.string_fields(), Catch::Matchers::ContainsSubstring("H5T_ARRAY"));
    CHECK_THAT(fields.string_fields(), Catch::Matchers::ContainsSubstring("H5T_STRING"));
    CHECK(fields.string(false).empty());
    CHECK(fields.string_fields(false).empty());
    CHECK(h5pp::TableFieldInfo{}.string_fields().empty());
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    session.configData().shouldDebugBreak = true;
    return session.run();
}
