#include <catch2/catch_all.hpp>
#include <h5pp/h5pp.h>
#include <string>
#include <string_view>
#include <vector>

template<typename Handle>
concept CanWriteDatasetHandle = requires(Handle handle) {
    handle.write(1);
};

template<typename Handle>
concept CanReadDatasetHandle = requires(Handle handle) {
    handle.template read<int>();
};

template<typename Handle>
concept CanWriteAttributeHandle = requires(Handle handle) {
    handle.write(1);
};

template<typename Handle>
concept CanReadAttributeHandle = requires(Handle handle) {
    handle.template read<int>();
};

namespace {
    struct Record {
        int    id    = 0;
        double value = 0;
    };

    template<typename Handle>
    concept CanAppendTableHandle = requires(Handle handle, const std::vector<Record> &records) {
        handle.appendRecords(records);
    };

    template<typename Handle>
    concept CanReadTableHandle = requires(Handle handle) {
        handle.template readRecords<Record>();
        handle.template readRecords<std::vector<Record>>(h5pp::TableSelection::ALL);
    };

    std::string make_path(std::string_view name) {
        h5pp::fs::create_directories(H5PP_TEST_DIR);
        return h5pp::format(H5PP_TEST_DIR "{}.h5", name);
    }

    h5pp::hid::h5t makeRecordType() {
        h5pp::hid::h5t H5_RECORD_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(Record));
        H5Tinsert(H5_RECORD_TYPE, "id", HOFFSET(Record, id), H5T_NATIVE_INT);
        H5Tinsert(H5_RECORD_TYPE, "value", HOFFSET(Record, value), H5T_NATIVE_DOUBLE);
        return H5_RECORD_TYPE;
    }
}

static_assert(requires(h5pp::File &file) {
    file.dataset("path").write(1);
    file.attribute("path", "attr").write(1);
});

static_assert(requires(const h5pp::File &file) {
    file.dataset("path").read<int>();
    file.attribute("path", "attr").read<int>();
    file.table("path").readRecords<Record>();
});

static_assert(CanWriteDatasetHandle<h5pp::Dataset>);
static_assert(not CanWriteDatasetHandle<h5pp::ConstDataset>);
static_assert(CanReadDatasetHandle<h5pp::Dataset>);
static_assert(CanReadDatasetHandle<h5pp::ConstDataset>);

static_assert(CanWriteAttributeHandle<h5pp::Attribute>);
static_assert(not CanWriteAttributeHandle<h5pp::ConstAttribute>);
static_assert(CanReadAttributeHandle<h5pp::Attribute>);
static_assert(CanReadAttributeHandle<h5pp::ConstAttribute>);

static_assert(CanAppendTableHandle<h5pp::Table>);
static_assert(not CanAppendTableHandle<h5pp::ConstTable>);
static_assert(CanReadTableHandle<h5pp::Table>);
static_assert(CanReadTableHandle<h5pp::ConstTable>);

TEST_CASE("const file exposes read-only dataset and attribute handles", "[const][dataset][attribute]") {
    h5pp::File file(make_path("const-handles"), h5pp::FileAccess::REPLACE, 0);

    file.writeDataset(std::vector<int>{1, 2, 3}, "data/vector");
    file.writeAttribute("data/vector", "unit", std::string("arb"));

    const h5pp::File &constFile = file;
    auto              data      = constFile.dataset("data/vector").read<std::vector<int>>();
    auto              unit      = constFile.attribute("data/vector", "unit").read<std::string>();

    REQUIRE(data == std::vector<int>{1, 2, 3});
    REQUIRE(unit == "arb");
}

TEST_CASE("const file exposes read-only table handles", "[const][table]") {
    h5pp::File file(make_path("const-table"), h5pp::FileAccess::REPLACE, 0);

    auto table = file.table("tables/records").create(makeRecordType(), "Records");
    table.appendRecords(std::vector<Record>{{1, 1.5}, {2, 2.5}, {3, 3.5}});

    const h5pp::File &constFile = file;
    auto              constInfo = constFile.table("tables/records").getInfo();
    auto              constData = constFile.table("tables/records").readRecords<std::vector<Record>>(0, 3);

    REQUIRE(constInfo.numRecords);
    REQUIRE(constInfo.numRecords.value() == 3);
    REQUIRE(constData.size() == 3);
    REQUIRE(constData[1].id == 2);
    REQUIRE(constFile.table("tables/records").fieldExists(std::vector<std::string>{"id", "value"}));
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    return session.run(argc, argv);
}
