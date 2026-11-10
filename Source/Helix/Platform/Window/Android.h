#pragma once

#include <Helix/Platform/Window/Input.h>

struct ANativeWindow;

namespace helix::platform::android
{
    // Called from the Java glue of the player (JNI) to feed the native window and input.
    void SetNativeWindow(ANativeWindow* window);
    ANativeWindow* GetNativeWindow();
    void PushEvent(const Event& event);
    void SetContentScale(f32 scale);
    void SetFocused(bool focused);
}
