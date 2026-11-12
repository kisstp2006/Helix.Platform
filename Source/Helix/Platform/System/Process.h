#pragma once

#include <Helix/Core/Strings/StringView.h>

namespace helix::platform
{
    // One running instance per name: held while it lives (the system lets it go when the process ends). Windows and
    // Linux; elsewhere every instance is the only one.
    class SingleInstance
    {
    public:
        SingleInstance() = default;
        ~SingleInstance() { Release(); }
        SingleInstance(const SingleInstance&) = delete;
        SingleInstance& operator=(const SingleInstance&) = delete;

        // False when another instance holds the name (on Windows its window comes to the front).
        bool Acquire(StringView name);
        void Release();
        bool IsHeld() const { return handle_ != 0; }

    private:
        uptr handle_ = 0;
    };

    bool IsDebuggerAttached();
    // Tells the process id (a message box on Windows, standard error elsewhere) and waits until a native debugger is
    // attached; returns at once where that cannot be known (Android, the Web).
    void WaitForNativeDebugger();
}
