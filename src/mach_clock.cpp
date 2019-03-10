static const struct ticks_to_nanosecond_ratio
{
    mach_timebase_info_data_t tb;
    ticks_to_nanosecond_ratio() { mach_timebase_info(&tb); }
} timebase_info;

static int64_t ticks_per_nanosecond_scaled()
{
    return (1ll << mach_absolute_time_clock::fixed_point_scale) * timebase_info.tb.denom / timebase_info.tb.numer;
}

static int64_t nanoseconds_per_tick_scaled()
{
    return (1ll << mach_absolute_time_clock::fixed_point_scale) * timebase_info.tb.numer / timebase_info.tb.denom;
}

const int64_t mach_absolute_time_clock::ticks_per_nanosecond_scaled = ::ticks_per_nanosecond_scaled();
const int64_t mach_absolute_time_clock::nanoseconds_per_tick_scaled = ::nanoseconds_per_tick_scaled();

