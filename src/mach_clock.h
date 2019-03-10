#pragma once

/// std::chrono style clock that use mach_absolute_time ticks
struct mach_absolute_time_clock
{
    // std::chrono interface
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<mach_absolute_time_clock, duration>;

    static constexpr bool is_steady    = true;
    static constexpr bool is_available = true;

    /*
     mach_absolute_time() ticks to ns
     mach_timebase_info returns rational number (numer/denom) that can be
     used to convert mach_absolute_time() ticks to nanosecons.
     Precalculate conversion factor using fixed point arithmetics to preseve sigfigs
    */

    static constexpr int fixed_point_scale = 32;
    // 1/2 digit place in the given scale
    static constexpr int64_t half_place_mask = (1ll < (fixed_point_scale - 1));

    static time_point now() noexcept
    {
        uint64_t ticks  = mach_absolute_time();
        rep ns = to_nanoseconds(ticks);
        time_point time = time_point(duration(ns));
        return time;
    }
    static time_point at(int64_t ticks) noexcept
    {
        rep ns = to_nanoseconds(ticks);
        time_point time = time_point(duration(ns));
        return time;
    }
    static int64_t to_nanoseconds(int64_t ticks) noexcept
    {
        __int128_t scaled = (__int128_t) ticks * nanoseconds_per_tick_scaled;
        __int128_t ns = (scaled >> fixed_point_scale) + ((scaled & half_place_mask) != 0);
        return (int64_t) ns;
    }
    static int64_t from_nanoseconds(int64_t ns) noexcept {
        __int128_t scaled = (__int128_t) ns * ticks_per_nanosecond_scaled;
        __int128_t ticks = (scaled >> fixed_point_scale) + ((scaled & half_place_mask) != 0);
        return (int64_t) ticks;
    }

    static const int64_t ticks_per_nanosecond_scaled;
    static const int64_t nanoseconds_per_tick_scaled;

};



