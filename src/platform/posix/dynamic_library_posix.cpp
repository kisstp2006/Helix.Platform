#include <helix/platform/dynamic_library.h>

#include <dlfcn.h>

namespace helix::platform
{
    bool DynamicLibrary::Open(StringView path)
    {
        Close();
        char text[1024];
        usize n = path.Size() < sizeof(text) - 1 ? path.Size() : sizeof(text) - 1;
        for (usize i = 0; i < n; i++)
            text[i] = path[i];
        text[n] = 0;
        handle_ = dlopen(text, RTLD_NOW | RTLD_LOCAL);
        return handle_ != nullptr;
    }

    void DynamicLibrary::Close()
    {
        if (handle_)
            dlclose(handle_);
        handle_ = nullptr;
    }

    void* DynamicLibrary::Symbol(const char* name) const { return handle_ ? dlsym(handle_, name) : nullptr; }
}
