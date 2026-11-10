#pragma once

#include <Helix/Core/Basics/Types.h>

namespace helix::platform
{
    u64 Ticks();
    u64 TicksPerSecond();
    f64 Now();
    void SleepSeconds(f64 seconds);

    class Stopwatch
    {
    public:
        Stopwatch() : start_(Ticks()) {}
        void Restart() { start_ = Ticks(); }
        f64 ElapsedSeconds() const { return f64(Ticks() - start_) / f64(TicksPerSecond()); }
        f64 ElapsedMilliseconds() const { return ElapsedSeconds() * 1000.0; }

    private:
        u64 start_;
    };
}
