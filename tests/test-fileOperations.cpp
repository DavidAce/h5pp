#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>

namespace {
    h5pp::fs::path make_path(const char *name) {
        auto directory = h5pp::fs::path(H5PP_TEST_DIR) / "fileOperations" / name;
        h5pp::fs::remove_all(directory);
        h5pp::fs::create_directories(directory);
        return h5pp::fs::absolute(directory / "source.h5");
    }
}

TEST_CASE("Copying and moving preserve root attributes", "[file-operations]") {
    auto       source = make_path("root-attributes");
    h5pp::File file(source, h5pp::FileAccess::REPLACE, 0);
    file.dataset("group/values").write(7);
    file.attribute("/", "units").write(std::string("meters"));
    file.attribute("/", "names").write(std::vector<std::string>{"first", "second"});
    file.attribute("/", "version").write(2);
    file.attribute("/", "shape").write(std::vector<int>{2, 3});
    struct Record {
        int    number;
        double value;
    };
    h5pp::hid::h5t type = H5Tcreate(H5T_COMPOUND, sizeof(Record));
    REQUIRE(H5Tinsert(type, "number", HOFFSET(Record, number), H5T_NATIVE_INT) >= 0);
    REQUIRE(H5Tinsert(type, "value", HOFFSET(Record, value), H5T_NATIVE_DOUBLE) >= 0);
    file.attribute("/", "record").write(Record{3, 1.5}, {.h5Type = type});
    struct DynamicRecord {
        int   number;
        hvl_t values;
    };
    h5pp::hid::h5t arrayType   = H5Tvlen_create(H5T_NATIVE_INT);
    h5pp::hid::h5t dynamicType = H5Tcreate(H5T_COMPOUND, sizeof(DynamicRecord));
    REQUIRE(H5Tinsert(dynamicType, "number", HOFFSET(DynamicRecord, number), H5T_NATIVE_INT) >= 0);
    REQUIRE(H5Tinsert(dynamicType, "values", HOFFSET(DynamicRecord, values), arrayType) >= 0);
    int values[]{4, 5};
    file.attribute("/", "dynamic")
        .write(
            DynamicRecord{
                6,
                {2, values}
    },
            {.h5Type = dynamicType});
    h5pp::fs::path destination;
    SECTION("Copy") { destination = file.copyFileTo(source.parent_path() / "copy.h5"); }
    SECTION("Move") { destination = file.moveFileTo(source.parent_path() / "moved.h5"); }
    h5pp::File result(destination, h5pp::FileAccess::READONLY, 0);
    CHECK(result.attribute("/", "units").read<std::string>() == "meters");
    CHECK(result.attribute("/", "names").read<std::vector<std::string>>() == std::vector<std::string>{"first", "second"});
    CHECK(result.attribute("/", "version").read<int>() == 2);
    CHECK(result.attribute("/", "shape").read<std::vector<int>>() == std::vector<int>{2, 3});
    CHECK(H5Tequal(result.attribute("/", "record").getInfo().h5Type.value(), type) > 0);
    auto record = result.attribute("/", "record").read<Record>();
    CHECK(record.number == 3);
    CHECK(record.value == 1.5);
    CHECK(result.dataset("group/values").read<int>() == 7);
    auto           handle = result.openFileHandle();
    h5pp::hid::h5a attr   = H5Aopen_by_name(handle, "/", "dynamic", H5P_DEFAULT, H5P_DEFAULT);
    h5pp::hid::h5s space  = H5Aget_space(attr);
    DynamicRecord  dynamic{};
    REQUIRE(H5Aread(attr, dynamicType, &dynamic) >= 0);
    CHECK(dynamic.number == 6);
    REQUIRE(dynamic.values.len == 2);
    CHECK(static_cast<int *>(dynamic.values.p)[1] == 5);
#if H5_VERSION_GE(1, 12, 0)
    CHECK(H5Treclaim(dynamicType, space, H5P_DEFAULT, &dynamic) >= 0);
#else
    CHECK(H5Dvlen_reclaim(dynamicType, space, H5P_DEFAULT, &dynamic) >= 0);
#endif
}

