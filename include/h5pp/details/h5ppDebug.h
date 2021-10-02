#pragma once
namespace h5pp{
#ifdef NDEBUG
    inline constexpr bool ndebug = true;
#else
    inline constexpr bool ndebug = false;
#endif
}