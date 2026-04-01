#include <catch2/catch_all.hpp>
#include <h5pp/v1/h5pp.h>

static_assert(std::is_destructible_v<h5pp::v1::File>);
static_assert(std::is_default_constructible_v<h5pp::v1::File>);
static_assert(std::is_copy_constructible_v<h5pp::v1::File>);
static_assert(std::is_copy_assignable_v<h5pp::v1::File>);
static_assert(std::is_move_constructible_v<h5pp::v1::File>);
static_assert(std::is_move_assignable_v<h5pp::v1::File>);

static_assert(std::is_destructible_v<h5pp::hid::h5f>);
static_assert(std::is_default_constructible_v<h5pp::hid::h5f>);
static_assert(std::is_copy_constructible_v<h5pp::hid::h5f>);
static_assert(std::is_copy_assignable_v<h5pp::hid::h5f>);
static_assert(std::is_move_constructible_v<h5pp::hid::h5f>);
static_assert(std::is_move_assignable_v<h5pp::hid::h5f>);

static_assert(std::is_destructible_v<h5pp::hid::h5a>);
static_assert(std::is_default_constructible_v<h5pp::hid::h5a>);
static_assert(std::is_copy_constructible_v<h5pp::hid::h5a>);
static_assert(std::is_copy_assignable_v<h5pp::hid::h5a>);
static_assert(std::is_move_constructible_v<h5pp::hid::h5a>);
static_assert(std::is_move_assignable_v<h5pp::hid::h5a>);

static_assert(std::is_destructible_v<h5pp::hid::h5d>);
static_assert(std::is_default_constructible_v<h5pp::hid::h5d>);
static_assert(std::is_copy_constructible_v<h5pp::hid::h5d>);
static_assert(std::is_copy_assignable_v<h5pp::hid::h5d>);
static_assert(std::is_move_constructible_v<h5pp::hid::h5d>);
static_assert(std::is_move_assignable_v<h5pp::hid::h5d>);

static_assert(std::is_destructible_v<h5pp::hid::h5e>);
static_assert(std::is_default_constructible_v<h5pp::hid::h5e>);
static_assert(std::is_copy_constructible_v<h5pp::hid::h5e>);
static_assert(std::is_copy_assignable_v<h5pp::hid::h5e>);
static_assert(std::is_move_constructible_v<h5pp::hid::h5e>);
static_assert(std::is_move_assignable_v<h5pp::hid::h5e>);

static_assert(std::is_destructible_v<h5pp::hid::h5g>);
static_assert(std::is_default_constructible_v<h5pp::hid::h5g>);
static_assert(std::is_copy_constructible_v<h5pp::hid::h5g>);
static_assert(std::is_copy_assignable_v<h5pp::hid::h5g>);
static_assert(std::is_move_constructible_v<h5pp::hid::h5g>);
static_assert(std::is_move_assignable_v<h5pp::hid::h5g>);

static_assert(std::is_destructible_v<h5pp::hid::h5o>);
static_assert(std::is_default_constructible_v<h5pp::hid::h5o>);
static_assert(std::is_copy_constructible_v<h5pp::hid::h5o>);
static_assert(std::is_copy_assignable_v<h5pp::hid::h5o>);
static_assert(std::is_move_constructible_v<h5pp::hid::h5o>);
static_assert(std::is_move_assignable_v<h5pp::hid::h5o>);

static_assert(std::is_destructible_v<h5pp::hid::h5p>);
static_assert(std::is_default_constructible_v<h5pp::hid::h5p>);
static_assert(std::is_copy_constructible_v<h5pp::hid::h5p>);
static_assert(std::is_copy_assignable_v<h5pp::hid::h5p>);
static_assert(std::is_move_constructible_v<h5pp::hid::h5p>);
static_assert(std::is_move_assignable_v<h5pp::hid::h5p>);

static_assert(std::is_destructible_v<h5pp::hid::h5s>);
static_assert(std::is_default_constructible_v<h5pp::hid::h5s>);
static_assert(std::is_copy_constructible_v<h5pp::hid::h5s>);
static_assert(std::is_copy_assignable_v<h5pp::hid::h5s>);
static_assert(std::is_move_constructible_v<h5pp::hid::h5s>);
static_assert(std::is_move_assignable_v<h5pp::hid::h5s>);

static_assert(std::is_destructible_v<h5pp::hid::h5t>);
static_assert(std::is_default_constructible_v<h5pp::hid::h5t>);
static_assert(std::is_copy_constructible_v<h5pp::hid::h5t>);
static_assert(std::is_copy_assignable_v<h5pp::hid::h5t>);
static_assert(std::is_move_constructible_v<h5pp::hid::h5t>);
static_assert(std::is_move_assignable_v<h5pp::hid::h5t>);

TEST_CASE("Implicitly defined special members remain available for file and hid wrappers", "[implicit-defs]") {
    h5pp::v1::File file;
    REQUIRE(file.getFilePath().empty());

    h5pp::hid::h5f file_handle;
    h5pp::hid::h5d dset_handle;
    h5pp::hid::h5a attr_handle;
    h5pp::hid::h5g group_handle;
    h5pp::hid::h5o object_handle;
    h5pp::hid::h5s space_handle;
    h5pp::hid::h5t type_handle;
    h5pp::hid::h5p plist_handle;
    h5pp::hid::h5e error_handle;

    CHECK_FALSE(static_cast<bool>(file_handle));
    CHECK_FALSE(static_cast<bool>(dset_handle));
    CHECK_FALSE(static_cast<bool>(attr_handle));
    CHECK_FALSE(static_cast<bool>(group_handle));
    CHECK_FALSE(static_cast<bool>(object_handle));
    CHECK_FALSE(static_cast<bool>(space_handle));
    CHECK_FALSE(static_cast<bool>(type_handle));
    CHECK_FALSE(static_cast<bool>(plist_handle));
    CHECK_FALSE(static_cast<bool>(error_handle));
}

int main(int argc, char *argv[]) {
    Catch::Session session;
    int            returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) return returnCode;
    return session.run();
}
