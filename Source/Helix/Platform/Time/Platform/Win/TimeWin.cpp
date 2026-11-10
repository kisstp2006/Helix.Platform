#include <Helix/Platform/Time/Time.h>

#include <windows.h>

namespace helix::platform
{
    namespace
    {
        LARGE_INTEGER Frequency()
        {
            static LARGE_INTEGER f = [] {
                LARGE_INTEGER v;
                QueryPerformanceFrequency(&v);
                return v;
            }();
            return f;
        }
    }

    u64 Ticks()
    {
        LARGE_INTEGER v;
        QueryPerformanceCounter(&v);
        return u64(v.QuadPart);
    }

    u64 TicksPerSecond() { return u64(Frequency().QuadPart); }

    void SleepSeconds(f64 seconds)
    {
        if (seconds <= 0.0)
            return;
        static thread_local HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        if (timer)
        {
            LARGE_INTEGER due;
            due.QuadPart = -LONGLONG(seconds * 1e7);
            if (SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
            {
                WaitForSingleObject(timer, INFINITE);
                return;
            }
        }
        Sleep(DWORD(seconds * 1000.0));
    }
}
