#include <helix/platform/window.h>
#include <helix/core/base/assert.h>

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

        class Win32Window final : public Window
        {
        public:
            HWND hwnd = nullptr;
            Array<Event> pending;
            WCHAR highSurrogate = 0;
            bool focused = false;

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

    Window* Window::Create(const WindowDesc& desc)
    {
        EnableDpiAwareness();
        if (!RegisterWindowClass())
            return nullptr;
        auto* window = New<Win32Window>(HeapAllocator());
        DWORD style = WS_OVERLAPPEDWINDOW;
        if (!desc.resizable)
            style &= ~DWORD(WS_THICKFRAME | WS_MAXIMIZEBOX);
        UINT dpi = GetDpiForSystem();
        RECT r{0, 0, LONG(MulDiv(desc.width, int(dpi), 96)), LONG(MulDiv(desc.height, int(dpi), 96))};
        AdjustWindowRectExForDpi(&r, style, FALSE, 0, dpi);
        wchar_t title[256];
        int n = MultiByteToWideChar(CP_UTF8, 0, desc.title.Data(), int(desc.title.Size()), title, 255);
        title[n > 0 ? n : 0] = 0;
        HWND hwnd = CreateWindowExW(0, kClassName, title, style, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, nullptr,
                                    nullptr, GetModuleHandleW(nullptr), window);
        if (!hwnd)
        {
            Delete(HeapAllocator(), window);
            return nullptr;
        }
        window->hwnd = hwnd;
        if (desc.visible)
        {
            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);
        }
        return window;
    }

    void Window::Destroy(Window* window)
    {
        auto* w = static_cast<Win32Window*>(window);
        if (!w)
            return;
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
