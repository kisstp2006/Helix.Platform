#include <Helix/Platform/System/Process.h>

#include <Helix/Core/Containers/Array.h>
#include <Helix/Core/Strings/Format.h>

#include <windows.h>

namespace helix::platform
{
    bool ReadEnvironment(StringView name, String& value)
    {
        wchar_t key[512];
        int n = MultiByteToWideChar(CP_UTF8, 0, name.Data(), int(name.Size()), key, 511);
        key[n > 0 ? n : 0] = 0;
        DWORD size = GetEnvironmentVariableW(key, nullptr, 0);
        if (size == 0)
            return false;
        Array<wchar_t> wide;
        wide.Resize(size);
        DWORD got = GetEnvironmentVariableW(key, wide.Data(), size);
        if (got == 0 || got >= size)
            return false;
        int bytes = WideCharToMultiByte(CP_UTF8, 0, wide.Data(), int(got), nullptr, 0, nullptr, nullptr);
        Array<char> utf8;
        utf8.Resize(usize(bytes));
        WideCharToMultiByte(CP_UTF8, 0, wide.Data(), int(got), utf8.Data(), bytes, nullptr, nullptr);
        value = String(StringView(utf8.Data(), utf8.Size()));
        return true;
    }

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