TEST_CASE("Copying or moving onto the source preserves its contents", "[file-operations]") {
    auto       source = make_path("same-path");
    h5pp::File file(source, h5pp::FileAccess::REPLACE, 0);
    file.dataset("values").write(7);
    auto target = source;
    SECTION("Identical path") {}
    SECTION("Normalized path") { target = source.parent_path() / "." / source.filename(); }
    SECTION("Hard link") {
        target = source.parent_path() / "alias.h5";
        std::error_code error;
        h5pp::fs::create_hard_link(source, target, error);
        if(error) SKIP("Hard links are unavailable on this filesystem");
    }
    CHECK_THROWS_WITH(file.copyFileTo(target, h5pp::FileAccess::REPLACE), Catch::Matchers::ContainsSubstring("same file"));
    CHECK_THROWS_WITH(file.moveFileTo(target, h5pp::FileAccess::REPLACE), Catch::Matchers::ContainsSubstring("same file"));
    CHECK(file.dataset("values").read<int>() == 7);
}

TEST_CASE("An invalid copy source does not replace the destination", "[file-operations]") {
    auto       destination = make_path("missing-source");
    h5pp::File file(destination, h5pp::FileAccess::REPLACE, 0);
    file.dataset("values").write(7);
    CHECK_THROWS(h5pp::hdf5::copyFile(destination.parent_path() / "missing.h5", destination, h5pp::FileAccess::REPLACE));
    CHECK(file.dataset("values").read<int>() == 7);
}

TEST_CASE("Open handles prevent moves without changing either file", "[file-operations]") {
    auto       source = make_path("open-handles");
    auto       target = source.parent_path() / "target.h5";
    h5pp::File file(source, h5pp::FileAccess::REPLACE, 0);
    file.dataset("group/values").write(7);
    h5pp::File(target, h5pp::FileAccess::REPLACE, 0).dataset("sentinel").write(9);
    auto check = [&](const std::string &expected) {
        try {
            auto moved = file.moveFileTo(target, h5pp::FileAccess::REPLACE);
            FAIL("Move succeeded despite outstanding handles: " << moved.string());
        } catch(const std::runtime_error &error) {
            auto message = std::string(error.what());
            CHECK_THAT(message, Catch::Matchers::ContainsSubstring(source.string()));
            CHECK_THAT(message, Catch::Matchers::ContainsSubstring(expected));
            CHECK_THAT(message, Catch::Matchers::ContainsSubstring("references"));
        }
        CHECK(file.getFilePath() == source);
        CHECK(file.dataset("group/values").read<int>() == 7);
        CHECK(h5pp::File(target, h5pp::FileAccess::READONLY, 0).dataset("sentinel").read<int>() == 9);
    };
    SECTION("Keep-open token") {
        auto token = file.keepFileOpen();
        check("file");
    }
    SECTION("Dataset object") {
        auto dataset = file.dataset("group/values");
        check("file");
    }
    SECTION("Group object") {
        auto group = file.group("group");
        check("file");
    }
    SECTION("Copied cached file") {
        file.setKeepFileOpened();
        auto other = file;
        check("file");
    }
    SECTION("Raw dataset identifier") {
        file.setCloseDegree(H5F_CLOSE_WEAK);
        h5pp::hid::h5d dataset = H5Dopen2(file.openFileHandle(), "group/values", H5P_DEFAULT);
        check("dataset [/group/values]");
    }
    SECTION("Raw group identifier") {
        file.setKeepFileOpened();
        h5pp::hid::h5g group = H5Gopen2(file.openFileHandle(), "group", H5P_DEFAULT);
        check("group [/group]");
    }
    SECTION("Unrelated open file") {
        auto unrelated = h5pp::File(target, h5pp::FileAccess::READONLY, 0).dataset("sentinel");
        auto moved     = file.moveFileTo(source.parent_path() / "other.h5");
        CHECK(moved.filename() == "other.h5");
        CHECK(unrelated.read<int>() == 9);
    }
    SECTION("Released token") {
        {
            auto token = file.keepFileOpen();
            check("file");
        }
        auto moved = file.moveFileTo(target, h5pp::FileAccess::REPLACE);
        CHECK_FALSE(h5pp::fs::exists(source));
        CHECK(moved == target);
    }
}

