#pragma once

#include <type_traits>

namespace h5pp::inline v2::detail {
    template<typename FileType>
    concept MutableFile = not std::is_const_v<FileType>;
}
