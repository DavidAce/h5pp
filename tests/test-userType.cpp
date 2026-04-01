#include <algorithm>
#include <catch2/catch_all.hpp>
#include <cstring>
#include <h5pp/h5pp.h>
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
        std::memcpy(particle.name, name.data(), std::min(name.size(), sizeof(particle.name) - 1));
        particle.name[sizeof(particle.name) - 1] = '\0';
        return particle;
    }
}

TEST_CASE("Explicit compound user types round-trip as datasets and attributes", "[user-type]") {
    auto       path  = make_path("userType");
    auto       types = register_types();
    h5pp::File file(path, h5pp::FileAccess::REPLACE, 0);

    auto particle = make_particle(0, "new name");

    h5pp::DatasetCreateOptions dset_create;
    dset_create.h5Type = types.particle_type;
    file.writeDataset(particle, "singleParticle", dset_create);

    h5pp::AttributeWriteOptions attr_write;
    attr_write.h5Type = types.particle_type;
    file.writeAttribute("singleParticle", "particleAttr", particle, attr_write);

    h5pp::DatasetReadOptions dset_read;
    dset_read.h5Type = types.particle_type;
    h5pp::AttributeReadOptions attr_read_opts;
    attr_read_opts.h5Type = types.particle_type;

    auto particle_read = file.readDataset<Particle>("singleParticle", dset_read);
    auto attr_read     = file.readAttribute<Particle>("singleParticle", "particleAttr", attr_read_opts);

    REQUIRE(particle_read == particle);
    REQUIRE(attr_read == particle);

    auto info = file.getDatasetInfo("singleParticle");
    REQUIRE(info.h5Type);
    REQUIRE(H5Tget_class(info.h5Type.value()) == H5T_COMPOUND);
}

TEST_CASE("Vectors of explicit compound user types preserve record order and fields", "[user-type]") {
    auto       path  = make_path("userType-vector");
    auto       types = register_types();
    h5pp::File file(path, h5pp::FileAccess::REPLACE, 0);

    std::vector<Particle> particles;
    for(int idx = 0; idx < 10; ++idx) particles.emplace_back(make_particle(idx, h5pp::format("p-{}", idx)));

    h5pp::DatasetCreateOptions create;
    create.h5Type = types.particle_type;
    file.writeDataset(particles, "particles", create);

    h5pp::DatasetReadOptions read;
    read.h5Type = types.particle_type;
    auto particles_read = file.readDataset<std::vector<Particle>>("particles", read);

    REQUIRE(particles_read == particles);

    auto info = file.getDatasetInfo("particles");
    REQUIRE(info.dsetDims);
    REQUIRE(info.dsetDims.value() == std::vector<hsize_t>{particles.size()});
}

TEST_CASE("Resizable compound datasets accept appends when created with an explicit HDF5 type", "[user-type]") {
    auto       path  = make_path("userType-append");
    auto       types = register_types();
    h5pp::File file(path, h5pp::FileAccess::REPLACE, 0);

    std::vector<Particle> first_batch;
    std::vector<Particle> second_batch;
    for(int idx = 0; idx < 4; ++idx) first_batch.emplace_back(make_particle(idx, h5pp::format("a-{}", idx)));
    for(int idx = 0; idx < 3; ++idx) second_batch.emplace_back(make_particle(idx + 10, h5pp::format("b-{}", idx)));

    h5pp::DatasetCreateOptions create;
    create.dims      = {first_batch.size()};
    create.dimsChunk = {first_batch.size()};
    create.dimsMax   = {H5S_UNLIMITED};
    create.h5Layout  = H5D_CHUNKED;
    create.h5Type    = types.particle_type;
    file.writeDataset(first_batch, "particles", create);

    h5pp::DatasetAppendOptions append;
    append.h5Type = types.particle_type;
    [[maybe_unused]] auto info = file.dataset("particles").append(second_batch, 0, append);

    h5pp::DatasetReadOptions read;
    read.h5Type = types.particle_type;
    auto combined = file.readDataset<std::vector<Particle>>("particles", read);
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
