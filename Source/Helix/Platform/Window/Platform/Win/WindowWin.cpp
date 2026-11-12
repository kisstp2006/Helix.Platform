#include <Helix/Platform/Window/Window.h>
#include <Helix/Core/Basics/Assert.h>

#include <windows.h>
#include <windowsx.h>

namespace helix::platform
{
    namespace
    {
        const wchar_t* kClassName = L"HelixWindow";

        KeyCode TranslateKey(WPARAM vk, LPARAM lp)
        {
            bool extended = (lp & (1 << 24)) != 0;
            UINT scancode = (UINT(lp) >> 16) & 0xFF;
            if (vk >= '0' && vk <= '9')
                return KeyCode(u16(KeyCode::Alpha0) + u16(vk - '0'));
            if (vk >= 'A' && vk <= 'Z')
                return KeyCode(u16(KeyCode::A) + u16(vk - 'A'));
            if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
                return KeyCode(u16(KeyCode::Keypad0) + u16(vk - VK_NUMPAD0));
            if (vk >= VK_F1 && vk <= VK_F15)
                return KeyCode(u16(KeyCode::F1) + u16(vk - VK_F1));
            switch (vk)
            {
            case VK_BACK: return KeyCode::Backspace;
            case VK_TAB: return KeyCode::Tab;
            case VK_CLEAR: return KeyCode::Clear;
            case VK_RETURN: return extended ? KeyCode::KeypadEnter : KeyCode::Return;
            case VK_PAUSE: return KeyCode::Pause;
            case VK_ESCAPE: return KeyCode::Escape;
            case VK_SPACE: return KeyCode::Space;
            case VK_PRIOR: return KeyCode::PageUp;
            case VK_NEXT: return KeyCode::PageDown;
            case VK_END: return KeyCode::End;
            case VK_HOME: return KeyCode::Home;
            case VK_LEFT: return KeyCode::LeftArrow;
            case VK_UP: return KeyCode::UpArrow;
            case VK_RIGHT: return KeyCode::RightArrow;
            case VK_DOWN: return KeyCode::DownArrow;
            case VK_SNAPSHOT: return KeyCode::Print;
            case VK_INSERT: return KeyCode::Insert;
            case VK_DELETE: return KeyCode::Delete;
            case VK_LWIN: return KeyCode::LeftWindows;
            case VK_RWIN: return KeyCode::RightWindows;
            case VK_APPS: return KeyCode::Menu;
            case VK_MULTIPLY: return KeyCode::KeypadMultiply;
            case VK_ADD: return KeyCode::KeypadPlus;
            case VK_SUBTRACT: return KeyCode::KeypadMinus;
            case VK_DECIMAL: return KeyCode::KeypadPeriod;
            case VK_DIVIDE: return KeyCode::KeypadDivide;
            case VK_NUMLOCK: return KeyCode::Numlock;
            case VK_SCROLL: return KeyCode::ScrollLock;
            case VK_CAPITAL: return KeyCode::CapsLock;
            case VK_SHIFT: return MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT ? KeyCode::RightShift : KeyCode::LeftShift;
            case VK_CONTROL: return extended ? KeyCode::RightControl : KeyCode::LeftControl;
            case VK_MENU: return extended ? KeyCode::RightAlt : KeyCode::LeftAlt;
            case VK_OEM_1: return KeyCode::Semicolon;
            case VK_OEM_PLUS: return KeyCode::Equals;
            case VK_OEM_COMMA: return KeyCode::Comma;
            case VK_OEM_MINUS: return KeyCode::Minus;
            case VK_OEM_PERIOD: return KeyCode::Period;
            case VK_OEM_2: return KeyCode::Slash;
            case VK_OEM_3: return KeyCode::BackQuote;
            case VK_OEM_4: return KeyCode::LeftBracket;
            case VK_OEM_5: return KeyCode::Backslash;
            case VK_OEM_6: return KeyCode::RightBracket;
            case VK_OEM_7: return KeyCode::Quote;
            default: return KeyCode::None;
            }
        }

