#include <Helix/Platform/Window/Window.h>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <cstring>

namespace helix::platform
{
    namespace
    {
        struct CodeMapping
        {
            const char* code;
            KeyCode key;
        };

        constexpr CodeMapping kCodes[] = {
            {"Backspace", KeyCode::Backspace}, {"Tab", KeyCode::Tab}, {"Enter", KeyCode::Return}, {"Pause", KeyCode::Pause},
            {"Escape", KeyCode::Escape}, {"Space", KeyCode::Space}, {"Quote", KeyCode::Quote}, {"Comma", KeyCode::Comma},
            {"Minus", KeyCode::Minus}, {"Period", KeyCode::Period}, {"Slash", KeyCode::Slash}, {"Semicolon", KeyCode::Semicolon},
            {"Equal", KeyCode::Equals}, {"BracketLeft", KeyCode::LeftBracket}, {"Backslash", KeyCode::Backslash},
            {"BracketRight", KeyCode::RightBracket}, {"Backquote", KeyCode::BackQuote}, {"Delete", KeyCode::Delete},
            {"NumpadDecimal", KeyCode::KeypadPeriod}, {"NumpadDivide", KeyCode::KeypadDivide}, {"NumpadMultiply", KeyCode::KeypadMultiply},
            {"NumpadSubtract", KeyCode::KeypadMinus}, {"NumpadAdd", KeyCode::KeypadPlus}, {"NumpadEnter", KeyCode::KeypadEnter},
            {"NumpadEqual", KeyCode::KeypadEquals}, {"ArrowUp", KeyCode::UpArrow}, {"ArrowDown", KeyCode::DownArrow},
            {"ArrowRight", KeyCode::RightArrow}, {"ArrowLeft", KeyCode::LeftArrow}, {"Insert", KeyCode::Insert}, {"Home", KeyCode::Home},
            {"End", KeyCode::End}, {"PageUp", KeyCode::PageUp}, {"PageDown", KeyCode::PageDown}, {"NumLock", KeyCode::Numlock},
            {"CapsLock", KeyCode::CapsLock}, {"ScrollLock", KeyCode::ScrollLock}, {"ShiftRight", KeyCode::RightShift},
            {"ShiftLeft", KeyCode::LeftShift}, {"ControlRight", KeyCode::RightControl}, {"ControlLeft", KeyCode::LeftControl},
            {"AltRight", KeyCode::RightAlt}, {"AltLeft", KeyCode::LeftAlt}, {"MetaLeft", KeyCode::LeftWindows},
            {"MetaRight", KeyCode::RightWindows}, {"ContextMenu", KeyCode::Menu}, {"PrintScreen", KeyCode::Print},
        };

        KeyCode TranslateCode(const char* code)
        {
            if (std::strncmp(code, "Key", 3) == 0 && code[3] >= 'A' && code[3] <= 'Z' && !code[4])
                return KeyCode(u16(KeyCode::A) + u16(code[3] - 'A'));
            if (std::strncmp(code, "Digit", 5) == 0 && code[5] >= '0' && code[5] <= '9' && !code[6])
                return KeyCode(u16(KeyCode::Alpha0) + u16(code[5] - '0'));
            if (std::strncmp(code, "Numpad", 6) == 0 && code[6] >= '0' && code[6] <= '9' && !code[7])
                return KeyCode(u16(KeyCode::Keypad0) + u16(code[6] - '0'));
            if (code[0] == 'F' && code[1] >= '1' && code[1] <= '9')
            {
                int n = code[1] - '0';
                if (code[2] >= '0' && code[2] <= '9')
                    n = n * 10 + (code[2] - '0');
                if (n >= 1 && n <= 15)
                    return KeyCode(u16(KeyCode::F1) + u16(n - 1));
            }
            for (const CodeMapping& m : kCodes)
                if (std::strcmp(m.code, code) == 0)
                    return m.key;
            return KeyCode(0);
        }

