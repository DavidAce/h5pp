#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <string>
#include <string_view>

namespace {
    std::string make_path(std::string_view name) {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }
}

TEST_CASE("advanced file controls and file-handle guards are reachable", "[file-handle][advanced]") {
    h5pp::File file(make_path("advanced"), h5pp::FileAccess::REPLACE, 0);

    auto advanced = file.advanced();
    advanced.setCloseDegree(H5F_CLOSE_STRONG);
    advanced.vlenEnableReclaimsTracking();
    advanced.vlenDisableReclaimsTracking();
    advanced.vlenDropReclaims();

    {
        auto keepOuter = file.keepFileOpen();
        {
            auto keepInner = advanced.keepFileOpen();
            auto handle    = advanced.openFileHandle();
            REQUIRE(handle.valid());
            REQUIRE(handle.refcount() >= 4);
        }

        auto handle = advanced.openFileHandle();
        REQUIRE(handle.valid());
        REQUIRE(handle.refcount() >= 3);
    }

    file.setKeepFileOpened();
    {
        auto keep   = file.keepFileOpen();
        auto handle = advanced.openFileHandle();
        REQUIRE(handle.valid());
        REQUIRE(handle.refcount() >= 3);
    }
    file.setKeepFileClosed();

    file.writeDataset(42, "data/value");
    REQUIRE(file.readDataset<int>("data/value") == 42);
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    return session.run(argc, argv);
}
