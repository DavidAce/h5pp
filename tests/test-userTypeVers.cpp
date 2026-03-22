#include <algorithm>
#include <array>
#include <catch2/catch_all.hpp>
#include <cstring>
#include <h5pp/h5pp.h>
#include <string>
#include <string_view>
#include <vector>

namespace {
    struct ParticleV1 {
        double x        = 0;
        double y        = 0;
        int    id       = 0;
        char   name[11] = "";

        bool operator==(const ParticleV1 &other) const {
            return x == other.x && y == other.y && id == other.id && std::strncmp(name, other.name, sizeof(name)) == 0;
        }
    };

    struct ParticleV2 {
        double x        = 0;
        double y        = 0;
        double z        = 3.1415;
        short  t        = 1;
        int    id       = 0;
        char   name[22] = "";

        bool operator==(const ParticleV2 &other) const {
            return x == other.x && y == other.y && z == other.z && t == other.t && id == other.id &&
                   std::strncmp(name, other.name, sizeof(name)) == 0;
        }
    };

    struct RegisteredTypes {
        h5pp::hid::h5t name_type;
        h5pp::hid::h5t particle_v1;
        h5pp::hid::h5t particle_v2;
    };

    RegisteredTypes register_types() {
        RegisteredTypes types;
        types.name_type = H5Tcopy(H5T_C_S1);
        H5Tset_size(types.name_type, 10);
        H5Tset_strpad(types.name_type, H5T_STR_NULLTERM);

        types.particle_v1 = H5Tcreate(H5T_COMPOUND, sizeof(ParticleV1));
        H5Tinsert(types.particle_v1, "x", HOFFSET(ParticleV1, x), H5T_NATIVE_DOUBLE);
        H5Tinsert(types.particle_v1, "y", HOFFSET(ParticleV1, y), H5T_NATIVE_DOUBLE);
        H5Tinsert(types.particle_v1, "id", HOFFSET(ParticleV1, id), H5T_NATIVE_INT);
        H5Tinsert(types.particle_v1, "name", HOFFSET(ParticleV1, name), types.name_type);

        types.particle_v2 = H5Tcreate(H5T_COMPOUND, sizeof(ParticleV2));
        H5Tinsert(types.particle_v2, "x", HOFFSET(ParticleV2, x), H5T_NATIVE_DOUBLE);
        H5Tinsert(types.particle_v2, "y", HOFFSET(ParticleV2, y), H5T_NATIVE_DOUBLE);
        H5Tinsert(types.particle_v2, "z", HOFFSET(ParticleV2, z), H5T_NATIVE_DOUBLE);
        H5Tinsert(types.particle_v2, "t", HOFFSET(ParticleV2, t), H5T_NATIVE_SHORT);
        H5Tinsert(types.particle_v2, "id", HOFFSET(ParticleV2, id), H5T_NATIVE_INT);
        H5Tinsert(types.particle_v2, "name", HOFFSET(ParticleV2, name), types.name_type);
        return types;
    }

    std::string make_path(std::string_view name) {
        h5pp::fs::create_directories("output");
        return h5pp::format("output/{}.h5", name);
    }

    bool are_common_members_equal(const ParticleV1 &lhs, const ParticleV2 &rhs) {
        constexpr auto compare_len = sizeof(ParticleV1::name) == sizeof(ParticleV2::name)
                                         ? sizeof(ParticleV1::name)
                                         : std::min(sizeof(ParticleV1::name), sizeof(ParticleV2::name)) - 1;
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.id == rhs.id && std::strncmp(lhs.name, rhs.name, compare_len) == 0;
    }

    bool are_common_members_equal_with_defaults(const ParticleV1 &lhs, const ParticleV2 &rhs) {
        const auto def = ParticleV2{};
        return are_common_members_equal(lhs, rhs) && rhs.z == def.z && rhs.t == def.t;
    }

    ParticleV1 create_unique_v1(int i) { return ParticleV1{100.0 + i, 200.0 + i, 1000 + i, "v1-123456"}; }

    ParticleV2 create_unique_v2(int i) {
        return ParticleV2{100.0 + i, 200.0 + i, 300.0 + i, static_cast<short>(400 + i), 1000 + i, "v2-1234567890123"};
    }
}

