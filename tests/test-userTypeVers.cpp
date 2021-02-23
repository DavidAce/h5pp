
#include <h5pp/h5pp.h>
#include <iostream>

struct ParticleV1 {
    double x = 0, y = 0;
    int    id       = 0;
    char   version[10] = ""; // Can't be replaced by std::string, or anything resizeable?
    bool   operator==(const ParticleV1 &p) const { return x == p.x and y == p.y and strncmp(version, p.version, 10) == 0 and id == p.id; }
    bool   operator!=(const ParticleV1 &p) const { return not(*this == p); }
};

struct ParticleV2 {
    double x = 0, y = 0, z = 0, t = 0;
    int    id       = 0;
    char   version[10] = ""; // Can't be replaced by std::string, or anything resizeable?
    bool   operator==(const ParticleV2 &p) const { return x == p.x and y == p.y and z == p.z and t == p.t and strncmp(version, p.version, 10) == 0 and id == p.id; }
    bool   operator!=(const ParticleV2 &p) const { return not(*this == p); }
};


void print_particle(const ParticleV1 &p, const std::string & msg = "") {
    h5pp::print("{} \t x: {} \t y: {} \t id: {}  \t version: {}\n", msg,p.x,p.y,p.id,p.version);
}
void print_particle(const ParticleV2 &p, const std::string & msg = "") {
    h5pp::print("{} \t x: {} \t y: {} \t z: {} \t t: {} \t id: {}  \t version: {}\n", msg,p.x,p.y, p.z,p.t,p.id,p.version);
}


int main() {
    h5pp::File file("output/userTypeVers.h5", h5pp::FilePermission::REPLACE, 2);

    // Create a type for the char array from the template H5T_C_S1
    // The template describes a string with a single char.
    // Set the size with H5Tset_size.
    h5pp::hid::h5t H5_VERS_TYPE = H5Tcopy(H5T_C_S1);
    H5Tset_size(H5_VERS_TYPE, 10);
    // Optionally set the null terminator '\0' and possibly padding.
    H5Tset_strpad(H5_VERS_TYPE, H5T_STR_NULLTERM);

    // Register the compound type
    h5pp::hid::h5t H5_PARTICLE_V1 = H5Tcreate(H5T_COMPOUND, sizeof(ParticleV1));
    H5Tinsert(H5_PARTICLE_V1, "x", HOFFSET(ParticleV1, x), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_V1, "y", HOFFSET(ParticleV1, y), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_V1, "id", HOFFSET(ParticleV1, id), H5T_NATIVE_INT);
    H5Tinsert(H5_PARTICLE_V1, "version", HOFFSET(ParticleV1, version), H5_VERS_TYPE);


    // Register the compound type
    h5pp::hid::h5t H5_PARTICLE_V2 = H5Tcreate(H5T_COMPOUND, sizeof(ParticleV2));
    H5Tinsert(H5_PARTICLE_V2, "x", HOFFSET(ParticleV2, x), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_V2, "y", HOFFSET(ParticleV2, y), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_V2, "z", HOFFSET(ParticleV2, z), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_V2, "t", HOFFSET(ParticleV2, t), H5T_NATIVE_DOUBLE);
    H5Tinsert(H5_PARTICLE_V2, "id", HOFFSET(ParticleV2, id), H5T_NATIVE_INT);
    H5Tinsert(H5_PARTICLE_V2, "version", HOFFSET(ParticleV2, version), H5_VERS_TYPE);

    // Define a single particle version 1
    ParticleV1 p1;
    p1.x  = 1;
    p1.y  = 2;
    p1.id = 1111;
    strncpy(p1.version, "v1", 10);
    // Define a single particle version 2
    ParticleV2 p2;
    p2.x  = 1;
    p2.y  = 2;
    p2.z  = 3;
    p2.t  = 4;
    p2.id = 2222;
    strncpy(p2.version, "v2", 10);


    // Write both particles
    file.writeDataset(p1, "singleParticle1", H5_PARTICLE_V1);
    file.writeDataset(p2, "singleParticle2", H5_PARTICLE_V2);


    auto p1_as_v1_h1 = file.readDataset<ParticleV1>("singleParticle1", std::nullopt, H5_PARTICLE_V1);
    print_particle(p1_as_v1_h1, "p1 as v1|h1: Should work:");

    try{
        auto p1_as_v1_h2 = file.readDataset<ParticleV1>("singleParticle1", std::nullopt, H5_PARTICLE_V2);
        print_particle(p1_as_v1_h2,"p1 as v1|h2");
    }catch (const std::exception & ex){
        h5pp::print("p1 as v1|h2: Caught expected error: {}\n", ex.what());
    }

    auto p1_as_v2_h1 = file.readDataset<ParticleV2>("singleParticle1", std::nullopt, H5_PARTICLE_V1);
    print_particle(p1_as_v2_h1, "p1 as v2|h1: Should fail:");

    auto p1_as_v2_h2 = file.readDataset<ParticleV2>("singleParticle1", std::nullopt, H5_PARTICLE_V2);
    print_particle(p1_as_v2_h2, "p1 as v2|h2: Should work:");

    auto p2_as_v1_h1 = file.readDataset<ParticleV1>("singleParticle2", std::nullopt, H5_PARTICLE_V1);
    print_particle(p2_as_v1_h1, "p2 as v1|h1: Should work:");

    try{
        auto p2_as_v1_h2 = file.readDataset<ParticleV1>("singleParticle2", std::nullopt, H5_PARTICLE_V2);
        print_particle(p2_as_v1_h2,"p2 as v1|h2");

    }catch (const std::exception & ex){
        h5pp::print("p2 as v1|h2: Caught expected error: {}\n", ex.what());
    }


    auto p2_as_v2_h1 = file.readDataset<ParticleV2>("singleParticle2", std::nullopt, H5_PARTICLE_V1);
    print_particle(p2_as_v2_h1,"p2 as v2|h1: Should fail:");

    auto p2_as_v2_h2 = file.readDataset<ParticleV2>("singleParticle2", std::nullopt, H5_PARTICLE_V2);
    print_particle(p2_as_v2_h2,"p2 as v2|h2: Should work:");

    return 0;
}