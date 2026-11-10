#pragma once

#include <Helix/Core/Strings/StringView.h>

namespace helix::platform
{
    class DynamicLibrary
    {
    public:
        DynamicLibrary() = default;
        ~DynamicLibrary() { Close(); }
        DynamicLibrary(const DynamicLibrary&) = delete;
        DynamicLibrary& operator=(const DynamicLibrary&) = delete;

        bool Open(StringView path);
        void Close();
        bool IsOpen() const { return handle_ != nullptr; }
        void* Symbol(const char* name) const;

        template <typename T>
        T Function(const char* name) const
        {
            return reinterpret_cast<T>(Symbol(name));
        }

    private:
        void* handle_ = nullptr;
    };
}
