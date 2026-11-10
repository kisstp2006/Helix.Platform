#include <helix/platform/dynamic_library.h>
#include <helix/platform/window.h>

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

namespace helix::platform
{
    namespace
    {
        struct Xlib
        {
            DynamicLibrary library;
            decltype(&XOpenDisplay) OpenDisplay;
            decltype(&XCloseDisplay) CloseDisplay;
            decltype(&XCreateWindow) CreateWindow;
            decltype(&XDestroyWindow) DestroyWindow;
            decltype(&XMapWindow) MapWindow;
            decltype(&XStoreName) StoreName;
            decltype(&XInternAtom) InternAtom;
            decltype(&XSetWMProtocols) SetWMProtocols;
            decltype(&XPending) Pending;
            decltype(&XNextEvent) NextEvent;
            decltype(&XLookupString) LookupString;
            decltype(&XLookupKeysym) LookupKeysym;
            decltype(&XFlush) Flush;
            decltype(&XGetWindowAttributes) GetWindowAttributes;
            decltype(&XCreateColormap) CreateColormap;
            decltype(&XkbSetDetectableAutoRepeat) SetDetectableAutoRepeat;
            decltype(&XSetWMNormalHints) SetWMNormalHints;
            bool loaded = false;

            bool Load()
            {
                if (loaded)
                    return true;
                if (!library.Open("libX11.so.6") && !library.Open("libX11.so"))
                    return false;
#define HX_X(name) name = library.Function<decltype(name)>("X" #name)
                HX_X(OpenDisplay);
                HX_X(CloseDisplay);
                HX_X(CreateWindow);
                HX_X(DestroyWindow);
                HX_X(MapWindow);
                HX_X(StoreName);
                HX_X(InternAtom);
                HX_X(SetWMProtocols);
                HX_X(Pending);
                HX_X(NextEvent);
                HX_X(LookupString);
                HX_X(LookupKeysym);
                HX_X(Flush);
                HX_X(GetWindowAttributes);
                HX_X(CreateColormap);
                HX_X(SetWMNormalHints);
#undef HX_X
                SetDetectableAutoRepeat = library.Function<decltype(SetDetectableAutoRepeat)>("XkbSetDetectableAutoRepeat");
                loaded = OpenDisplay && NextEvent && CreateWindow;
                return loaded;
            }
        };

        Xlib x;

        KeyCode TranslateKeysym(KeySym s)
        {
            if (s >= XK_a && s <= XK_z)
                return KeyCode(u16(KeyCode::A) + u16(s - XK_a));
            if (s >= XK_A && s <= XK_Z)
                return KeyCode(u16(KeyCode::A) + u16(s - XK_A));
            if (s >= XK_0 && s <= XK_9)
                return KeyCode(u16(KeyCode::Alpha0) + u16(s - XK_0));
            if (s >= XK_KP_0 && s <= XK_KP_9)
                return KeyCode(u16(KeyCode::Keypad0) + u16(s - XK_KP_0));
            if (s >= XK_F1 && s <= XK_F15)
                return KeyCode(u16(KeyCode::F1) + u16(s - XK_F1));
            switch (s)
            {
            case XK_BackSpace: return KeyCode::Backspace;
            case XK_Tab: return KeyCode::Tab;
            case XK_Clear: return KeyCode::Clear;
            case XK_Return: return KeyCode::Return;
            case XK_Pause: return KeyCode::Pause;
            case XK_Escape: return KeyCode::Escape;
            case XK_space: return KeyCode::Space;
            case XK_apostrophe: return KeyCode::Quote;
            case XK_comma: return KeyCode::Comma;
            case XK_minus: return KeyCode::Minus;
            case XK_period: return KeyCode::Period;
            case XK_slash: return KeyCode::Slash;
            case XK_semicolon: return KeyCode::Semicolon;
            case XK_equal: return KeyCode::Equals;
            case XK_bracketleft: return KeyCode::LeftBracket;
            case XK_backslash: return KeyCode::Backslash;
            case XK_bracketright: return KeyCode::RightBracket;
            case XK_grave: return KeyCode::BackQuote;
            case XK_Delete: return KeyCode::Delete;
            case XK_KP_Decimal: return KeyCode::KeypadPeriod;
            case XK_KP_Divide: return KeyCode::KeypadDivide;
            case XK_KP_Multiply: return KeyCode::KeypadMultiply;
            case XK_KP_Subtract: return KeyCode::KeypadMinus;
            case XK_KP_Add: return KeyCode::KeypadPlus;
            case XK_KP_Enter: return KeyCode::KeypadEnter;
            case XK_KP_Equal: return KeyCode::KeypadEquals;
            case XK_Up: return KeyCode::UpArrow;
            case XK_Down: return KeyCode::DownArrow;
            case XK_Right: return KeyCode::RightArrow;
            case XK_Left: return KeyCode::LeftArrow;
            case XK_Insert: return KeyCode::Insert;
            case XK_Home: return KeyCode::Home;
            case XK_End: return KeyCode::End;
            case XK_Page_Up: return KeyCode::PageUp;
            case XK_Page_Down: return KeyCode::PageDown;
            case XK_Num_Lock: return KeyCode::Numlock;
            case XK_Caps_Lock: return KeyCode::CapsLock;
            case XK_Scroll_Lock: return KeyCode::ScrollLock;
            case XK_Shift_R: return KeyCode::RightShift;
            case XK_Shift_L: return KeyCode::LeftShift;
            case XK_Control_R: return KeyCode::RightControl;
            case XK_Control_L: return KeyCode::LeftControl;
            case XK_Alt_R: return KeyCode::RightAlt;
            case XK_Alt_L: return KeyCode::LeftAlt;
            case XK_Super_L: return KeyCode::LeftWindows;
            case XK_Super_R: return KeyCode::RightWindows;
            case XK_ISO_Level3_Shift: return KeyCode::AltGr;
            case XK_Menu: return KeyCode::Menu;
            case XK_Print: return KeyCode::Print;
            default: return KeyCode(0);
            }
        }

