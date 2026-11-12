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

HX_TEST(Window, DisplaysAreListedWithThePrimaryFirst)
{
#if HX_PLATFORM_ANDROID
    return;
#endif
    if (!Window::IsSupported())
        return;
    Array<DisplayInfo> displays;
    GetDisplays(displays);
    HX_REQUIRE(!displays.Empty());
    HX_CHECK(displays[0].primary);
    for (const DisplayInfo& d : displays)
    {
        HX_CHECK(d.width > 0 && d.height > 0);
        HX_CHECK(d.workWidth > 0 && d.workWidth <= d.width);
        HX_CHECK(d.dpi > 0.0f);
        HX_CHECK(!d.name.Empty());
    }
    Array<DisplayMode> modes;
    GetDisplayModes(0, modes);
    HX_REQUIRE(!modes.Empty());
    for (usize i = 1; i < modes.Size(); i++)
        HX_CHECK(i64(modes[i - 1].width) * modes[i - 1].height >= i64(modes[i].width) * modes[i].height);
}

HX_TEST(Window, SizePositionAndModes)
{
#if HX_PLATFORM_ANDROID || HX_PLATFORM_WEB
    return;
#endif
    if (!Window::IsSupported())
        return;
    Window* w = Window::Create({.title = "helix test", .width = 320, .height = 200, .visible = false, .x = 40, .y = 60});
    HX_REQUIRE(w != nullptr);
    Array<Event> events;
    w->PollEvents(events);
    i32 fw = 0, fh = 0;
    w->GetFramebufferSize(fw, fh);
    HX_CHECK_EQ(fw, 320);
    HX_CHECK_EQ(fh, 200);
    w->SetSize(400, 300);
    w->PollEvents(events);
    w->GetFramebufferSize(fw, fh);
    HX_CHECK_EQ(fw, 400);
    HX_CHECK_EQ(fh, 300);
    HX_CHECK(w->GetMode() == WindowMode::Windowed);
#if HX_PLATFORM_WINDOWS
    i32 x = 0, y = 0;
    w->GetPosition(x, y);
    HX_CHECK_EQ(x, 40);
    HX_CHECK_EQ(y, 60);
    w->SetPosition(100, 120);
    w->GetPosition(x, y);
    HX_CHECK_EQ(x, 100);
    HX_CHECK_EQ(y, 120);
    // Full screen covers the display, windowed again comes back to the old place and size.
    Array<DisplayInfo> displays;
    GetDisplays(displays);
    HX_CHECK(w->SetMode(WindowMode::FullScreen, 0));
    w->PollEvents(events);
    w->GetFramebufferSize(fw, fh);
    HX_CHECK_EQ(fw, displays[0].width);
    HX_CHECK_EQ(fh, displays[0].height);
    HX_CHECK(w->GetMode() == WindowMode::FullScreen);
    HX_CHECK_EQ(w->GetDisplay(), 0u);
    HX_CHECK(w->SetMode(WindowMode::Windowed));
    w->PollEvents(events);
    w->GetFramebufferSize(fw, fh);
    HX_CHECK_EQ(fw, 400);
    HX_CHECK_EQ(fh, 300);
    w->GetPosition(x, y);
    HX_CHECK_EQ(x, 100);
    HX_CHECK_EQ(y, 120);
    HX_CHECK(w->SetMode(WindowMode::Maximized));
    w->PollEvents(events);
    w->GetFramebufferSize(fw, fh);
    HX_CHECK(fw > 400);
    HX_CHECK(w->SetMode(WindowMode::Windowed));
#endif
    Window::Destroy(w);
}
