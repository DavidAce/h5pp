#include <h5pp/h5pp.h>


int main(){
    static_assert(std::is_destructible_v<h5pp::File>);
    static_assert(std::is_default_constructible_v<h5pp::File>);
    static_assert(std::is_copy_constructible_v<h5pp::File>);
    static_assert(std::is_copy_assignable_v<h5pp::File>);
    static_assert(std::is_move_constructible_v<h5pp::File>);
    static_assert(std::is_move_assignable_v<h5pp::File>);

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

}