        u8 Modifiers(unsigned state)
        {
            u8 m = 0;
            if (state & ShiftMask)
                m |= ModShift;
            if (state & ControlMask)
                m |= ModControl;
            if (state & Mod1Mask)
                m |= ModAlt;
            if (state & Mod4Mask)
                m |= ModCommand;
            return m;
        }

        u32 DecodeUtf8(const char* s, int n)
        {
            const u8* p = reinterpret_cast<const u8*>(s);
            if (n <= 0)
                return 0;
            if (p[0] < 0x80)
                return p[0];
            if ((p[0] & 0xE0) == 0xC0 && n >= 2)
                return u32(p[0] & 0x1F) << 6 | u32(p[1] & 0x3F);
            if ((p[0] & 0xF0) == 0xE0 && n >= 3)
                return u32(p[0] & 0x0F) << 12 | u32(p[1] & 0x3F) << 6 | u32(p[2] & 0x3F);
            return 0;
        }

        class X11Window final : public Window
        {
        public:
            Display* display = nullptr;
            ::Window handle = 0;
            Atom deleteAtom = 0;
            i32 width = 0, height = 0;
            bool focused = false;

            void PollEvents(Array<Event>& out) override
            {
                while (x.Pending(display))
                {
                    XEvent xe;
                    x.NextEvent(display, &xe);
                    Event e;
                    switch (xe.type)
                    {
                    case ClientMessage:
                        if (Atom(xe.xclient.data.l[0]) == deleteAtom)
                        {
                            e.type = EventType::WindowClose;
                            closeRequested_ = true;
                            out.Push(e);
                        }
                        break;
                    case ConfigureNotify:
                        if (xe.xconfigure.width != width || xe.xconfigure.height != height)
                        {
                            width = xe.xconfigure.width;
                            height = xe.xconfigure.height;
                            e.type = EventType::WindowResize;
                            e.resize = {width, height};
                            out.Push(e);
                        }
                        break;
                    case FocusIn:
                    case FocusOut:
                        focused = xe.type == FocusIn;
                        e.type = focused ? EventType::WindowFocus : EventType::WindowBlur;
                        out.Push(e);
                        break;
                    case KeyPress:
                    case KeyRelease:
                    {
                        e.type = xe.type == KeyPress ? EventType::KeyDown : EventType::KeyUp;
                        e.modifiers = Modifiers(xe.xkey.state);
                        e.key = {TranslateKeysym(x.LookupKeysym(&xe.xkey, 0)), xe.xkey.keycode};
                        out.Push(e);
                        if (xe.type == KeyPress)
                        {
                            char text[8];
                            KeySym sym;
                            int n = x.LookupString(&xe.xkey, text, sizeof(text), &sym, nullptr);
                            u32 cp = DecodeUtf8(text, n);
                            if (cp >= 32 && cp != 127)
                            {
                                Event t;
                                t.type = EventType::Text;
                                t.text = {cp};
                                out.Push(t);
                            }
                        }
                        break;
                    }
                    case MotionNotify:
                        e.type = EventType::MouseMove;
                        e.modifiers = Modifiers(xe.xmotion.state);
                        e.mouse = {f32(xe.xmotion.x), f32(xe.xmotion.y), MouseButton::Left};
                        out.Push(e);
                        break;
                    case ButtonPress:
                    case ButtonRelease:
                    {
                        unsigned b = xe.xbutton.button;
                        e.modifiers = Modifiers(xe.xbutton.state);
                        if (b >= 4 && b <= 7)
                        {
                            if (xe.type == ButtonPress)
                            {
                                e.type = EventType::MouseWheel;
                                e.wheel = b == 4 ? WheelData{0, 1} : b == 5 ? WheelData{0, -1} : b == 6 ? WheelData{1, 0} : WheelData{-1, 0};
                                out.Push(e);
                            }
                            break;
                        }
                        MouseButton mb = b == 1 ? MouseButton::Left : b == 2 ? MouseButton::Middle : b == 3 ? MouseButton::Right : b == 8 ? MouseButton::X1 : MouseButton::X2;
                        e.type = xe.type == ButtonPress ? EventType::MouseDown : EventType::MouseUp;
                        e.mouse = {f32(xe.xbutton.x), f32(xe.xbutton.y), mb};
                        out.Push(e);
                        break;
                    }
                    default:
                        break;
                    }
                }
            }

