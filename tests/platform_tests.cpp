#include <helix/platform/dynamic_library.h>
#include <helix/platform/file_system.h>
#include <helix/platform/thread.h>
#include <helix/platform/time.h>
#include <helix/platform/window.h>
#include <helix/core/guid/guid.h>
#include <helix/core/strings/format.h>
#include <helix/testing/test.h>

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

HX_TEST(FileSystem, WriteReadListRemove)
{
    String dir = JoinPath(TempDirectory(), Format("helix_test_{}", Guid::Generate().ToString()));
    String nested = JoinPath(dir, "a/b");
    HX_REQUIRE(CreateDirectories(nested));
    HX_CHECK(DirectoryExists(nested));
    String file = JoinPath(nested, "data.bin");
    u8 data[300];
    for (int i = 0; i < 300; i++)
        data[i] = u8(i);
    HX_REQUIRE(WriteFile(file, Span<const u8>(data, 300)));
    HX_CHECK(FileExists(file));
    Array<u8> read;
    HX_REQUIRE(ReadFile(file, read));
    HX_CHECK_EQ(read.Size(), 300u);
    HX_CHECK_EQ(read[299], 43);
    {
        File f;
        HX_REQUIRE(f.Open(file, FileMode::Append));
        HX_CHECK_EQ(f.Write("xyz", 3), 3u);
    }
    {
        File f;
        HX_REQUIRE(f.Open(file, FileMode::Read));
        HX_CHECK_EQ(f.Size(), 303);
        HX_CHECK(f.Seek(300));
        char tail[3];
        HX_CHECK_EQ(f.Read(tail, 3), 3u);
        HX_CHECK(StringView(tail, 3) == "xyz");
    }
    int count = 0;
    HX_CHECK(ListDirectory(nested, [](const DirectoryEntry& e, void* user) {
        if (e.name == "data.bin" && !e.isDirectory && e.size == 303)
            ++*static_cast<int*>(user);
    }, &count));
    HX_CHECK_EQ(count, 1);
    String moved = JoinPath(nested, "moved.bin");
    HX_CHECK(RenameFile(file, moved));
    HX_CHECK(!FileExists(file) && FileExists(moved));
    HX_CHECK(RemoveFile(moved));
    HX_CHECK(RemoveEmptyDirectory(nested));
    HX_CHECK(RemoveEmptyDirectory(JoinPath(dir, "a")));
    HX_CHECK(RemoveEmptyDirectory(dir));
    HX_CHECK(!DirectoryExists(dir));
    HX_CHECK(!ExecutablePath().Empty());
}

HX_TEST(FileSystem, Paths)
{
    HX_CHECK(JoinPath("a/", "/b") == "a/b");
    HX_CHECK(JoinPath("a", "b") == "a/b");
    HX_CHECK(GetFileName("x/y/file.tar.gz") == "file.tar.gz");
    HX_CHECK(GetExtension("x/y/file.tar.gz") == ".gz");
    HX_CHECK(GetExtension("x/.hidden").Empty());
    HX_CHECK(GetDirectoryName("x/y/file") == "x/y");
    HX_CHECK(GetDirectoryName("file").Empty());
}

HX_TEST(DynamicLibrary, OpenSystemLibrary)
{
#if HX_PLATFORM_WINDOWS
    const char* name = "kernel32.dll";
    const char* symbol = "GetTickCount64";
#elif HX_PLATFORM_LINUX
    const char* name = "libc.so.6";
    const char* symbol = "malloc";
#else
    const char* name = nullptr;
    const char* symbol = nullptr;
#endif
    if (!name)
        return;
    DynamicLibrary lib;
    HX_REQUIRE(lib.Open(name));
    HX_CHECK(lib.Symbol(symbol) != nullptr);
    HX_CHECK(lib.Symbol("helix_symbol_that_does_not_exist") == nullptr);
    DynamicLibrary missing;
    HX_CHECK(!missing.Open("helix_missing_library_xyz"));
}

HX_TEST(Window, CreatePollDestroy)
{
#if HX_PLATFORM_ANDROID
    return;
#endif
    if (!Window::IsSupported())
    {
        std::printf("    window test skipped: no display\n");
        return;
    }
    Window* w = Window::Create({.title = "helix test", .width = 320, .height = 200, .visible = true});
    HX_REQUIRE(w != nullptr);
    Array<Event> events;
    for (int i = 0; i < 10; i++)
    {
        w->PollEvents(events);
        SleepSeconds(0.01);
    }
    i32 fw = 0, fh = 0;
    w->GetFramebufferSize(fw, fh);
    HX_CHECK(fw > 0 && fh > 0);
    HX_CHECK(w->GetContentScale() > 0.0f);
    HX_CHECK(w->GetNativeHandle().window != nullptr);
    w->SetTitle("renamed");
    HX_CHECK(!w->CloseRequested());
    Window::Destroy(w);
}
