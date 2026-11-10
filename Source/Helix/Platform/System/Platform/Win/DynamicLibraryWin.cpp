#include <Helix/Platform/System/DynamicLibrary.h>

#include <windows.h>

namespace helix::platform
{
    bool DynamicLibrary::Open(StringView path)
    {
        Close();
        wchar_t wide[1024];
        int n = MultiByteToWideChar(CP_UTF8, 0, path.Data(), int(path.Size()), wide, 1023);
        wide[n > 0 ? n : 0] = 0;
        handle_ = LoadLibraryW(wide);
        return handle_ != nullptr;
    }

    void DynamicLibrary::Close()
    {
        if (handle_)
            FreeLibrary(static_cast<HMODULE>(handle_));
        handle_ = nullptr;
    }

    void* DynamicLibrary::Symbol(const char* name) const
    {
        return handle_ ? reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle_), name)) : nullptr;
    }
}
