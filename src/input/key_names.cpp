#include <helix/platform/input.h>

namespace helix::platform
{
    const char* KeyCodeName(KeyCode key)
    {
        static const char* letters[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
                                        "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
        static const char* digits[] = {"Alpha0", "Alpha1", "Alpha2", "Alpha3", "Alpha4", "Alpha5", "Alpha6", "Alpha7", "Alpha8", "Alpha9"};
        static const char* keypad[] = {"Keypad0", "Keypad1", "Keypad2", "Keypad3", "Keypad4", "Keypad5", "Keypad6", "Keypad7", "Keypad8", "Keypad9"};
        static const char* functions[] = {"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15"};
        u16 v = u16(key);
        if (key >= KeyCode::A && key <= KeyCode::Z)
            return letters[v - u16(KeyCode::A)];
        if (key >= KeyCode::Alpha0 && key <= KeyCode::Alpha9)
            return digits[v - u16(KeyCode::Alpha0)];
        if (key >= KeyCode::Keypad0 && key <= KeyCode::Keypad9)
            return keypad[v - u16(KeyCode::Keypad0)];
        if (key >= KeyCode::F1 && key <= KeyCode::F15)
            return functions[v - u16(KeyCode::F1)];
        switch (key)
        {
        case KeyCode::Backspace: return "Backspace";
        case KeyCode::Tab: return "Tab";
        case KeyCode::Return: return "Return";
        case KeyCode::Escape: return "Escape";
        case KeyCode::Space: return "Space";
        case KeyCode::Delete: return "Delete";
        case KeyCode::UpArrow: return "UpArrow";
        case KeyCode::DownArrow: return "DownArrow";
        case KeyCode::LeftArrow: return "LeftArrow";
        case KeyCode::RightArrow: return "RightArrow";
        case KeyCode::LeftShift: return "LeftShift";
        case KeyCode::RightShift: return "RightShift";
        case KeyCode::LeftControl: return "LeftControl";
        case KeyCode::RightControl: return "RightControl";
        case KeyCode::LeftAlt: return "LeftAlt";
        case KeyCode::RightAlt: return "RightAlt";
        case KeyCode::KeypadEnter: return "KeypadEnter";
        case KeyCode::Home: return "Home";
        case KeyCode::End: return "End";
        case KeyCode::PageUp: return "PageUp";
        case KeyCode::PageDown: return "PageDown";
        case KeyCode::Insert: return "Insert";
        case KeyCode::None: return "None";
        default: return "Other";
        }
    }
}
