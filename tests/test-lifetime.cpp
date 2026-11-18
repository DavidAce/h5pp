#include <catch2/catch_all.hpp>
#include <cstdlib>
#include <h5pp/h5pp.h>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {
    std::string make_path(const char *name) {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::format(H5PP_TEST_DIR "lifetime-{}.h5", name);
    }
}

TEST_CASE("Objects and their children remain usable after File destruction", "[lifetime]") {
    auto [dataset, attribute, table, group, link] = [] {
        h5pp::File file(make_path("objects"), h5pp::FileAccess::REPLACE, 0);
        file.dataset("group/values").write(std::vector<int>{1, 2, 3});
        file.attribute("group/values", "unit").write(std::string("m"));
        h5pp::hid::h5t rowType = H5Tcreate(H5T_COMPOUND, sizeof(int));
        REQUIRE(H5Tinsert(rowType, "value", 0, H5T_NATIVE_INT) >= 0);
        auto table = file.table("group/rows").ensure(rowType, "rows");
        return std::tuple{file.dataset("group/values"),
                          file.attribute("group/values", "unit"),
                          table,
                          file.group("group"),
                          file.link("group/values")};
    }();

    CHECK(dataset.read<std::vector<int>>() == std::vector<int>{1, 2, 3});
    CHECK(attribute.read<std::string>() == "m");
    CHECK(group.dataset("values").select({1}, {1}).read<std::vector<int>>() == std::vector<int>{2});
    CHECK(link.asDataset().read<std::vector<int>>() == std::vector<int>{1, 2, 3});
    CHECK(link.attribute("unit").read<std::string>() == "m");
    CHECK(group.link("values").asDataset().attribute("unit").read<std::string>() == "m");
    CHECK(group.link("/").asGroup().dataset("values").exists());
    table.appendRecords(std::vector<int>{7, 8});
    CHECK(table.readRecords<std::vector<int>>() == std::vector<int>{7, 8});
    CHECK(group.table("rows").readRecords<std::vector<int>>() == std::vector<int>{7, 8});
    group.group("child").create();
    CHECK(group.group("child").exists());
    dataset.attribute("description").write(std::string("retained"));
    CHECK(dataset.attribute("description").read<std::string>() == "retained");
}

TEST_CASE("Objects obtained from temporary files own their file handle", "[lifetime]") {
    auto dataset = h5pp::File(make_path("temporary"), h5pp::FileAccess::REPLACE, 0).dataset("values");
    dataset.write(42);
    auto copy  = dataset;
    auto moved = std::move(dataset);
    CHECK(copy.read<int>() == 42);
    moved.write(43);
    CHECK(copy.read<int>() == 43);
}

TEST_CASE("Objects keep their original file through assignment, moves and swaps", "[lifetime]") {
    auto       firstPath  = make_path("first");
    auto       secondPath = make_path("second");
    h5pp::File first(firstPath, h5pp::FileAccess::REPLACE, 0);
    h5pp::File second(secondPath, h5pp::FileAccess::REPLACE, 0);
    first.dataset("value").write(1);
    second.dataset("value").write(2);
    auto dataset = first.dataset("value");
    SECTION("assignment") { first = second; }
    SECTION("move") { auto moved = std::move(first); }
    SECTION("swap") { std::swap(first, second); }
    dataset.write(99);
    CHECK(h5pp::File(firstPath, h5pp::FileAccess::READONLY, 0).dataset("value").read<int>() == 99);
    CHECK(h5pp::File(secondPath, h5pp::FileAccess::READONLY, 0).dataset("value").read<int>() == 2);
}