        u8 Modifiers()
        {
            u8 m = 0;
            if (GetKeyState(VK_SHIFT) & 0x8000)
                m |= ModShift;
            if (GetKeyState(VK_CONTROL) & 0x8000)
                m |= ModControl;
            if (GetKeyState(VK_MENU) & 0x8000)
                m |= ModAlt;
            if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000)
                m |= ModCommand;
            return m;
        }

        void EnableDpiAwareness()
        {
            static bool done = false;
            if (done)
                return;
            done = true;
            using SetContextFn = BOOL(WINAPI*)(HANDLE);
            if (HMODULE user32 = GetModuleHandleW(L"user32.dll"))
                if (auto fn = reinterpret_cast<SetContextFn>(reinterpret_cast<void*>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"))))
                    fn(reinterpret_cast<HANDLE>(-4));
        }

        // The monitors with the primary one first (the order of GetDisplays).
        BOOL CALLBACK CollectMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM user)
        {
            auto& list = *reinterpret_cast<Array<HMONITOR>*>(user);
            MONITORINFO mi{};
            mi.cbSize = sizeof(mi);
            GetMonitorInfoW(monitor, &mi);
            list.Push(monitor);
            if ((mi.dwFlags & MONITORINFOF_PRIMARY) != 0)
                for (usize i = list.Size() - 1; i > 0; i--)
                {
                    HMONITOR m = list[i];
                    list[i] = list[i - 1];
                    list[i - 1] = m;
                }
            return TRUE;
        }

        void Monitors(Array<HMONITOR>& out)
        {
            out.Clear();
            EnumDisplayMonitors(nullptr, nullptr, &CollectMonitor, reinterpret_cast<LPARAM>(&out));
        }

        HMONITOR MonitorAt(u32 display)
        {
            Array<HMONITOR> list;
            Monitors(list);
            if (display < list.Size())
                return list[display];
            return MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
        }

        String Narrow(const wchar_t* wide)
        {
            char buffer[512];
            int n = WideCharToMultiByte(CP_UTF8, 0, wide, -1, buffer, sizeof(buffer), nullptr, nullptr);
            return String(StringView(buffer, n > 0 ? usize(n - 1) : 0));
        }

        f32 MonitorDpi(HMONITOR monitor)
        {
            using GetDpiFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
            static GetDpiFn fn = []() -> GetDpiFn {
                HMODULE shcore = LoadLibraryW(L"shcore.dll");
                return shcore ? reinterpret_cast<GetDpiFn>(reinterpret_cast<void*>(GetProcAddress(shcore, "GetDpiForMonitor"))) : nullptr;
            }();
            UINT x = 96, y = 96;
            if (fn && fn(monitor, 0, &x, &y) == S_OK)
                return f32(x);
            return f32(GetDpiForSystem());
        }

        bool MonitorDevice(HMONITOR monitor, MONITORINFOEXW& mi)
        {
            mi = {};
            mi.cbSize = sizeof(mi);
            return GetMonitorInfoW(monitor, &mi) != 0;
        }

        class Win32Window final : public Window
        {
        public:
            HWND hwnd = nullptr;
            Array<Event> pending;
            WCHAR highSurrogate = 0;
            bool focused = false;
            DWORD windowedStyle = WS_OVERLAPPEDWINDOW;
            WindowMode mode = WindowMode::Windowed;
            // The window's place while full screen, and the display whose mode was changed.
            RECT windowedRect{};
            bool haveWindowedRect = false;
            wchar_t exclusiveDevice[CCHDEVICENAME] = {};

            DWORD CurrentStyle() const { return DWORD(GetWindowLongPtrW(hwnd, GWL_STYLE)); }

            void SetSize(i32 width, i32 height) override
            {
                if (mode == WindowMode::FullScreen || mode == WindowMode::ExclusiveFullScreen)
                    return;
                if (mode == WindowMode::Maximized)
                    SetMode(WindowMode::Windowed, 0, {});
                RECT r{0, 0, width, height};
                AdjustWindowRectExForDpi(&r, CurrentStyle(), FALSE, 0, GetDpiForWindow(hwnd));
                SetWindowPos(hwnd, nullptr, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }

            void GetPosition(i32& x, i32& y) const override
            {
                POINT p{0, 0};
                ClientToScreen(hwnd, &p);
                x = p.x;
                y = p.y;
            }

            void SetPosition(i32 x, i32 y) override
            {
                if (mode != WindowMode::Windowed)
                    return;
                RECT window;
                GetWindowRect(hwnd, &window);
                POINT origin{0, 0};
                ClientToScreen(hwnd, &origin);
                SetWindowPos(hwnd, nullptr, x - (origin.x - window.left), y - (origin.y - window.top), 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }

            void RestoreDisplayMode()
            {
                if (exclusiveDevice[0])
                {
                    ChangeDisplaySettingsExW(exclusiveDevice, nullptr, nullptr, 0, nullptr);
                    exclusiveDevice[0] = 0;
                }
            }

            bool SetMode(WindowMode next, u32 display, const DisplayMode& exclusive) override
            {
                bool full = next == WindowMode::FullScreen || next == WindowMode::ExclusiveFullScreen;
                bool wasFull = mode == WindowMode::FullScreen || mode == WindowMode::ExclusiveFullScreen;
                if (!wasFull && full)
                {
                    // Screen coordinates of the window as it is (a maximized one comes back maximized from SW_RESTORE's
                    // point of view: its normal place is taken then).
                    if (mode == WindowMode::Maximized)
                    {
                        WINDOWPLACEMENT wp{};
                        wp.length = sizeof(wp);
                        GetWindowPlacement(hwnd, &wp);
                        windowedRect = wp.rcNormalPosition;
                    }
                    else
                        GetWindowRect(hwnd, &windowedRect);
                    haveWindowedRect = true;
                }
                RestoreDisplayMode();
                bool ok = true;
                if (full)
                {
                    HMONITOR monitor = MonitorAt(display);
                    MONITORINFOEXW mi;
                    MonitorDevice(monitor, mi);
                    if (next == WindowMode::ExclusiveFullScreen)
                    {
                        RECT client;
                        GetClientRect(hwnd, &client);
                        DEVMODEW dm{};
                        dm.dmSize = sizeof(dm);
                        EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm);
                        dm.dmPelsWidth = DWORD(exclusive.width > 0 ? exclusive.width : client.right - client.left);
                        dm.dmPelsHeight = DWORD(exclusive.height > 0 ? exclusive.height : client.bottom - client.top);
                        if (exclusive.refreshNumerator > 0)
                            dm.dmDisplayFrequency = DWORD((exclusive.refreshNumerator + exclusive.refreshDenominator / 2) / exclusive.refreshDenominator);
                        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
                        if (ChangeDisplaySettingsExW(mi.szDevice, &dm, nullptr, CDS_FULLSCREEN, nullptr) == DISP_CHANGE_SUCCESSFUL)
                        {
                            wcscpy_s(exclusiveDevice, mi.szDevice);
                            MonitorDevice(monitor, mi);
                        }
                        else
                        {
                            next = WindowMode::FullScreen;
                            ok = false;
                        }
                    }
                    SetWindowLongPtrW(hwnd, GWL_STYLE, LONG_PTR((windowedStyle & ~DWORD(WS_OVERLAPPEDWINDOW)) | WS_POPUP | (CurrentStyle() & WS_VISIBLE)));
                    const RECT& r = mi.rcMonitor;
                    SetWindowPos(hwnd, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);
                }
                else
                {
                    SetWindowLongPtrW(hwnd, GWL_STYLE, LONG_PTR(windowedStyle | (CurrentStyle() & WS_VISIBLE)));
                    if (wasFull && haveWindowedRect)
                    {
                        const RECT& r = windowedRect;
                        SetWindowPos(hwnd, nullptr, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
                    }
                    else
                        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                    if ((CurrentStyle() & WS_VISIBLE) != 0)
                        ShowWindow(hwnd, next == WindowMode::Maximized ? SW_MAXIMIZE : SW_RESTORE);
                    else if (next == WindowMode::Maximized)
                    {
                        // A hidden window takes the work area of its display.
                        MONITORINFOEXW mi;
                        MonitorDevice(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), mi);
                        const RECT& r = mi.rcWork;
                        SetWindowPos(hwnd, nullptr, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE);
                    }
                }
                mode = next;
                return ok;
            }

            WindowMode GetMode() const override { return mode; }

            u32 GetDisplay() const override
            {
                HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                Array<HMONITOR> list;
                Monitors(list);
                for (u32 i = 0; i < list.Size(); i++)
                    if (list[i] == monitor)
                        return i;
                return 0;
            }

            void Push(Event e) { pending.Push(e); }

            void PollEvents(Array<Event>& out) override
            {
                MSG msg;
                while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                    {
                        Event e;
                        e.type = EventType::Quit;
                        Push(e);
                        closeRequested_ = true;
                        continue;
                    }
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
                for (const Event& e : pending)
                    out.Push(e);
                pending.Clear();
            }

            void SetTitle(StringView title) override
            {
                wchar_t wide[256];
                int n = MultiByteToWideChar(CP_UTF8, 0, title.Data(), int(title.Size()), wide, 255);
                wide[n > 0 ? n : 0] = 0;
                SetWindowTextW(hwnd, wide);
            }

            void GetSize(i32& width, i32& height) const override
            {
                RECT r;
                GetClientRect(hwnd, &r);
                f32 scale = GetContentScale();
                width = i32(f32(r.right - r.left) / scale + 0.5f);
                height = i32(f32(r.bottom - r.top) / scale + 0.5f);
            }

            void GetFramebufferSize(i32& width, i32& height) const override
            {
                RECT r;
                GetClientRect(hwnd, &r);
                width = r.right - r.left;
                height = r.bottom - r.top;
            }

            f32 GetContentScale() const override { return f32(GetDpiForWindow(hwnd)) / 96.0f; }
            NativeWindowHandle GetNativeHandle() const override { return {hwnd, GetModuleHandleW(nullptr)}; }
            bool HasFocus() const override { return focused; }

            LRESULT Handle(UINT message, WPARAM wp, LPARAM lp)
            {
                Event e;
                e.modifiers = Modifiers();
                switch (message)
                {
                case WM_CLOSE:
                    e.type = EventType::WindowClose;
                    closeRequested_ = true;
                    Push(e);
                    return 0;
                case WM_SIZE:
                    e.type = EventType::WindowResize;
                    e.resize = {LOWORD(lp), HIWORD(lp)};
                    Push(e);
                    return 0;
                case WM_SETFOCUS:
                case WM_KILLFOCUS:
                    focused = message == WM_SETFOCUS;
                    e.type = focused ? EventType::WindowFocus : EventType::WindowBlur;
                    Push(e);
                    return 0;
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYUP:
                {
                    bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
                    e.type = down ? EventType::KeyDown : EventType::KeyUp;
                    e.repeat = down && (lp & (1 << 30)) != 0;
                    e.key = {TranslateKey(wp, lp), (u32(lp) >> 16) & 0x1FF};
                    Push(e);
                    if (message == WM_SYSKEYDOWN && wp == VK_F4)
                        break;
                    return 0;
                }
                case WM_CHAR:
                {
                    WCHAR c = WCHAR(wp);
                    if (c >= 0xD800 && c <= 0xDBFF)
                    {
                        highSurrogate = c;
                        return 0;
                    }
                    u32 cp = c;
                    if (c >= 0xDC00 && c <= 0xDFFF && highSurrogate)
                        cp = 0x10000 + ((u32(highSurrogate) - 0xD800) << 10) + (u32(c) - 0xDC00);
                    highSurrogate = 0;
                    if (cp >= 32 && cp != 127)
                    {
                        e.type = EventType::Text;
                        e.text = {cp};
                        Push(e);
                    }
                    return 0;
                }
                case WM_MOUSEMOVE:
                    e.type = EventType::MouseMove;
                    e.mouse = {f32(GET_X_LPARAM(lp)), f32(GET_Y_LPARAM(lp)), MouseButton::Left};
                    Push(e);
                    return 0;
                case WM_LBUTTONDOWN:
                case WM_RBUTTONDOWN:
                case WM_MBUTTONDOWN:
                case WM_XBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_RBUTTONUP:
                case WM_MBUTTONUP:
                case WM_XBUTTONUP:
                {
                    bool down = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN;
                    MouseButton b = MouseButton::Left;
                    if (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP)
                        b = MouseButton::Right;
                    else if (message == WM_MBUTTONDOWN || message == WM_MBUTTONUP)
                        b = MouseButton::Middle;
                    else if (message == WM_XBUTTONDOWN || message == WM_XBUTTONUP)
                        b = GET_XBUTTON_WPARAM(wp) == XBUTTON1 ? MouseButton::X1 : MouseButton::X2;
                    e.type = down ? EventType::MouseDown : EventType::MouseUp;
                    e.mouse = {f32(GET_X_LPARAM(lp)), f32(GET_Y_LPARAM(lp)), b};
                    Push(e);
                    if (down)
                        SetCapture(hwnd);
                    else
                        ReleaseCapture();
                    return message == WM_XBUTTONDOWN || message == WM_XBUTTONUP ? TRUE : 0;
                }
                case WM_MOUSEWHEEL:
                case WM_MOUSEHWHEEL:
                {
                    f32 delta = f32(GET_WHEEL_DELTA_WPARAM(wp)) / f32(WHEEL_DELTA);
                    e.type = EventType::MouseWheel;
                    e.wheel = message == WM_MOUSEWHEEL ? WheelData{0.0f, delta} : WheelData{delta, 0.0f};
                    Push(e);
                    return 0;
                }
                case WM_DPICHANGED:
                {
                    const RECT* r = reinterpret_cast<const RECT*>(lp);
                    SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
                    return 0;
                }
                default:
                    break;
                }
                return DefWindowProcW(hwnd, message, wp, lp);
            }
        };

        LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
        {
            if (message == WM_NCCREATE)
            {
                auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            }
            auto* window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (!window || !window->hwnd)
                return DefWindowProcW(hwnd, message, wp, lp);
            return window->Handle(message, wp, lp);
        }

        bool RegisterWindowClass()
        {
            static bool registered = false;
            if (registered)
                return true;
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            wc.lpfnWndProc = &WindowProc;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
            wc.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
            wc.lpszClassName = kClassName;
            registered = RegisterClassExW(&wc) != 0;
            return registered;
        }
    }

    bool Window::IsSupported() { return true; }

    void GetDisplays(Array<DisplayInfo>& out)
    {
        EnableDpiAwareness();
        out.Clear();
        Array<HMONITOR> list;
        Monitors(list);
        for (HMONITOR monitor : list)
        {
            MONITORINFOEXW mi;
            if (!MonitorDevice(monitor, mi))
                continue;
            DisplayInfo d;
            DISPLAY_DEVICEW dd{};
            dd.cb = sizeof(dd);
            d.name = EnumDisplayDevicesW(mi.szDevice, 0, &dd, 0) ? Narrow(dd.DeviceString) : Narrow(mi.szDevice);
            d.x = mi.rcMonitor.left;
            d.y = mi.rcMonitor.top;
            d.width = mi.rcMonitor.right - mi.rcMonitor.left;
            d.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
            d.workX = mi.rcWork.left;
            d.workY = mi.rcWork.top;
            d.workWidth = mi.rcWork.right - mi.rcWork.left;
            d.workHeight = mi.rcWork.bottom - mi.rcWork.top;
            DEVMODEW dm{};
            dm.dmSize = sizeof(dm);
            if (EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm) && dm.dmDisplayFrequency > 1)
                d.refreshNumerator = dm.dmDisplayFrequency;
            d.dpi = MonitorDpi(monitor);
            d.primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
            out.Push(Move(d));
        }
    }

