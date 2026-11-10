#pragma once

#include <helix/core/base/types.h>

namespace helix::platform
{
    // Values follow the script API KeyCode enumeration.
    enum class KeyCode : u16
    {
        None = 0,
        Backspace = 8,
        Tab = 9,
        Clear = 12,
        Return = 13,
        Pause = 19,
        Escape = 27,
        Space = 32,
        Exclaim = 33,
        DoubleQuote = 34,
        Hash = 35,
        Dollar = 36,
        Percent = 37,
        Ampersand = 38,
        Quote = 39,
        LeftParen = 40,
        RightParen = 41,
        Asterisk = 42,
        Plus = 43,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,
        Alpha0 = 48,
        Alpha1,
        Alpha2,
        Alpha3,
        Alpha4,
        Alpha5,
        Alpha6,
        Alpha7,
        Alpha8,
        Alpha9,
        Colon = 58,
        Semicolon = 59,
        Less = 60,
        Equals = 61,
        Greater = 62,
        Question = 63,
        At = 64,
        LeftBracket = 91,
        Backslash = 92,
        RightBracket = 93,
        Caret = 94,
        Underscore = 95,
        BackQuote = 96,
        A = 97,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        LeftCurlyBracket = 123,
        Pipe = 124,
        RightCurlyBracket = 125,
        Tilde = 126,
        Delete = 127,
        Keypad0 = 256,
        Keypad1,
        Keypad2,
        Keypad3,
        Keypad4,
        Keypad5,
        Keypad6,
        Keypad7,
        Keypad8,
        Keypad9,
        KeypadPeriod = 266,
        KeypadDivide = 267,
        KeypadMultiply = 268,
        KeypadMinus = 269,
        KeypadPlus = 270,
        KeypadEnter = 271,
        KeypadEquals = 272,
        UpArrow = 273,
        DownArrow = 274,
        RightArrow = 275,
        LeftArrow = 276,
        Insert = 277,
        Home = 278,
        End = 279,
        PageUp = 280,
        PageDown = 281,
        F1 = 282,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,
        Numlock = 300,
        CapsLock = 301,
        ScrollLock = 302,
        RightShift = 303,
        LeftShift = 304,
        RightControl = 305,
        LeftControl = 306,
        RightAlt = 307,
        LeftAlt = 308,
        RightCommand = 309,
        LeftCommand = 310,
        LeftWindows = 311,
        RightWindows = 312,
        AltGr = 313,
        Help = 315,
        Print = 316,
        SysReq = 317,
        Break = 318,
        Menu = 319,
    };

    enum class MouseButton : u8
    {
        Left,
        Right,
        Middle,
        X1,
        X2,
    };

    enum KeyModifier : u8
    {
        ModShift = 1,
        ModControl = 2,
        ModAlt = 4,
        ModCommand = 8,
    };

    enum class EventType : u8
    {
        None,
        Quit,
        WindowClose,
        WindowResize,
        WindowFocus,
        WindowBlur,
        KeyDown,
        KeyUp,
        Text,
        MouseMove,
        MouseDown,
        MouseUp,
        MouseWheel,
        TouchBegin,
        TouchMove,
        TouchEnd,
        TouchCancel,
    };

    struct ResizeData
    {
        i32 width, height;
    };

    struct KeyData
    {
        KeyCode key;
        u32 scancode;
    };

    struct TextData
    {
        u32 codepoint;
    };

    struct MouseData
    {
        f32 x, y;
        MouseButton button;
    };

    struct WheelData
    {
        f32 dx, dy;
    };

    struct TouchData
    {
        i32 id;
        f32 x, y;
    };

    struct Event
    {
        EventType type = EventType::None;
        u8 modifiers = 0;
        bool repeat = false;
        union
        {
            ResizeData resize;
            KeyData key;
            TextData text;
            MouseData mouse;
            WheelData wheel;
            TouchData touch;
        };

        Event() : resize{0, 0} {}
    };

    const char* KeyCodeName(KeyCode key);
}
