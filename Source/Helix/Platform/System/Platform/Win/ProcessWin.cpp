#include <Helix/Platform/System/Process.h>

#include <Helix/Core/Strings/Format.h>

#include <windows.h>

namespace helix::platform
{
    bool SingleInstance::Acquire(StringView name)
    {
        Release();
        // Object names take no backslash but the one of the namespace.
        String object("Local\\Helix.");
        for (usize i = 0; i < name.Size(); i++)
            object.Append(name[i] == '\\' ? '_' : name[i]);
        wchar_t wide[300];
        int n = MultiByteToWideChar(CP_UTF8, 0, object.Data(), int(object.Size()), wide, 299);
        wide[n > 0 ? n : 0] = 0;
        HANDLE mutex = CreateMutexW(nullptr, TRUE, wide);
        if (!mutex)
            return true;
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            CloseHandle(mutex);
            // The window of the other instance comes to the front.
            if (HWND other = FindWindowW(L"HelixWindow", nullptr))
            {
                ShowWindow(other, SW_RESTORE);
                SetForegroundWindow(other);
            }
            return false;
        }
        handle_ = reinterpret_cast<uptr>(mutex);
        return true;
    }

    void SingleInstance::Release()
    {
        if (!handle_)
            return;
        HANDLE mutex = reinterpret_cast<HANDLE>(handle_);
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        handle_ = 0;
    }

    bool IsDebuggerAttached() { return IsDebuggerPresent() != FALSE; }

    void WaitForNativeDebugger()
    {
        if (IsDebuggerPresent())
            return;
        wchar_t text[128];
        wsprintfW(text, L"Attach a native debugger to process %lu, then press OK.", GetCurrentProcessId());
        MessageBoxW(nullptr, text, L"Helix", MB_OK | MB_ICONINFORMATION);
        while (!IsDebuggerPresent())
            Sleep(100);
    }
}