    void GetDisplayModes(u32 display, Array<DisplayMode>& out)
    {
        out.Clear();
        MONITORINFOEXW mi;
        if (!MonitorDevice(MonitorAt(display), mi))
            return;
        DEVMODEW dm{};
        dm.dmSize = sizeof(dm);
        for (DWORD i = 0; EnumDisplaySettingsW(mi.szDevice, i, &dm); i++)
        {
            if (dm.dmBitsPerPel != 32)
                continue;
            DisplayMode m{i32(dm.dmPelsWidth), i32(dm.dmPelsHeight), dm.dmDisplayFrequency > 1 ? dm.dmDisplayFrequency : 0, 1};
            bool seen = false;
            for (const DisplayMode& o : out)
                seen = seen || (o.width == m.width && o.height == m.height && o.refreshNumerator == m.refreshNumerator);
            if (!seen)
                out.Push(m);
        }
        if (out.Empty() && EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm))
            out.Push({i32(dm.dmPelsWidth), i32(dm.dmPelsHeight), dm.dmDisplayFrequency > 1 ? dm.dmDisplayFrequency : 0, 1});
        // Largest first.
        for (usize i = 1; i < out.Size(); i++)
            for (usize j = i; j > 0; j--)
            {
                const DisplayMode& a = out[j - 1];
                const DisplayMode& b = out[j];
                bool after = i64(a.width) * a.height < i64(b.width) * b.height ||
                             (i64(a.width) * a.height == i64(b.width) * b.height && a.refreshNumerator < b.refreshNumerator);
                if (!after)
                    break;
                DisplayMode s = out[j - 1];
                out[j - 1] = out[j];
                out[j] = s;
            }
    }

    Window* Window::Create(const WindowDesc& desc)
    {
        EnableDpiAwareness();
        if (!RegisterWindowClass())
            return nullptr;
        auto* window = New<Win32Window>(HeapAllocator());
        HWND parent = static_cast<HWND>(desc.parent);
        DWORD style = WS_OVERLAPPEDWINDOW;
        if (!desc.resizable)
            style &= ~DWORD(WS_THICKFRAME | WS_MAXIMIZEBOX);
        if (!desc.decorated)
            style = WS_POPUP;
        if (parent)
            style = WS_CHILD | WS_CLIPSIBLINGS;
        window->windowedStyle = style;
        // The size is in pixels of the display the window opens on.
        HMONITOR monitor = MonitorAt(desc.display);
        UINT dpi = UINT(MonitorDpi(monitor));
        i32 width = desc.width, height = desc.height;
        if (parent)
        {
            RECT pr;
            GetClientRect(parent, &pr);
            width = pr.right - pr.left;
            height = pr.bottom - pr.top;
        }
        RECT r{0, 0, LONG(width), LONG(height)};
        AdjustWindowRectExForDpi(&r, style, FALSE, 0, dpi);
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
        if (parent)
            x = y = 0;
        else if (desc.x != kDefaultWindowPosition && desc.y != kDefaultWindowPosition)
        {
            x = desc.x + r.left;
            y = desc.y + r.top;
        }
        else if (desc.display != 0)
        {
            MONITORINFOEXW mi;
            MonitorDevice(monitor, mi);
            x = (mi.rcWork.left + mi.rcWork.right - (r.right - r.left)) / 2;
            y = (mi.rcWork.top + mi.rcWork.bottom - (r.bottom - r.top)) / 2;
        }
        wchar_t title[256];
        int n = MultiByteToWideChar(CP_UTF8, 0, desc.title.Data(), int(desc.title.Size()), title, 255);
        title[n > 0 ? n : 0] = 0;
        HWND hwnd = CreateWindowExW(0, kClassName, title, style, x, y, r.right - r.left, r.bottom - r.top, parent, nullptr,
                                    GetModuleHandleW(nullptr), window);
        if (!hwnd)
        {
            Delete(HeapAllocator(), window);
            return nullptr;
        }
        window->hwnd = hwnd;
        if (!parent && desc.mode != WindowMode::Windowed)
            window->SetMode(desc.mode, desc.display, desc.exclusiveMode);
        if (desc.visible)
        {
            ShowWindow(hwnd, window->mode == WindowMode::Maximized ? SW_MAXIMIZE : SW_SHOW);
            UpdateWindow(hwnd);
        }
        return window;
    }

    void Window::Destroy(Window* window)
    {
        auto* w = static_cast<Win32Window*>(window);
        if (!w)
            return;
        w->RestoreDisplayMode();
        HWND hwnd = w->hwnd;
        w->hwnd = nullptr;
        DestroyWindow(hwnd);
        Delete(HeapAllocator(), w);
    }

    void RunMainLoop(FrameFunction frame, void* user)
    {
        while (frame(user))
        {
        }
    }
}
