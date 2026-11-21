#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>

namespace {
    h5pp::fs::path make_path(const char *name) {
        auto directory = h5pp::fs::path(H5PP_TEST_DIR) / "fileAccess" / name;
        h5pp::fs::remove_all(directory);
        h5pp::fs::create_directories(directory);
        return directory / "data.h5";
    }

    void write_value(const h5pp::fs::path &path, h5pp::FileAccess access, int value) {
        h5pp::File file(path, access, 0);
        file.dataset("value").write(value);
    }

    int read_value(const h5pp::fs::path &path) {
        h5pp::File file(path, h5pp::FileAccess::READONLY, 0);
        return file.dataset("value").read<int>();
    }
}

TEST_CASE("Renaming file collisions preserves each file", "[file-access]") {
    auto path = make_path("rename");
    write_value(path, h5pp::FileAccess::REPLACE, 1);
    for(int value : {2, 3}) {
        h5pp::File file(path, h5pp::FileAccess::RENAME, 0);
        auto       expected = path.parent_path() / h5pp::format("data-{}.h5", value - 1);
        REQUIRE(file.getFilePath() == h5pp::fs::absolute(expected));
        file.dataset("value").write(value);
    }
    CHECK(read_value(path) == 1);
    CHECK(read_value(path.parent_path() / "data-1.h5") == 2);
    CHECK(read_value(path.parent_path() / "data-2.h5") == 3);
}

TEST_CASE("Backing up file collisions preserves successive versions", "[file-access]") {
    auto path = make_path("backup");
    write_value(path, h5pp::FileAccess::REPLACE, 1);
    write_value(path, h5pp::FileAccess::BACKUP, 2);
    write_value(path, h5pp::FileAccess::BACKUP, 3);
    CHECK(read_value(path) == 3);
    CHECK(read_value(path.string() + ".bak_1") == 1);
    CHECK(read_value(path.string() + ".bak_2") == 2);
}

TEST_CASE("Rejecting a file collision leaves the original data unchanged", "[file-access]") {
    auto path = make_path("collision");
    write_value(path, h5pp::FileAccess::REPLACE, 7);
    REQUIRE_THROWS_WITH(h5pp::File(path, h5pp::FileAccess::COLLISION_FAIL, 0), Catch::Matchers::ContainsSubstring("COLLISION_FAIL"));
    CHECK(read_value(path) == 7);
}

TEST_CASE("Opening a missing file read-only does not create it", "[file-access]") {
    auto path = make_path("missing");
    REQUIRE_THROWS_WITH(h5pp::File(path, h5pp::FileAccess::READONLY, 0), Catch::Matchers::ContainsSubstring("does not exist"));
    CHECK_FALSE(h5pp::fs::exists(path));
}

namespace {
    template<typename T>
    concept CanCopyLink = requires(T link) { link.copyTo("copy"); };
    template<typename T>
    concept CanCopyFrom = requires(T link, const h5pp::hid::h5f &source) { link.copyFromLocation(source, "source"); };
    static_assert(CanCopyLink<h5pp::Link>);
    static_assert(not CanCopyLink<h5pp::ConstLink>);
    static_assert(not CanCopyFrom<h5pp::ConstLink>);
}

TEST_CASE("Read-only objects reject writes while a writable file handle exists", "[file-access]") {
    h5pp::File writer(make_path("readonly-shared"), h5pp::FileAccess::REPLACE, 0);
    auto       token = writer.keepFileOpen();
    writer.dataset("values").write(7);
    h5pp::hid::h5t rowType = H5Tcreate(H5T_COMPOUND, sizeof(int));
    REQUIRE(H5Tinsert(rowType, "value", 0, H5T_NATIVE_INT) >= 0);
    auto table = writer.table("rows").create(rowType, "rows");
    table.appendRecords(std::vector<int>{1});
    h5pp::File reader(writer.getFilePath(), h5pp::FileAccess::READONLY, 0);
    CHECK_THROWS(reader.group("new-group").create());
    CHECK_THROWS(reader.table("rows").appendRecords(std::vector<int>{2}));
    CHECK_THROWS(reader.table("rows").writeRecords(std::vector<int>{2}));
    CHECK_THROWS(reader.table("absent").create(rowType, "absent"));
    CHECK_THROWS(reader.table("rows").appendRecordsFrom(table.getInfo()));
    CHECK_THROWS(reader.group("/").table("rows").appendRecords(std::vector<int>{2}));
    CHECK_THROWS(reader.link("rows").asTable().appendRecords(std::vector<int>{2}));
    CHECK_THROWS(reader.link("values").deleteLink());
    CHECK_THROWS(reader.link("values").copyTo("copy"));
    CHECK_THROWS(reader.link("values").moveTo("moved"));
    CHECK_THROWS(reader.link("values").createSoftLink("soft"));
    CHECK_THROWS(reader.link("absent").copyFromLocation(writer.openFileHandle(), "values"));
    CHECK(table.readRecords<std::vector<int>>() == std::vector<int>{1});
    CHECK(writer.dataset("values").read<int>() == 7);
    CHECK_FALSE(writer.link("copy").exists());
    CHECK_FALSE(writer.link("absent").exists());
}

int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