        u32 DecodeUtf8(const char* s)
        {
            const u8* p = reinterpret_cast<const u8*>(s);
            if (!p[0])
                return 0;
            if (p[0] < 0x80)
                return p[1] ? 0 : p[0];
            if ((p[0] & 0xE0) == 0xC0)
                return u32(p[0] & 0x1F) << 6 | u32(p[1] & 0x3F);
            if ((p[0] & 0xF0) == 0xE0)
                return u32(p[0] & 0x0F) << 12 | u32(p[1] & 0x3F) << 6 | u32(p[2] & 0x3F);
            if ((p[0] & 0xF8) == 0xF0)
                return u32(p[0] & 0x07) << 18 | u32(p[1] & 0x3F) << 12 | u32(p[2] & 0x3F) << 6 | u32(p[3] & 0x3F);
            return 0;
        }

        u8 Modifiers(bool shift, bool ctrl, bool alt, bool meta)
        {
            return u8((shift ? ModShift : 0) | (ctrl ? ModControl : 0) | (alt ? ModAlt : 0) | (meta ? ModCommand : 0));
        }

        class WebWindow final : public Window
        {
        public:
            String selector;
            Array<Event> pending;
            bool focused = true;

            void PollEvents(Array<Event>& out) override
            {
                for (const Event& e : pending)
                    out.Push(e);
                pending.Clear();
            }

            void SetTitle(StringView title) override { emscripten_set_window_title(String(title).CStr()); }

            void GetSize(i32& width, i32& height) const override
            {
                double w = 0, h = 0;
                emscripten_get_element_css_size(selector.CStr(), &w, &h);
                width = i32(w);
                height = i32(h);
            }

            void GetFramebufferSize(i32& width, i32& height) const override
            {
                emscripten_get_canvas_element_size(selector.CStr(), &width, &height);
            }

            f32 GetContentScale() const override { return f32(emscripten_get_device_pixel_ratio()); }
            NativeWindowHandle GetNativeHandle() const override { return {const_cast<char*>(selector.CStr()), nullptr}; }
            bool HasFocus() const override { return focused; }

            void SyncCanvasSize()
            {
                double w = 0, h = 0;
                emscripten_get_element_css_size(selector.CStr(), &w, &h);
                f32 scale = GetContentScale();
                emscripten_set_canvas_element_size(selector.CStr(), int(w * scale), int(h * scale));
                Event e;
                e.type = EventType::WindowResize;
                e.resize = {i32(w), i32(h)};
                pending.Push(e);
            }
        };

        EM_BOOL OnKey(int type, const EmscriptenKeyboardEvent* k, void* user)
        {
            auto* w = static_cast<WebWindow*>(user);
            Event e;
            e.type = type == EMSCRIPTEN_EVENT_KEYDOWN ? EventType::KeyDown : EventType::KeyUp;
            e.modifiers = Modifiers(k->shiftKey, k->ctrlKey, k->altKey, k->metaKey);
            e.repeat = k->repeat;
            e.key = {TranslateCode(k->code), u32(k->keyCode)};
            w->pending.Push(e);
            if (type == EMSCRIPTEN_EVENT_KEYDOWN && !k->ctrlKey && !k->metaKey)
            {
                u32 cp = DecodeUtf8(k->key);
                if (cp >= 32 && cp != 127)
                {
                    Event t;
                    t.type = EventType::Text;
                    t.text = {cp};
                    w->pending.Push(t);
                }
            }
            return e.key.key != KeyCode(0);
        }

        EM_BOOL OnMouse(int type, const EmscriptenMouseEvent* m, void* user)
        {
            auto* w = static_cast<WebWindow*>(user);
            Event e;
            e.modifiers = Modifiers(m->shiftKey, m->ctrlKey, m->altKey, m->metaKey);
            MouseButton b = m->button == 0 ? MouseButton::Left : m->button == 1 ? MouseButton::Middle : m->button == 2 ? MouseButton::Right
                          : m->button == 3 ? MouseButton::X1 : MouseButton::X2;
            e.mouse = {f32(m->targetX), f32(m->targetY), b};
            e.type = type == EMSCRIPTEN_EVENT_MOUSEDOWN ? EventType::MouseDown : type == EMSCRIPTEN_EVENT_MOUSEUP ? EventType::MouseUp : EventType::MouseMove;
            w->pending.Push(e);
            return EM_TRUE;
        }