TEST_CASE("Objects capture compression defaults and preserve read-only access", "[lifetime]") {
    auto dataset = [] {
        h5pp::File file(make_path("settings"), h5pp::FileAccess::REPLACE, 0);
        file.setCompressionLevel(1);
        auto dataset = file.dataset("values");
        file.setCompressionLevel(9);
        return dataset;
    }();
    dataset.create(std::vector<int>{1, 2}, {.dimsChunk = std::vector<hsize_t>{2}}).write(std::vector<int>{1, 2});
    CHECK(h5pp::hdf5::getDeflateLevel(dataset.getInfo().h5DsetCreate.value()) == 1);
    auto readonly = h5pp::File(make_path("settings"), h5pp::FileAccess::READONLY, 0).dataset("values");
    REQUIRE_THROWS(readonly.write(0));
    CHECK(readonly.read<std::vector<int>>() == std::vector<int>{1, 2});
    auto constant = [] {
        const h5pp::File file(make_path("settings"), h5pp::FileAccess::READONLY, 0);
        return file.dataset("values");
    }();
    CHECK(constant.read<std::vector<int>>() == std::vector<int>{1, 2});
}

TEST_CASE("The last owning object releases its HDF5 file handle", "[lifetime]") {
    hid_t id;
    {
        auto dataset = [&] {
            h5pp::File file(make_path("release"), h5pp::FileAccess::REPLACE, 0);
            file.setKeepFileOpened();
            id = file.advanced().openFileHandle();
            return file.dataset("value");
        }();
        REQUIRE(H5Iis_valid(id) > 0);
        dataset.write(42);
    }
    CHECK(H5Iis_valid(id) == 0);
}

TEST_CASE("Keep-open tokens can outlive File and each other", "[lifetime]") {
    std::optional<h5pp::File::KeepFileOpenToken> outer, inner;
    hid_t                                        id;
    {
        h5pp::File file(make_path("tokens"), h5pp::FileAccess::REPLACE, 0);
        outer.emplace(file);
        inner.emplace(file);
        auto handle = file.advanced().openFileHandle();
        id          = handle;
        CHECK(handle == file.advanced().openFileHandle());
    }
    auto moved = std::move(*outer);
    outer.reset();
    REQUIRE(H5Iis_valid(id) > 0);
    moved = std::move(*inner);
    inner.reset();
    CHECK(H5Iis_valid(id) > 0);
}

TEST_CASE("Saved advanced controls own a File copy", "[lifetime]") {
    auto controls = [] {
        h5pp::File file(make_path("advanced"), h5pp::FileAccess::REPLACE, 0);
        file.dataset("value").write(42);
        return file.advanced();
    }();
    CHECK(controls.openFileHandle().valid());
    controls.vlenDropReclaims();
    CHECK(controls.getLowLevelFile().dataset("value").read<int>() == 42);
}

TEST_CASE("File copies retain table allocation records for explicit reclamation", "[lifetime][vlen]") {
    struct Record {
        hvl_t values;
    };
    struct Counts {
        size_t allocated = 0, freed = 0;
    } counts;
    h5pp::File file(make_path("reclaim"), h5pp::FileAccess::REPLACE, 0);
    REQUIRE(H5Pset_vlen_mem_manager(
                file.plists.dsetXfer,
                [](size_t size, void *context) -> void * {
                    ++static_cast<Counts *>(context)->allocated;
                    return std::malloc(size);
                },
                &counts,
                [](void *memory, void *context) {
                    ++static_cast<Counts *>(context)->freed;
                    std::free(memory);
                },
                &counts) >= 0);
    h5pp::hid::h5t arrayType = H5Tvlen_create(H5T_NATIVE_INT);
    h5pp::hid::h5t rowType   = H5Tcreate(H5T_COMPOUND, sizeof(Record));
    REQUIRE(H5Tinsert(rowType, "values", HOFFSET(Record, values), arrayType) >= 0);
    auto table    = file.table("rows").create(rowType, "rows");
    int  values[] = {1, 2};
    table.appendRecords(std::vector<Record>{{{2, values}}});
    auto records = table.readRecords<std::vector<Record>>();
    REQUIRE(records.size() == 1);
    REQUIRE(records[0].values.len == 2);
    CHECK(static_cast<int *>(records[0].values.p)[1] == 2);
    REQUIRE(counts.allocated > 0);
    CHECK(counts.freed == 0);
    auto controls = file.advanced();
    controls.vlenReclaim();
    CHECK(counts.freed == counts.allocated);
    file.advanced().vlenReclaim();
    CHECK(counts.freed == counts.allocated);
}

int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
