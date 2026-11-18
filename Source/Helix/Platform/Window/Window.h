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

    // What a point of a window with a frame of its own is (WindowDesc::customFrame): the application's client area, the
    // caption that moves the window (a double click maximizes it), or an edge that resizes it.
    enum class WindowHitTest : u8
    {
        Client,
        Caption,
        Left,
        Right,
        Top,
        Bottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
    };

    // The hit test of a window with a frame of its own: client pixels from the top left.
    using WindowHitTestFunction = WindowHitTest (*)(void* user, i32 x, i32 y);

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
        // The application draws the frame (an editor's title bar): no system title bar, while the system's resize
        // edges, snapping and shadow stay where the platform keeps them (Windows; X11 moves and resizes through the
        // window manager). The hit test (SetHitTest) names the caption; elsewhere the system frame stays.
        bool customFrame = false;
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
        // A native window that owns this one (its NativeWindowHandle::window): the window stays above its owner and
        // has no task bar button of its own (Windows: an owned window; X11: transient for the owner).
        void* owner = nullptr;
        StringView canvasSelector = "#canvas";
    };

    // The pointer shapes the system draws.
    enum class SystemCursor : u8
    {
        Arrow,
        Text,
        Hand,
        ResizeVertical,
        ResizeHorizontal,
        // Up to the right (and down to the left), up to the left (and down to the right).
        ResizeDiagonalUp,
        ResizeDiagonalDown,
        Move,
        NotAllowed,
        Wait,
        Crosshair,
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
        // The window to the task bar and back (SetMode(Windowed) or SetMode(Maximized) brings it back too).
        virtual void Minimize() {}
        virtual bool Minimized() const { return false; }
        // A window with a frame of its own: the function naming its caption and edges (null: all client).
        void SetHitTest(WindowHitTestFunction function, void* user)
        {
            hitTest_ = function;
            hitTestUser_ = user;
        }
        WindowHitTest HitTest(i32 x, i32 y) const { return hitTest_ ? hitTest_(hitTestUser_, x, y) : WindowHitTest::Client; }
        bool HasCustomFrame() const { return customFrame_; }
        // The display that holds most of the window.
        virtual u32 GetDisplay() const = 0;
        bool CloseRequested() const { return closeRequested_; }

        // Text input through the system's input method (IME): while on, keys may compose text, which the caller shows
        // at its caret (client pixels from the top left, the caret's top and height), where the candidate window
        // goes. Off when the window opens; platforms without an input method ignore it.
        virtual void SetTextInput(bool on, i32 caretX, i32 caretY, i32 caretHeight)
        {
            (void)on, (void)caretX, (void)caretY, (void)caretHeight;
        }
        // The pointer over the window's client area: a system shape, or an image (RGBA8, rows from the top, straight
        // alpha) with its hotspot (pixels from its top left). False where the platform cannot (no pointer on Android;
        // X11 images need libXcursor), the pointer staying as it was.
        virtual bool SetCursor(SystemCursor shape)
        {
            (void)shape;
            return false;
        }
        virtual bool SetCursorImage(Span<const u8> rgba, u32 width, u32 height, i32 hotspotX, i32 hotspotY)
        {
            (void)rgba, (void)width, (void)height, (void)hotspotX, (void)hotspotY;
            return false;
        }
        // The text being composed (codepoints), empty when nothing is; changes come as Composition events.
        Span<const u32> Composition() const { return Span<const u32>(composition_.Data(), composition_.Size()); }
        // The system clipboard's text (UTF-8). Where the window reaches no clipboard (Android) it is the window's own:
        // what was set last. On the Web the system's is written; reading it is the browser's to allow, so a read gives
        // what this window set or pasted last.
        virtual bool SetClipboardText(StringView text)
        {
            clipboard_ = String(text);
            return true;
        }
        virtual String GetClipboardText() { return clipboard_; }

    protected:
        bool closeRequested_ = false;
        bool customFrame_ = false;
        WindowHitTestFunction hitTest_ = nullptr;
        void* hitTestUser_ = nullptr;
        Array<u32> composition_;
        String clipboard_;
    };

    using FrameFunction = bool (*)(void* user);

    // Native platforms loop until the frame function returns false, the web yields to the browser each frame. On the
    // Web the call does not return (main ends there) unless returnAtOnce: then it returns and the caller returns from
    // main (a program whose main was called by managed code, which must not be unwound).
    void RunMainLoop(FrameFunction frame, void* user, bool returnAtOnce = false);
}