TEST_CASE("Single versioned compound datasets remain forward and backward compatible", "[user-type-vers][single]") {
    auto       path  = make_path("userTypeVers-single");
    auto       types = register_types();
    h5pp::File file(path, h5pp::FileAccess::REPLACE, 0);

    auto p1 = create_unique_v1(5);
    auto p2 = create_unique_v2(7);

    file.writeDataset(p1, "singleParticle1", types.particle_v1);
    file.writeDataset(p2, "singleParticle2", types.particle_v2);

    SECTION("Exact reads with matching memory and file types") {
        REQUIRE(file.readDataset<ParticleV1>("singleParticle1", std::nullopt, types.particle_v1) == p1);
        REQUIRE(file.readDataset<ParticleV2>("singleParticle2", std::nullopt, types.particle_v2) == p2);
    }

    SECTION("Mismatched memory type with matching file type fails") {
        REQUIRE_THROWS(file.readDataset<ParticleV1>("singleParticle1", std::nullopt, types.particle_v2));
        REQUIRE_THROWS(file.readDataset<ParticleV1>("singleParticle2", std::nullopt, types.particle_v2));
    }

    SECTION("Forward compatibility populates missing members with defaults") {
        auto p1_as_v2 = file.readDataset<ParticleV2>("singleParticle1", std::nullopt, types.particle_v2);
        REQUIRE(are_common_members_equal_with_defaults(p1, p1_as_v2));
        REQUIRE(p1_as_v2.z == ParticleV2{}.z);
        REQUIRE(p1_as_v2.t == ParticleV2{}.t);
    }

    SECTION("Backward compatibility ignores unknown members") {
        auto p2_as_v1 = file.readDataset<ParticleV1>("singleParticle2", std::nullopt, types.particle_v1);
        REQUIRE(are_common_members_equal(p2_as_v1, p2));
    }

    SECTION("Using the old file type to read into the new memory type is not a compatibility path") {
        auto p1_as_v2_h1 = file.readDataset<ParticleV2>("singleParticle1", std::nullopt, types.particle_v1);
        auto p2_as_v2_h1 = file.readDataset<ParticleV2>("singleParticle2", std::nullopt, types.particle_v1);
        REQUIRE_FALSE(are_common_members_equal_with_defaults(p1, p1_as_v2_h1));
        REQUIRE_FALSE(are_common_members_equal_with_defaults(create_unique_v1(7), p2_as_v2_h1));
    }
}

TEST_CASE("Vectors of versioned compound datasets remain compatible across layouts", "[user-type-vers][vector]") {
    auto       layout = GENERATE(values({H5D_COMPACT, H5D_CONTIGUOUS, H5D_CHUNKED}));
    auto       path   = make_path(h5pp::format("userTypeVers-vector-{}", static_cast<int>(layout)));
    auto       types  = register_types();
    h5pp::File file(path, h5pp::FileAccess::REPLACE, 0);

    std::vector<ParticleV1> vp1;
    std::vector<ParticleV2> vp2;
    for(int i = 0; i < 10; ++i) {
        vp1.push_back(create_unique_v1(i));
        vp2.push_back(create_unique_v2(i + 10));
    }

    file.writeDataset(vp1, "vectorParticle1", layout, std::nullopt, std::nullopt, std::nullopt, types.particle_v1);
    file.writeDataset(vp2, "vectorParticle2", layout, std::nullopt, std::nullopt, std::nullopt, types.particle_v2);

    auto info_v1 = file.getDatasetInfo("vectorParticle1");
    auto info_v2 = file.getDatasetInfo("vectorParticle2");
    REQUIRE(info_v1.dsetDims);
    REQUIRE(info_v2.dsetDims);
    REQUIRE(info_v1.dsetDims.value() == std::vector<hsize_t>{vp1.size()});
    REQUIRE(info_v2.dsetDims.value() == std::vector<hsize_t>{vp2.size()});

    SECTION("Matching versions round-trip exactly") {
        REQUIRE(file.readDataset<std::vector<ParticleV1>>("vectorParticle1", std::nullopt, types.particle_v1) == vp1);
        REQUIRE(file.readDataset<std::vector<ParticleV2>>("vectorParticle2", std::nullopt, types.particle_v2) == vp2);
    }

    SECTION("Old files are readable by new software with defaults") {
        auto vp1_as_v2 = file.readDataset<std::vector<ParticleV2>>("vectorParticle1", std::nullopt, types.particle_v2);
        REQUIRE(std::equal(vp1.begin(), vp1.end(), vp1_as_v2.begin(), are_common_members_equal_with_defaults));
    }

    SECTION("New files are readable by old software by ignoring unknown members") {
        auto vp2_as_v1 = file.readDataset<std::vector<ParticleV1>>("vectorParticle2", std::nullopt, types.particle_v1);
        REQUIRE(std::equal(vp2_as_v1.begin(), vp2_as_v1.end(), vp2.begin(), are_common_members_equal));
    }
}

TEST_CASE("Versioned compound attributes follow the same compatibility rules", "[user-type-vers][attribute]") {
    auto       path  = make_path("userTypeVers-attr");
    auto       types = register_types();
    h5pp::File file(path, h5pp::FileAccess::REPLACE, 0);

    auto p1 = create_unique_v1(1);
    auto p2 = create_unique_v2(2);
    file.writeDataset(std::vector<int>{1, 2, 3}, "holder");
    file.writeAttribute(p1, "holder", "attr_v1", std::nullopt, types.particle_v1);
    file.writeAttribute(p2, "holder", "attr_v2", std::nullopt, types.particle_v2);

    REQUIRE(file.readAttribute<ParticleV1>("holder", "attr_v1", std::nullopt, types.particle_v1) == p1);
    REQUIRE(file.readAttribute<ParticleV2>("holder", "attr_v2", std::nullopt, types.particle_v2) == p2);

    auto attr_v1_as_v2 = file.readAttribute<ParticleV2>("holder", "attr_v1", std::nullopt, types.particle_v2);
    auto attr_v2_as_v1 = file.readAttribute<ParticleV1>("holder", "attr_v2", std::nullopt, types.particle_v1);

    REQUIRE(are_common_members_equal_with_defaults(p1, attr_v1_as_v2));
    REQUIRE(are_common_members_equal(attr_v2_as_v1, p2));
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    return session.run();
}