            void SetTitle(StringView title) override
            {
                String t(title);
                x.StoreName(display, handle, t.CStr());
                x.Flush(display);
            }

            void GetSize(i32& w, i32& h) const override
            {
                w = width;
                h = height;
            }

            void GetFramebufferSize(i32& w, i32& h) const override
            {
                w = width;
                h = height;
            }

            f32 GetContentScale() const override { return 1.0f; }
            NativeWindowHandle GetNativeHandle() const override { return {reinterpret_cast<void*>(uptr(handle)), display}; }
            bool HasFocus() const override { return focused; }
        };
    }

    bool Window::IsSupported()
    {
        if (!x.Load())
            return false;
        Display* d = x.OpenDisplay(nullptr);
        if (!d)
            return false;
        x.CloseDisplay(d);
        return true;
    }

    Window* Window::Create(const WindowDesc& desc)
    {
        if (!x.Load())
            return nullptr;
        Display* display = x.OpenDisplay(nullptr);
        if (!display)
            return nullptr;
        int screen = DefaultScreen(display);
        ::Window root = RootWindow(display, screen);
        XSetWindowAttributes attrs{};
        attrs.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask |
                           FocusChangeMask | ExposureMask;
        attrs.background_pixel = BlackPixel(display, screen);
        ::Window handle = x.CreateWindow(display, root, 0, 0, unsigned(desc.width), unsigned(desc.height), 0, CopyFromParent, InputOutput,
                                         CopyFromParent, CWEventMask | CWBackPixel, &attrs);
        if (!handle)
        {
            x.CloseDisplay(display);
            return nullptr;
        }
        auto* window = New<X11Window>(HeapAllocator());
        window->display = display;
        window->handle = handle;
        window->width = desc.width;
        window->height = desc.height;
        window->deleteAtom = x.InternAtom(display, "WM_DELETE_WINDOW", False);
        x.SetWMProtocols(display, handle, &window->deleteAtom, 1);
        if (x.SetDetectableAutoRepeat)
            x.SetDetectableAutoRepeat(display, True, nullptr);
        if (!desc.resizable && x.SetWMNormalHints)
        {
            XSizeHints hints{};
            hints.flags = PMinSize | PMaxSize;
            hints.min_width = hints.max_width = desc.width;
            hints.min_height = hints.max_height = desc.height;
            x.SetWMNormalHints(display, handle, &hints);
        }
        window->SetTitle(desc.title);
        if (desc.visible)
            x.MapWindow(display, handle);
        x.Flush(display);
        return window;
    }

    void Window::Destroy(Window* window)
    {
        auto* w = static_cast<X11Window*>(window);
        if (!w)
            return;
        x.DestroyWindow(w->display, w->handle);
        x.CloseDisplay(w->display);
        Delete(HeapAllocator(), w);
    }

    void RunMainLoop(FrameFunction frame, void* user)
    {
        while (frame(user))
        {
        }
    }
}
