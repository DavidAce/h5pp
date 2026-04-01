#include <algorithm>
#include <catch2/catch_all.hpp>
#include <cstring>
#include <h5pp/v1/h5pp.h>
#include <string>
#include <string_view>
#include <vector>

namespace {
    struct Particle {
        double x        = 0;
        double y        = 0;
        double z        = 0;
        double t        = 0;
        int    id       = 0;
        char   name[10] = "some name";

        bool operator==(const Particle &other) const {
            return x == other.x && y == other.y && z == other.z && t == other.t && id == other.id &&
                   std::strncmp(name, other.name, sizeof(name)) == 0;
        }
    };

    struct RegisteredTypes {
        h5pp::hid::h5t name_type;
        h5pp::hid::h5t particle_type;
    };

    std::string make_path(std::string_view name) {
        h5pp::fs::create_directories("output");
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }

    RegisteredTypes register_types() {
        RegisteredTypes types;
        types.name_type = H5Tcopy(H5T_C_S1);
        H5Tset_size(types.name_type, 10);
        H5Tset_strpad(types.name_type, H5T_STR_NULLTERM);

        types.particle_type = H5Tcreate(H5T_COMPOUND, sizeof(Particle));
        H5Tinsert(types.particle_type, "x", HOFFSET(Particle, x), H5T_NATIVE_DOUBLE);
        H5Tinsert(types.particle_type, "y", HOFFSET(Particle, y), H5T_NATIVE_DOUBLE);
        H5Tinsert(types.particle_type, "z", HOFFSET(Particle, z), H5T_NATIVE_DOUBLE);
        H5Tinsert(types.particle_type, "t", HOFFSET(Particle, t), H5T_NATIVE_DOUBLE);
        H5Tinsert(types.particle_type, "name", HOFFSET(Particle, name), types.name_type);
        H5Tinsert(types.particle_type, "id", HOFFSET(Particle, id), H5T_NATIVE_INT);
        return types;
    }

    Particle make_particle(int seed, std::string_view name) {
        Particle particle;
        particle.x  = 1.0 + seed;
        particle.y  = 2.0 + seed;
        particle.z  = 3.0 + seed;
        particle.t  = 4.0 + seed;
        particle.id = 5 + seed;
        std::strncpy(particle.name, name.data(), sizeof(particle.name));
        particle.name[sizeof(particle.name) - 1] = '\0';
        return particle;
    }
}

TEST_CASE("Explicit compound user types round-trip as datasets and attributes", "[user-type]") {
    auto       path  = make_path("userType");
    auto       types = register_types();
    h5pp::v1::File file(path, h5pp::FileAccess::REPLACE, 0);

    auto particle = make_particle(0, "new name");
    file.writeDataset(particle, "singleParticle", types.particle_type);
    file.writeAttribute(particle, "singleParticle", "particleAttr", std::nullopt, types.particle_type);

    auto particle_read = file.readDataset<Particle>("singleParticle", std::nullopt, types.particle_type);
    auto attr_read     = file.readAttribute<Particle>("singleParticle", "particleAttr", std::nullopt, types.particle_type);

    REQUIRE(particle_read == particle);
    REQUIRE(attr_read == particle);

    auto info = file.getDatasetInfo("singleParticle");
    REQUIRE(info.h5Type);
    REQUIRE(H5Tget_class(info.h5Type.value()) == H5T_COMPOUND);
}

TEST_CASE("Vectors of explicit compound user types preserve record order and fields", "[user-type]") {
    auto       path  = make_path("userType-vector");
    auto       types = register_types();
    h5pp::v1::File file(path, h5pp::FileAccess::REPLACE, 0);

    std::vector<Particle> particles;
    for(int idx = 0; idx < 10; ++idx) particles.emplace_back(make_particle(idx, h5pp::format("p-{}", idx)));

    file.writeDataset(particles, "particles", types.particle_type);
    auto particles_read = file.readDataset<std::vector<Particle>>("particles", std::nullopt, types.particle_type);

    REQUIRE(particles_read == particles);

    auto info = file.getDatasetInfo("particles");
    REQUIRE(info.dsetDims);
    REQUIRE(info.dsetDims.value() == std::vector<hsize_t>{particles.size()});
}

TEST_CASE("Resizable compound datasets accept appends when created with an explicit HDF5 type", "[user-type]") {
    auto       path  = make_path("userType-append");
    auto       types = register_types();
    h5pp::v1::File file(path, h5pp::FileAccess::REPLACE, 0);

    std::vector<Particle> first_batch;
    std::vector<Particle> second_batch;
    for(int idx = 0; idx < 4; ++idx) first_batch.emplace_back(make_particle(idx, h5pp::format("a-{}", idx)));
    for(int idx = 0; idx < 3; ++idx) second_batch.emplace_back(make_particle(idx + 10, h5pp::format("b-{}", idx)));

    file.writeDataset_chunked(first_batch,
                              "particles",
                              std::vector<hsize_t>{first_batch.size()},
                              std::vector<hsize_t>{first_batch.size()},
                              std::vector<hsize_t>{H5S_UNLIMITED},
                              types.particle_type);

    h5pp::Options options;
    options.linkPath = "particles";
    options.h5Type   = types.particle_type;
    file.appendToDataset(second_batch, 0, options);

    auto combined = file.readDataset<std::vector<Particle>>("particles", std::nullopt, types.particle_type);
    REQUIRE(combined.size() == first_batch.size() + second_batch.size());
    REQUIRE(std::equal(first_batch.begin(), first_batch.end(), combined.begin()));
    REQUIRE(std::equal(second_batch.begin(), second_batch.end(), combined.begin() + static_cast<long>(first_batch.size())));
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            return_code = session.applyCommandLine(argc, argv);
    if(return_code != 0) return return_code;
    return session.run();
}
