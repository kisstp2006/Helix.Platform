#include "file/file_internal.h"

#include <windows.h>

namespace helix::platform
{
    namespace
    {
        struct WidePath
        {
            wchar_t text[1024];
            bool ok;

            explicit WidePath(StringView path)
            {
                int n = MultiByteToWideChar(CP_UTF8, 0, path.Data(), int(path.Size()), text, 1023);
                ok = n > 0 || path.Empty();
                text[n > 0 ? n : 0] = 0;
                for (int i = 0; i < n; i++)
                    if (text[i] == L'/')
                        text[i] = L'\\';
            }
        };

        String FromWide(const wchar_t* text, int length)
        {
            char buffer[4096];
            int n = WideCharToMultiByte(CP_UTF8, 0, text, length, buffer, sizeof(buffer), nullptr, nullptr);
            for (int i = 0; i < n; i++)
                if (buffer[i] == '\\')
                    buffer[i] = '/';
            return String(StringView(buffer, n > 0 ? usize(n) : 0));
        }

        HANDLE H(i64 handle) { return reinterpret_cast<HANDLE>(handle); }
    }

    bool File::Open(StringView path, FileMode mode)
    {
        Close();
        WidePath w(path);
        if (!w.ok)
            return false;
        DWORD access = 0, creation = 0;
        switch (mode)
        {
        case FileMode::Read: access = GENERIC_READ; creation = OPEN_EXISTING; break;
        case FileMode::Write: access = GENERIC_WRITE; creation = CREATE_ALWAYS; break;
        case FileMode::Append: access = FILE_APPEND_DATA; creation = OPEN_ALWAYS; break;
        case FileMode::ReadWrite: access = GENERIC_READ | GENERIC_WRITE; creation = OPEN_ALWAYS; break;
        }
        HANDLE h = CreateFileW(w.text, access, FILE_SHARE_READ, nullptr, creation, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE)
            return false;
        handle_ = reinterpret_cast<i64>(h);
        return true;
    }

    void File::Close()
    {
        if (handle_ != -1)
            CloseHandle(H(handle_));
        handle_ = -1;
    }

    usize File::Read(void* buffer, usize size)
    {
        usize total = 0;
        while (total < size)
        {
            DWORD chunk = DWORD(Min<usize>(size - total, 1u << 30)), read = 0;
            if (!::ReadFile(H(handle_), static_cast<u8*>(buffer) + total, chunk, &read, nullptr) || read == 0)
                break;
            total += read;
        }
        return total;
    }

    usize File::Write(const void* data, usize size)
    {
        usize total = 0;
        while (total < size)
        {
            DWORD chunk = DWORD(Min<usize>(size - total, 1u << 30)), written = 0;
            if (!::WriteFile(H(handle_), static_cast<const u8*>(data) + total, chunk, &written, nullptr) || written == 0)
                break;
            total += written;
        }
        return total;
    }

    bool File::Seek(i64 offset)
    {
        LARGE_INTEGER li;
        li.QuadPart = offset;
        return SetFilePointerEx(H(handle_), li, nullptr, FILE_BEGIN) != 0;
    }

    i64 File::Tell() const
    {
        LARGE_INTEGER zero{}, pos{};
        return SetFilePointerEx(H(handle_), zero, &pos, FILE_CURRENT) ? pos.QuadPart : -1;
    }

    i64 File::Size() const
    {
        LARGE_INTEGER size;
        return GetFileSizeEx(H(handle_), &size) ? size.QuadPart : -1;
    }

    bool File::Flush() { return FlushFileBuffers(H(handle_)) != 0; }

    bool FileExists(StringView path)
    {
        WidePath w(path);
        DWORD a = GetFileAttributesW(w.text);
        return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool DirectoryExists(StringView path)
    {
        WidePath w(path);
        DWORD a = GetFileAttributesW(w.text);
        return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool CreateSingleDirectory(StringView path)
    {
        WidePath w(path);
        return CreateDirectoryW(w.text, nullptr) != 0;
    }

    bool RemoveFile(StringView path)
    {
        WidePath w(path);
        return DeleteFileW(w.text) != 0;
    }

    bool RemoveEmptyDirectory(StringView path)
    {
        WidePath w(path);
        return RemoveDirectoryW(w.text) != 0;
    }

    bool RenameFile(StringView from, StringView to)
    {
        WidePath a(from), b(to);
        return MoveFileExW(a.text, b.text, MOVEFILE_REPLACE_EXISTING) != 0;
    }

    bool ListDirectory(StringView path, DirectoryCallback callback, void* user)
    {
        String pattern = JoinPath(path, "*");
        WidePath w(pattern);
        WIN32_FIND_DATAW data;
        HANDLE find = FindFirstFileW(w.text, &data);
        if (find == INVALID_HANDLE_VALUE)
            return false;
        do
        {
            if (data.cFileName[0] == L'.' && (data.cFileName[1] == 0 || (data.cFileName[1] == L'.' && data.cFileName[2] == 0)))
                continue;
            String name = FromWide(data.cFileName, -1);
            if (!name.Empty() && name[name.Size() - 1] == '\0')
                name.Resize(name.Size() - 1);
            bool dir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            i64 size = dir ? 0 : i64((u64(data.nFileSizeHigh) << 32) | data.nFileSizeLow);
            callback({name.View(), dir, size}, user);
        } while (FindNextFileW(find, &data));
        FindClose(find);
        return true;
    }

    String CurrentDirectory()
    {
        wchar_t buffer[1024];
        DWORD n = GetCurrentDirectoryW(1024, buffer);
        return FromWide(buffer, int(n));
    }

    String ExecutablePath()
    {
        wchar_t buffer[1024];
        DWORD n = GetModuleFileNameW(nullptr, buffer, 1024);
        return FromWide(buffer, int(n));
    }

    String TempDirectory()
    {
        wchar_t buffer[1024];
        DWORD n = GetTempPathW(1024, buffer);
        String s = FromWide(buffer, int(n));
        if (!s.Empty() && s[s.Size() - 1] == '/')
            s.Resize(s.Size() - 1);
        return s;
    }
}
