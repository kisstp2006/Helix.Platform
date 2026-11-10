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
