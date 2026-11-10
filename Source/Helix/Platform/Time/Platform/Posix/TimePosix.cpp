#include <Helix/Platform/Time/Time.h>

#include <errno.h>
#include <time.h>

#if HX_PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

namespace helix::platform
{
    u64 Ticks()
    {
#if HX_PLATFORM_WEB
        return u64(emscripten_get_now() * 1000.0);
#else
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return u64(ts.tv_sec) * 1000000000ull + u64(ts.tv_nsec);
#endif
    }

    u64 TicksPerSecond()
    {
#if HX_PLATFORM_WEB
        return 1000000ull;
#else
        return 1000000000ull;
#endif
    }

    void SleepSeconds(f64 seconds)
    {
        if (seconds <= 0.0)
            return;
        timespec ts;
        ts.tv_sec = time_t(seconds);
        ts.tv_nsec = long((seconds - f64(ts.tv_sec)) * 1e9);
        while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
        {
        }
    }
}
