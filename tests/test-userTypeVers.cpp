#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <h5pp/h5pp.h>
#include <iostream>

struct ParticleV1 {
    double x = 0, y = 0;
    int    id       = 0;
    char   name[11] = ""; // Can't be replaced by std::string, or anything resizeable?
    bool   operator==(const ParticleV1 &p) const { return x == p.x and y == p.y and strncmp(name, p.name, 11) == 0 and id == p.id; }
    bool   operator!=(const ParticleV1 &p) const { return not(*this == p); }
};

struct ParticleV2 {
    double x = 0, y = 0, z = 0, t = 0;
    int    id       = 0;
    char   name[11] = ""; // Can't be replaced by std::string, or anything resizeable?
    bool   operator==(const ParticleV2 &p) const { return x == p.x and y == p.y and z == p.z and t == p.t and strncmp(name, p.name, 11) == 0 and id == p.id; }
    bool   operator!=(const ParticleV2 &p) const { return not(*this == p); }
};

bool are_common_members_equal(const ParticleV1 &p1, const ParticleV2 &p2)
{
    return p1.x == p2.x and p1.y == p2.y and strncmp(p1.name, p2.name, 11) == 0 and p1.id == p2.id;
}

bool are_common_members_equal_with_defaults(const ParticleV1 &p1, const ParticleV2 &p2)
{
    return are_common_members_equal(p1, p2) and p2.z == 0.0 and p2.t == 0.0;
}

auto create_unique_v1(int i)
{
    return ParticleV1{100.0+i, 200.0+i, 1000+i, "v1"};
}

auto create_unique_v2(int i)
{
    return ParticleV2{100.0+i, 200.0+i, 300.0+i, 400.0+i, 1000+i, "v2"};
}

void print_particle(const ParticleV1 &p, const std::string & msg = "") {
    h5pp::print("{} \t x: {} \t y: {} \t id: {}  \t name: {}\n", msg,p.x,p.y,p.id,p.name);
}
void print_particle(const ParticleV2 &p, const std::string & msg = "") {
    h5pp::print("{} \t x: {} \t y: {} \t z: {} \t t: {} \t id: {}  \t name: {}\n", msg,p.x,p.y, p.z,p.t,p.id,p.name);
}

h5pp::hid::h5t H5_NAME_TYPE;
h5pp::hid::h5t H5_PARTICLE_V1;
h5pp::hid::h5t H5_PARTICLE_V2;

void register_types()
{
    // Create a type for the char array from the template H5T_C_S1
    // The template describes a string with a single char.
    // Set the size with H5Tset_size.
    H5_NAME_TYPE = H5Tcopy(H5T_C_S1);
    H5Tset_size(H5_NAME_TYPE, 10);
    // Optionally set the null terminator '\0' and possibly padding.
    H5Tset_strpad(H5_NAME_TYPE, H5T_STR_NULLTERM);

    // Register the compound type
    H5_PARTICLE_V1 = H5Tcreate(H5T_COMPOUND, sizeof(ParticleV1));
    H5Tinsert(H5_PARTICLE_V1, "x", HOFFSET(ParticleV1, x), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_V1, "y", HOFFSET(ParticleV1, y), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_V1, "id", HOFFSET(ParticleV1, id), H5T_NATIVE_INT);
    H5Tinsert(H5_PARTICLE_V1, "name", HOFFSET(ParticleV1, name), H5_NAME_TYPE);


    // Register the compound type
    H5_PARTICLE_V2 = H5Tcreate(H5T_COMPOUND, sizeof(ParticleV2));
    H5Tinsert(H5_PARTICLE_V2, "x", HOFFSET(ParticleV2, x), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_V2, "y", HOFFSET(ParticleV2, y), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_V2, "z", HOFFSET(ParticleV2, z), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_V2, "t", HOFFSET(ParticleV2, t), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_V2, "id", HOFFSET(ParticleV2, id), H5T_NATIVE_INT);
    H5Tinsert(H5_PARTICLE_V2, "name", HOFFSET(ParticleV2, name), H5_NAME_TYPE);
}


