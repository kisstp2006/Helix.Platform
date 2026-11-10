#pragma once

#include <helix/core/containers/array.h>
#include <helix/core/strings/string.h>

namespace helix::platform
{
    enum class FileMode : u8
    {
        Read,
        Write,
        Append,
        ReadWrite,
    };

    class File
    {
    public:
        File() = default;
        ~File() { Close(); }
        File(File&& other) noexcept : handle_(other.handle_) { other.handle_ = -1; }
        File& operator=(File&& other) noexcept;
        File(const File&) = delete;
        File& operator=(const File&) = delete;

        bool Open(StringView path, FileMode mode);
        void Close();
        bool IsOpen() const { return handle_ != -1; }

        usize Read(void* buffer, usize size);
        usize Write(const void* data, usize size);
        bool Seek(i64 offset);
        i64 Tell() const;
        i64 Size() const;
        bool Flush();

    private:
        i64 handle_ = -1;
    };

    struct DirectoryEntry
    {
        StringView name;
        bool isDirectory;
        i64 size;
    };

    using DirectoryCallback = void (*)(const DirectoryEntry& entry, void* user);

    bool ReadFile(StringView path, Array<u8>& out);
    bool WriteFile(StringView path, Span<const u8> data);
    bool FileExists(StringView path);
    bool DirectoryExists(StringView path);
    bool CreateDirectories(StringView path);
    bool RemoveFile(StringView path);
    bool RemoveEmptyDirectory(StringView path);
    bool RenameFile(StringView from, StringView to);
    bool ListDirectory(StringView path, DirectoryCallback callback, void* user);
    String CurrentDirectory();
    String ExecutablePath();
    String TempDirectory();

    String JoinPath(StringView a, StringView b);
    StringView GetFileName(StringView path);
    StringView GetExtension(StringView path);
    StringView GetDirectoryName(StringView path);
}