TEST_CASE("Moving releases the File cache and subsequent writes reach the destination", "[file-operations]") {
    auto       source = make_path("cache");
    h5pp::File file(source, h5pp::FileAccess::REPLACE, 0);
    file.setKeepFileOpened();
    file.dataset("values").write(1);
    auto moved = file.moveFileTo(source.parent_path() / "moved.h5");
    file.dataset("values").write(2);
    CHECK_FALSE(h5pp::fs::exists(source));
    CHECK(h5pp::File(moved, h5pp::FileAccess::READONLY, 0).dataset("values").read<int>() == 2);
}

TEST_CASE("File copies skip reference-valued root attributes and continue", "[file-operations][references]") {
    auto       source = make_path("root-references");
    h5pp::File file(source, h5pp::FileAccess::REPLACE, 0);
    file.dataset("values").write(std::vector<int>{1, 2, 3});
    file.attribute("/", "units").write(std::string("meters"));
    {
        auto           handle = file.openFileHandle();
        h5pp::hid::h5s scalar = H5Screate(H5S_SCALAR);
        hobj_ref_t     objectRef{};
        REQUIRE(H5Rcreate(&objectRef, handle, "values", H5R_OBJECT, -1) >= 0);
        h5pp::hid::h5a objectAttr = H5Acreate2(handle, "dataset-ref", H5T_STD_REF_OBJ, scalar, H5P_DEFAULT, H5P_DEFAULT);
        REQUIRE(H5Awrite(objectAttr, H5T_STD_REF_OBJ, &objectRef) >= 0);
        h5pp::hid::h5d dataset = H5Dopen2(handle, "values", H5P_DEFAULT);
        h5pp::hid::h5s region  = H5Dget_space(dataset);
        hsize_t        offset = 1, extent = 1;
        REQUIRE(H5Sselect_hyperslab(region, H5S_SELECT_SET, &offset, nullptr, &extent, nullptr) >= 0);
        hdset_reg_ref_t regionRef{};
        REQUIRE(H5Rcreate(&regionRef, handle, "values", H5R_DATASET_REGION, region) >= 0);
        h5pp::hid::h5a regionAttr = H5Acreate2(handle, "region-ref", H5T_STD_REF_DSETREG, scalar, H5P_DEFAULT, H5P_DEFAULT);
        REQUIRE(H5Awrite(regionAttr, H5T_STD_REF_DSETREG, &regionRef) >= 0);
        struct Record {
            int        value;
            hobj_ref_t reference;
        } record{7, objectRef};
        h5pp::hid::h5t type = H5Tcreate(H5T_COMPOUND, sizeof(Record));
        REQUIRE(H5Tinsert(type, "value", HOFFSET(Record, value), H5T_NATIVE_INT) >= 0);
        REQUIRE(H5Tinsert(type, "reference", HOFFSET(Record, reference), H5T_STD_REF_OBJ) >= 0);
        h5pp::hid::h5a recordAttr = H5Acreate2(handle, "record-ref", type, scalar, H5P_DEFAULT, H5P_DEFAULT);
        REQUIRE(H5Awrite(recordAttr, type, &record) >= 0);
    }
    h5pp::fs::path destination;
    SECTION("Copy") { destination = file.copyFileTo(source.parent_path() / "copy.h5"); }
    SECTION("Move") { destination = file.moveFileTo(source.parent_path() / "moved.h5"); }
    h5pp::File result(destination, h5pp::FileAccess::READONLY, 0);
    CHECK_FALSE(result.attribute("/", "dataset-ref").exists());
    CHECK_FALSE(result.attribute("/", "region-ref").exists());
    CHECK_FALSE(result.attribute("/", "record-ref").exists());
    CHECK(result.attribute("/", "units").read<std::string>() == "meters");
    CHECK(result.dataset("values").read<std::vector<int>>() == std::vector<int>{1, 2, 3});
}

int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