TEST_CASE( "Single particles are compatible", "[single-particles]" ) 
{
    h5pp::File file("output/userTypeVers.h5", h5pp::FilePermission::REPLACE, 2);

    // Create a single particle version 1
    ParticleV1 p1 = create_unique_v1(5);
    
    // Create a single particle version 2
    ParticleV2 p2 = create_unique_v2(7);

    // Write both particles
    file.writeDataset(p1, "singleParticle1", H5_PARTICLE_V1);
    file.writeDataset(p2, "singleParticle2", H5_PARTICLE_V2);

    SECTION( "p1 as v1|h1" ) {
        auto p1_as_v1_h1 = file.readDataset<ParticleV1>("singleParticle1", std::nullopt, H5_PARTICLE_V1);
        print_particle(p1_as_v1_h1, "p1 as v1|h1: Should work:");
        REQUIRE( p1_as_v1_h1 == p1 );
    }

    SECTION( "p1 as v1|h2" ) {
        REQUIRE_THROWS( file.readDataset<ParticleV1>("singleParticle1", std::nullopt, H5_PARTICLE_V2) );
    }

    SECTION( "p1 as v2|h1" ) {
        auto p1_as_v2_h1 = file.readDataset<ParticleV2>("singleParticle1", std::nullopt, H5_PARTICLE_V1);
        print_particle(p1_as_v2_h1, "p1 as v2|h1: Should fail:");
        REQUIRE_FALSE( are_common_members_equal_with_defaults(p1, p1_as_v2_h1) );
    }

    //
    // Forward compatibility: p1 written by v1 software is forward compatible with p2 read by v2 software
    // with missing members initialized to 0 (not the same as default initialized).
    //
    SECTION( "p1 as v2|h2" ) {
        auto p1_as_v2_h2 = file.readDataset<ParticleV2>("singleParticle1", std::nullopt, H5_PARTICLE_V2);
        print_particle(p1_as_v2_h2, "p1 as v2|h2: Should work:");
        REQUIRE( are_common_members_equal_with_defaults(p1, p1_as_v2_h2) );
    }
    
    //
    // Backward compatibility: p2 written by v2 software is backward compatible with p1 read by v1 software
    // with new v2 members being ignored.
    //
    SECTION( "p2 as v1|h1" ) {
        auto p2_as_v1_h1 = file.readDataset<ParticleV1>("singleParticle2", std::nullopt, H5_PARTICLE_V1);
        print_particle(p2_as_v1_h1, "p2 as v1|h1: Should work:");
        REQUIRE( are_common_members_equal(p2_as_v1_h1, p2) );
    }

    SECTION( "p2 as v1|h2" ) {
        REQUIRE_THROWS( file.readDataset<ParticleV1>("singleParticle2", std::nullopt, H5_PARTICLE_V2) );
    }

    SECTION( "p2 as v2|h1" ) {
        auto p2_as_v2_h1 = file.readDataset<ParticleV2>("singleParticle2", std::nullopt, H5_PARTICLE_V1);
        print_particle(p2_as_v2_h1,"p2 as v2|h1: Should fail:");
        REQUIRE_FALSE( are_common_members_equal_with_defaults(p1, p2_as_v2_h1) );
    }

    SECTION( "p2 as v2|h2" ) {
        auto p2_as_v2_h2 = file.readDataset<ParticleV2>("singleParticle2", std::nullopt, H5_PARTICLE_V2);
        print_particle(p2_as_v2_h2,"p2 as v2|h2: Should work:");
        REQUIRE( p2_as_v2_h2 == p2 );
    }
}

TEST_CASE( "Vector of versioned structs is compatible" )
{
  for (const auto layout : {H5D_COMPACT, H5D_CONTIGUOUS, H5D_CHUNKED})
  {
    std::string layout_str = std::array<std::string,3>{"H5D_COMPACT","H5D_CONTIGUOUS","H5D_CHUNKED"}[layout];

    h5pp::File file("output/userTypeVers.h5", h5pp::FilePermission::REPLACE, 2);    

    // Create vectors of version 1 and 2 particles.
    std::vector<ParticleV1> vp1;
    std::vector<ParticleV2> vp2;

    for (int i = 0; i < 10; i++) {
        vp1.push_back(create_unique_v1(i));
        vp2.push_back(create_unique_v2(i+10));
    }

    // Write both particle vectors.
    file.writeDataset(vp1, "vectorParticle1", layout, std::nullopt, std::nullopt, std::nullopt, H5_PARTICLE_V1);
    file.writeDataset(vp2, "vectorParticle2", layout, std::nullopt, std::nullopt, std::nullopt, H5_PARTICLE_V2);

    SECTION( "file(vp1) is readable by v1 - " + layout_str) {
        auto vp1_as_v1_h1 = file.readDataset<std::vector<ParticleV1>>("vectorParticle1", std::nullopt, H5_PARTICLE_V1);
        REQUIRE(
            std::equal(vp1.begin(), vp1.end(), vp1_as_v1_h1.begin())
        );
    }

    SECTION( "file(vp2) is readable by v2 - " + layout_str ) {
        auto vp2_as_v2_h2 = file.readDataset<std::vector<ParticleV2>>("vectorParticle2", std::nullopt, H5_PARTICLE_V2);
        REQUIRE(
            std::equal(vp2.begin(), vp2.end(), vp2_as_v2_h2.begin())
        );
    }

    SECTION( "file(vp1) is readable by v2 with zero init missing members - " + layout_str ) {
        auto vp1_as_v2_h2 = file.readDataset<std::vector<ParticleV2>>("vectorParticle1", std::nullopt, H5_PARTICLE_V2);
        REQUIRE(
            std::equal(vp1.begin(), vp1.end(), vp1_as_v2_h2.begin(), are_common_members_equal_with_defaults)
        );
    }

    SECTION( "file(vp2) is readable by v1 ignoring unknown members - " + layout_str ) {
        auto vp2_as_v1_h1 = file.readDataset<std::vector<ParticleV1>>("vectorParticle2", std::nullopt, H5_PARTICLE_V1);
        REQUIRE(
            std::equal(vp2_as_v1_h1.begin(), vp2_as_v1_h1.end(), vp2.begin(), are_common_members_equal)
        );
    }
  }
}


int main(int argc, char *argv[]) {

    register_types();

    Catch::Session session; // There must be exactly one instance
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) // Indicates a command line error
        return returnCode;

    //    session.configData().showSuccessfulTests = true;
    //    session.configData().reporterName = "compact";
    return session.run();
}
