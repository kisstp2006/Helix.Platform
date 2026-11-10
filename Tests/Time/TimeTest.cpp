#include <Helix/Platform/System/DynamicLibrary.h>
#include <Helix/Platform/IO/FileSystem.h>
#include <Helix/Platform/Threading/Thread.h>
#include <Helix/Platform/Time/Time.h>
#include <Helix/Platform/Window/Window.h>
#include <Helix/Core/Types/Guid.h>
#include <Helix/Core/Strings/Format.h>
#include <Helix/Testing/Test.h>

#include <atomic>
#include <cstdlib>

#define HX_CHECK_MSG_NO_DISPLAY() HX_CHECK(std::getenv("DISPLAY") == nullptr)

using namespace helix;
using namespace helix::platform;

HX_TEST(Time, MonotonicAndSleep)
{
    u64 a = Ticks();
    Stopwatch sw;
    SleepSeconds(0.02);
    u64 b = Ticks();
    HX_CHECK(b > a);
    f64 ms = sw.ElapsedMilliseconds();
    HX_CHECK(ms >= 15.0);
    HX_CHECK(ms < 500.0);
    HX_CHECK(TicksPerSecond() >= 1000000u);
}