        EM_BOOL OnWheel(int, const EmscriptenWheelEvent* we, void* user)
        {
            auto* w = static_cast<WebWindow*>(user);
            Event e;
            e.type = EventType::MouseWheel;
            f32 scale = we->deltaMode == DOM_DELTA_PIXEL ? 1.0f / 100.0f : (we->deltaMode == DOM_DELTA_LINE ? 1.0f / 3.0f : 1.0f);
            e.wheel = {f32(-we->deltaX) * scale, f32(-we->deltaY) * scale};
            w->pending.Push(e);
            return EM_TRUE;
        }

        EM_BOOL OnResize(int, const EmscriptenUiEvent*, void* user)
        {
            static_cast<WebWindow*>(user)->SyncCanvasSize();
            return EM_FALSE;
        }

        EM_BOOL OnFocus(int type, const EmscriptenFocusEvent*, void* user)
        {
            auto* w = static_cast<WebWindow*>(user);
            w->focused = type == EMSCRIPTEN_EVENT_FOCUS;
            Event e;
            e.type = w->focused ? EventType::WindowFocus : EventType::WindowBlur;
            w->pending.Push(e);
            return EM_FALSE;
        }

        EM_BOOL OnTouch(int type, const EmscriptenTouchEvent* t, void* user)
        {
            auto* w = static_cast<WebWindow*>(user);
            for (int i = 0; i < t->numTouches; i++)
            {
                const EmscriptenTouchPoint& p = t->touches[i];
                if (!p.isChanged)
                    continue;
                Event e;
                e.type = type == EMSCRIPTEN_EVENT_TOUCHSTART ? EventType::TouchBegin
                       : type == EMSCRIPTEN_EVENT_TOUCHMOVE  ? EventType::TouchMove
                       : type == EMSCRIPTEN_EVENT_TOUCHEND   ? EventType::TouchEnd
                                                             : EventType::TouchCancel;
                e.touch = {i32(p.identifier), f32(p.targetX), f32(p.targetY)};
                w->pending.Push(e);
            }
            return EM_TRUE;
        }

        struct LoopState
        {
            FrameFunction frame;
            void* user;
        };

        void LoopTick(void* arg)
        {
            auto* s = static_cast<LoopState*>(arg);
            if (!s->frame(s->user))
                emscripten_cancel_main_loop();
        }
    }

    bool Window::IsSupported() { return EM_ASM_INT({ return typeof document !== 'undefined' ? 1 : 0; }) != 0; }

    Window* Window::Create(const WindowDesc& desc)
    {
        if (!IsSupported())
            return nullptr;
        auto* w = New<WebWindow>(HeapAllocator());
        w->selector = desc.canvasSelector;
        const char* sel = w->selector.CStr();
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, w, EM_TRUE, &OnKey);
        emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, w, EM_TRUE, &OnKey);
        emscripten_set_mousedown_callback(sel, w, EM_TRUE, &OnMouse);
        emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, w, EM_TRUE, &OnMouse);
        emscripten_set_mousemove_callback(sel, w, EM_TRUE, &OnMouse);
        emscripten_set_wheel_callback(sel, w, EM_TRUE, &OnWheel);
        emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, w, EM_FALSE, &OnResize);
        emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, w, EM_FALSE, &OnFocus);
        emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, w, EM_FALSE, &OnFocus);
        emscripten_set_touchstart_callback(sel, w, EM_TRUE, &OnTouch);
        emscripten_set_touchmove_callback(sel, w, EM_TRUE, &OnTouch);
        emscripten_set_touchend_callback(sel, w, EM_TRUE, &OnTouch);
        emscripten_set_touchcancel_callback(sel, w, EM_TRUE, &OnTouch);
        w->SetTitle(desc.title);
        w->SyncCanvasSize();
        return w;
    }

    void Window::Destroy(Window* window)
    {
        if (!window)
            return;
        emscripten_html5_remove_all_event_listeners();
        Delete(HeapAllocator(), static_cast<WebWindow*>(window));
    }

    void RunMainLoop(FrameFunction frame, void* user)
    {
        static LoopState state;
        state = {frame, user};
        emscripten_set_main_loop_arg(&LoopTick, &state, 0, true);
    }
}
