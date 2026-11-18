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

HX_TEST(Window, TextInputTurnsOnAndOffWithNothingComposed)
{
#if HX_PLATFORM_ANDROID
    return;
#endif
    if (!Window::IsSupported())
        return;
    Window* w = Window::Create({.title = "helix text input", .width = 320, .height = 200, .visible = true});
    HX_REQUIRE(w != nullptr);
    Array<Event> events;
    w->SetTextInput(true, 20, 40, 16);
    // A second call moves the caret only.
    w->SetTextInput(true, 60, 40, 16);
    w->PollEvents(events);
    HX_CHECK(w->Composition().Empty());
    w->SetTextInput(false, 0, 0, 0);
    w->PollEvents(events);
    HX_CHECK(w->Composition().Empty());
    for (const Event& e : events)
        HX_CHECK(e.type != EventType::Composition);
    Window::Destroy(w);
}

HX_TEST(Window, CursorsAreSystemShapesOrImages)
{
#if HX_PLATFORM_ANDROID
    return;
#endif
    if (!Window::IsSupported())
        return;
    Window* w = Window::Create({.title = "helix cursors", .width = 320, .height = 200, .visible = true});
    HX_REQUIRE(w != nullptr);
    for (u8 shape = 0; shape <= u8(SystemCursor::Crosshair); shape++)
        HX_CHECK(w->SetCursor(SystemCursor(shape)));
    // A 16 pixel arrow of an image with its hotspot at the tip (X11 makes one only with libXcursor).
    Array<u8> pixels;
    for (u32 y = 0; y < 16; y++)
        for (u32 x = 0; x < 16; x++)
        {
            bool inside = x <= y;
            pixels.Push(255), pixels.Push(inside ? 255 : 0), pixels.Push(0), pixels.Push(inside ? 255 : 0);
        }
    bool image = w->SetCursorImage(Span<const u8>(pixels.Data(), pixels.Size()), 16, 16, 0, 0);
#if HX_PLATFORM_WINDOWS || HX_PLATFORM_WEB
    HX_CHECK(image);
#else
    (void)image;
#endif
    // Too few pixels: refused, nothing changes.
    HX_CHECK(!w->SetCursorImage(Span<const u8>(pixels.Data(), 8), 16, 16, 0, 0));
    HX_CHECK(w->SetCursor(SystemCursor::Arrow));
    Window::Destroy(w);
}

HX_TEST(Window, TheClipboardTakesTextAndGivesItBack)
{
    if (!Window::IsSupported())
        return;
    Window* w = Window::Create({.title = "helix clipboard", .width = 160, .height = 100, .visible = false});
    HX_REQUIRE(w != nullptr);
    // The text that was there goes back afterwards (the system's clipboard is the user's).
    String before = w->GetClipboardText();
    HX_CHECK(w->SetClipboardText("helix Ã¡rvÃ­ztÅ±rÅ clipboard"));
    HX_CHECK(w->GetClipboardText() == StringView("helix Ã¡rvÃ­ztÅ±rÅ clipboard"));
    HX_CHECK(w->SetClipboardText(""));
    HX_CHECK(w->GetClipboardText().Empty());
    w->SetClipboardText(before.View());
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

// A frame of the application's own: the whole window is the client area of the asked size; the hit test names the
// caption; minimize and back.
HX_TEST(Window, ACustomFrameIsAllClientWithItsHitTest)
{
#if HX_PLATFORM_ANDROID || HX_PLATFORM_WEB
    return;
#endif
    if (!Window::IsSupported())
        return;
    Window* w = Window::Create({.title = "helix custom frame", .width = 400, .height = 240, .visible = true, .customFrame = true});
    HX_REQUIRE(w != nullptr);
    Array<Event> events;
    w->PollEvents(events);
    HX_CHECK(w->HasCustomFrame());
    i32 width = 0, height = 0;
    w->GetSize(width, height);
    HX_CHECK(width == 400 && height == 240);
    HX_CHECK(w->HitTest(10, 10) == WindowHitTest::Client);
    w->SetHitTest([](void*, i32, i32 y) { return y < 30 ? WindowHitTest::Caption : WindowHitTest::Client; }, nullptr);
    HX_CHECK(w->HitTest(100, 10) == WindowHitTest::Caption && w->HitTest(100, 100) == WindowHitTest::Client);
    w->SetSize(420, 260);
    w->PollEvents(events);
    w->GetSize(width, height);
    HX_CHECK(width == 420 && height == 260);
    w->Minimize();
    w->PollEvents(events);
#if HX_PLATFORM_WINDOWS
    HX_CHECK(w->Minimized());
#endif
    w->SetMode(WindowMode::Windowed);
    w->PollEvents(events);
    Window::Destroy(w);
}
