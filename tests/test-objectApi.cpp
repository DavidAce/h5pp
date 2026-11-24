#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
    template<typename T>
    concept WritesInt = requires(T object) { object.write(1); };

    template<typename T>
    concept WritesTable = requires(T object) { object.write(std::vector<int>{1}); };

    static_assert(std::is_same_v<decltype(std::declval<h5pp::Dataset>().write(1)), h5pp::Dataset>);
    static_assert(std::is_same_v<decltype(std::declval<h5pp::Attribute>().write(1)), h5pp::Attribute>);
    static_assert(std::is_same_v<decltype(std::declval<h5pp::Table>().append(std::vector<int>{1})), h5pp::Table>);
    static_assert(std::is_same_v<decltype(std::declval<h5pp::Group>().create()), h5pp::Group>);
    static_assert(not WritesInt<h5pp::ConstDataset>);
    static_assert(not WritesInt<h5pp::ConstAttribute>);
    static_assert(not WritesTable<h5pp::ConstTable>);
    static_assert(std::is_same_v<decltype(std::declval<h5pp::ConstTable>().attribute("unit")), h5pp::ConstAttribute>);

    std::string path(const char *name) {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::format(H5PP_TEST_DIR "object-api-{}.h5", name);
    }

    h5pp::hid::h5t record_type() {
        h5pp::hid::h5t type = H5Tcreate(H5T_COMPOUND, sizeof(int));
        if(H5Tinsert(type, "value", 0, H5T_NATIVE_INT) < 0) throw std::runtime_error("Cannot create record type");
        return type;
    }
}

TEST_CASE("Dataset and attribute writes return independent usable objects", "[object-api]") {
    h5pp::File file(path("dataset"), h5pp::FileAccess::REPLACE, 0);
    auto       group   = file.group("run").create();
    auto       dataset = group.dataset("values")
                             .create(std::vector<int>{1, 2}, {.h5Layout = H5D_CHUNKED})
                             .write(std::vector<int>{1, 2})
                             .append(std::vector<int>{3}, 0);
    CHECK(dataset.read<std::vector<int>>() == std::vector<int>{1, 2, 3});
    CHECK(dataset.getInfo().dsetDims.value() == std::vector<hsize_t>{3});
    auto attribute = dataset.attribute("unit").write(std::string("m"));
    CHECK(attribute.read<std::string>() == "m");
    CHECK(attribute.getName() == "unit");
    CHECK(attribute.getLinkPath() == dataset.getPath());
    CHECK(attribute.getInfo().attrName.value() == "unit");
    CHECK(dataset.getAttributeNames() == std::vector<std::string>{"unit"});

    auto selected = dataset.select({1}, {1}).write(std::vector<int>{7});
    CHECK(selected.read<std::vector<int>>() == std::vector<int>{7});
    CHECK(dataset.read<std::vector<int>>() == std::vector<int>{1, 7, 3});
    CHECK(dataset.resize({4}).getInfo().dsetDims.value() == std::vector<hsize_t>{4});

    CHECK(dataset.getPath() == "run/values");
    CHECK(dataset.getName() == "values");
    CHECK(dataset.getParentPath() == "run");
    CHECK(dataset.getFilePath() == file.getFilePath());
    CHECK(attribute.getFilePath() == file.getFilePath());
    CHECK(group.getFileName() == file.getFileName());
    CHECK(dataset.getFileAccess() == file.getFileAccess());
    auto named = group.dataset(R"(back\slash)").write(4);
    CHECK(named.getName() == R"(back\slash)");
    CHECK(named.getParentPath() == "run");

    CHECK(group.attribute("iteration").write(1).write(2).read<int>() == 2);
    CHECK(file.attribute("description").write(std::string("root")).read<std::string>() == "root");
    const auto &reader = file;
    CHECK(reader.attribute("description").read<std::string>() == "root");
    CHECK(reader.dataset("run/values").attribute("unit").read<std::string>() == "m");
}

TEST_CASE("Tables support the same write append read and attribute chains", "[object-api]") {
    h5pp::File file(path("table"), h5pp::FileAccess::REPLACE, 0);
    auto       table =
        file.group("run").create().table("rows").create(record_type(), "rows").write(std::vector<int>{1, 2}).append(std::vector<int>{3});
    CHECK(table.read<std::vector<int>>() == std::vector<int>{1, 2, 3});
    CHECK(table.read<int>(h5pp::TableSelection::LAST) == 3);
    std::vector<int> values;
    table.readInto(values);
    CHECK(values == std::vector<int>{1, 2, 3});
    table.readInto(values, h5pp::TableSelection::FIRST);
    CHECK(values == std::vector<int>{1});
    auto attr = table.attribute("unit").write(std::string("count"));
    CHECK(attr.read<std::string>() == "count");
    CHECK(table.getAttributeNames().size() >= 1);
    CHECK(table.findAttributes("unit") == std::vector<std::string>{"unit"});
    CHECK_NOTHROW(table.getTypeInfo());
    CHECK(table.getPath() == "run/rows");
    CHECK(table.getName() == "rows");
    CHECK(table.getParentPath() == "run");
    CHECK(table.getFilePath() == file.getFilePath());
    CHECK(attr.getFileName() == file.getFileName());
    auto copied = file.table("copy").appendRecordsFrom(table.getInfo()).append(std::vector<int>{4});
    CHECK(copied.read<std::vector<int>>() == std::vector<int>{1, 2, 3, 4});
    CHECK(copied.getInfo().numRecords.value() == 4);
    const auto &reader = file;
    CHECK(reader.table("run/rows").attribute("unit").read<std::string>() == "count");
    CHECK(reader.table("run/rows").read<std::vector<int>>() == std::vector<int>{1, 2, 3});
}

TEST_CASE("Returned groups and links keep relative navigation and search", "[object-api]") {
    h5pp::File file(path("group"), h5pp::FileAccess::REPLACE, 0);
    auto       root  = file.group("run").create();
    auto       child = root.createGroup("child");
    child.dataset("values").write(42);
    CHECK(child.getPath() == "run/child");
    CHECK(child.getParentPath() == "run");
    CHECK(child.findDatasets("value") == std::vector<std::string>{"values"});
    CHECK(root.findGroups("child") == std::vector<std::string>{"child"});
    auto alias = root.createSoftLink("child/values", "alias");
    CHECK(alias.getPath() == "run/alias");
    CHECK(alias.getFilePath() == file.getFilePath());
    CHECK(alias.getName() == "alias");
    CHECK(alias.asDataset().read<int>() == 42);
    CHECK(alias.attribute("label").write(std::string("shared")).read<std::string>() == "shared");
    CHECK(child.dataset("values").attribute("label").read<std::string>() == "shared");
}

TEST_CASE("Objects returned by writes survive their File", "[object-api][lifetime]") {
    auto dataset = h5pp::File(path("temporary"), h5pp::FileAccess::REPLACE, 0).dataset("values").write(12);
    CHECK(dataset.read<int>() == 12);
    CHECK(dataset.getFileName() == "object-api-temporary.h5");
    auto attr = dataset.attribute("unit").write(1);
    dataset   = h5pp::File(path("other"), h5pp::FileAccess::REPLACE, 0).dataset("other").write(3);
    CHECK(attr.read<int>() == 1);
    CHECK(attr.getFileName() == "object-api-temporary.h5");
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    return session.run(argc, argv);
}
