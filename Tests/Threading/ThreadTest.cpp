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

HX_TEST(Thread, MutexProtectsCounter)
{
    if (!ThreadsSupported())
        return;
    struct Shared
    {
        Mutex mutex;
        i64 counter = 0;
    } shared;
    Thread threads[4];
    for (Thread& t : threads)
    {
        HX_REQUIRE(t.Start({.function = [](void* p) {
                                auto* s = static_cast<Shared*>(p);
                                for (int i = 0; i < 100000; i++)
                                {
                                    ScopedLock lock(s->mutex);
                                    s->counter++;
                                }
                            },
                            .user = &shared,
                            .name = "counter"}));
    }
    for (Thread& t : threads)
        t.Join();
    HX_CHECK_EQ(shared.counter, 400000);
}

HX_TEST(Thread, SemaphoreAndConditionVariable)
{
    if (!ThreadsSupported())
        return;
    struct Shared
    {
        Semaphore ready{0};
        Semaphore go{0};
        std::atomic<int> value{0};
    } shared;
    Thread t;
    HX_REQUIRE(t.Start({.function = [](void* p) {
                            auto* s = static_cast<Shared*>(p);
                            s->ready.Release();
                            s->go.Acquire();
                            s->value = 42;
                        },
                        .user = &shared}));
    shared.ready.Acquire();
    HX_CHECK_EQ(shared.value.load(), 0);
    shared.go.Release();
    t.Join();
    HX_CHECK_EQ(shared.value.load(), 42);

    Mutex m;
    ConditionVariable cv;
    ScopedLock lock(m);
    Stopwatch sw;
    HX_CHECK(!cv.WaitFor(m, 0.02));
    HX_CHECK(sw.ElapsedMilliseconds() >= 10.0);
    HX_CHECK(LogicalCoreCount() >= 1);
}
