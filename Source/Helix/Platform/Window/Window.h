#pragma once

#include <Helix/Core/Containers/Array.h>
#include <Helix/Core/Strings/String.h>
#include <Helix/Platform/Window/Input.h>

namespace helix::platform
{
    enum class WindowMode : u8
    {
        Windowed = 0,
        // The operating system's maximized window.
        Maximized = 1,
        // A window without a frame covering its display (borderless full screen).
        FullScreen = 2,
        // The display switched to the window's mode (Windows; elsewhere FullScreen).
        ExclusiveFullScreen = 3,
    };

    struct DisplayMode
    {
        i32 width = 0, height = 0;
        // Hertz as a ratio; 0 when the system does not tell.
        u32 refreshNumerator = 0, refreshDenominator = 1;
    };

    struct DisplayInfo
    {
        String name;
        // Desktop pixels: the whole display and the part not taken by task bars.
        i32 x = 0, y = 0, width = 0, height = 0;
        i32 workX = 0, workY = 0, workWidth = 0, workHeight = 0;
        u32 refreshNumerator = 0, refreshDenominator = 1;
        f32 dpi = 96.0f;
        bool primary = false;
    };

    // The displays, the primary one first.
    void GetDisplays(Array<DisplayInfo>& out);
    // The modes of a display (largest first, no duplicates); its current mode when the system lists none.
    void GetDisplayModes(u32 display, Array<DisplayMode>& out);

    // The orientation an application asks of a mobile display (the script API's ScreenOrientation values).
    enum class DisplayOrientation : u8
    {
        Portrait = 1,
        PortraitUpsideDown = 2,
        LandscapeLeft = 3,
        LandscapeRight = 4,
        Auto = 5,
    };

    // Keeps the display from dimming and sleeping (Windows, Android, the Web's wake lock; nothing on X11).
    void SetKeepScreenOn(bool keepOn);
    // Mobile displays (Android, the Web's orientation lock); nothing on the desktop.
    void SetDisplayOrientation(DisplayOrientation orientation);

    inline constexpr i32 kDefaultWindowPosition = 0x7fffffff;

    struct WindowDesc
    {
        StringView title = "Helix";
        // The client area in pixels.
        i32 width = 1280;
        i32 height = 720;
        bool resizable = true;
        bool visible = true;
        // False: no frame and no title bar.
        bool decorated = true;
        WindowMode mode = WindowMode::Windowed;
        // The display the window opens on (centered when no position is given) and goes full screen on.
        u32 display = 0;
        // The client area's top left in desktop pixels; the system's choice by default.
        i32 x = kDefaultWindowPosition;
        i32 y = kDefaultWindowPosition;
        // For ExclusiveFullScreen: the display mode (the window's size and the current refresh rate when zero).
        DisplayMode exclusiveMode;
        // A native window to embed into (Windows), filling its client area.
        void* parent = nullptr;
        StringView canvasSelector = "#canvas";
    };

    struct NativeWindowHandle
    {
        void* window = nullptr;
        void* display = nullptr;
    };

    class Window
    {
    public:
        static Window* Create(const WindowDesc& desc);
        static void Destroy(Window* window);
        static bool IsSupported();

        virtual ~Window() = default;
        virtual void PollEvents(Array<Event>& out) = 0;
        virtual void SetTitle(StringView title) = 0;
        virtual void GetSize(i32& width, i32& height) const = 0;
        virtual void GetFramebufferSize(i32& width, i32& height) const = 0;
        virtual f32 GetContentScale() const = 0;
        virtual NativeWindowHandle GetNativeHandle() const = 0;
        virtual bool HasFocus() const = 0;
        // The client area in pixels (windowed and maximized windows).
        virtual void SetSize(i32 width, i32 height) = 0;
        // The client area's top left in desktop pixels.
        virtual void GetPosition(i32& x, i32& y) const = 0;
        virtual void SetPosition(i32 x, i32 y) = 0;
        // False when the mode is not available here (the window then takes the closest one, see GetMode).
        virtual bool SetMode(WindowMode mode, u32 display = 0, const DisplayMode& exclusive = {}) = 0;
        virtual WindowMode GetMode() const = 0;
        // The display that holds most of the window.
        virtual u32 GetDisplay() const = 0;
        bool CloseRequested() const { return closeRequested_; }

    protected:
        bool closeRequested_ = false;
    };

    using FrameFunction = bool (*)(void* user);

    // Native platforms loop until the frame function returns false, the web yields to the browser each frame. On the
    // Web the call does not return (main ends there) unless returnAtOnce: then it returns and the caller returns from
    // main (a program whose main was called by managed code, which must not be unwound).
    void RunMainLoop(FrameFunction frame, void* user, bool returnAtOnce = false);
}
