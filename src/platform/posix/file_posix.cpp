#include "file/file_internal.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

namespace helix::platform
{
    namespace
    {
        struct CPath
        {
            char text[1024];
            explicit CPath(StringView path)
            {
                usize n = Min<usize>(path.Size(), sizeof(text) - 1);
                for (usize i = 0; i < n; i++)
                    text[i] = path[i];
                text[n] = 0;
            }
        };

        int Fd(i64 handle) { return int(handle); }
    }

    bool File::Open(StringView path, FileMode mode)
    {
        Close();
        CPath p(path);
        int flags = O_CLOEXEC;
        switch (mode)
        {
        case FileMode::Read: flags |= O_RDONLY; break;
        case FileMode::Write: flags |= O_WRONLY | O_CREAT | O_TRUNC; break;
        case FileMode::Append: flags |= O_WRONLY | O_CREAT | O_APPEND; break;
        case FileMode::ReadWrite: flags |= O_RDWR | O_CREAT; break;
        }
        int fd = open(p.text, flags, 0644);
        if (fd < 0)
            return false;
        handle_ = fd;
        return true;
    }

    void File::Close()
    {
        if (handle_ != -1)
            close(Fd(handle_));
        handle_ = -1;
    }

    usize File::Read(void* buffer, usize size)
    {
        usize total = 0;
        while (total < size)
        {
            ssize_t n = read(Fd(handle_), static_cast<u8*>(buffer) + total, size - total);
            if (n <= 0)
                break;
            total += usize(n);
        }
        return total;
    }

    usize File::Write(const void* data, usize size)
    {
        usize total = 0;
        while (total < size)
        {
            ssize_t n = write(Fd(handle_), static_cast<const u8*>(data) + total, size - total);
            if (n <= 0)
                break;
            total += usize(n);
        }
        return total;
    }

    bool File::Seek(i64 offset) { return lseek(Fd(handle_), off_t(offset), SEEK_SET) >= 0; }
    i64 File::Tell() const { return i64(lseek(Fd(handle_), 0, SEEK_CUR)); }

    i64 File::Size() const
    {
        struct stat st;
        return fstat(Fd(handle_), &st) == 0 ? i64(st.st_size) : -1;
    }

    bool File::Flush() { return fsync(Fd(handle_)) == 0; }

    bool FileExists(StringView path)
    {
        CPath p(path);
        struct stat st;
        return stat(p.text, &st) == 0 && S_ISREG(st.st_mode);
    }

    bool DirectoryExists(StringView path)
    {
        CPath p(path);
        struct stat st;
        return stat(p.text, &st) == 0 && S_ISDIR(st.st_mode);
    }

    bool CreateSingleDirectory(StringView path)
    {
        CPath p(path);
        return mkdir(p.text, 0755) == 0;
    }

    bool RemoveFile(StringView path)
    {
        CPath p(path);
        return unlink(p.text) == 0;
    }

    bool RemoveEmptyDirectory(StringView path)
    {
        CPath p(path);
        return rmdir(p.text) == 0;
    }

    bool RenameFile(StringView from, StringView to)
    {
        CPath a(from), b(to);
        return rename(a.text, b.text) == 0;
    }

    bool ListDirectory(StringView path, DirectoryCallback callback, void* user)
    {
        CPath p(path);
        DIR* dir = opendir(p.text);
        if (!dir)
            return false;
        while (dirent* e = readdir(dir))
        {
            StringView name(e->d_name);
            if (name == "." || name == "..")
                continue;
            String full = JoinPath(path, name);
            struct stat st;
            bool isDir = false;
            i64 size = 0;
            if (stat(full.CStr(), &st) == 0)
            {
                isDir = S_ISDIR(st.st_mode);
                size = isDir ? 0 : i64(st.st_size);
            }
            callback({name, isDir, size}, user);
        }
        closedir(dir);
        return true;
    }

    String CurrentDirectory()
    {
        char buffer[1024];
        return String(getcwd(buffer, sizeof(buffer)) ? buffer : "");
    }

    String ExecutablePath()
    {
#if HX_PLATFORM_WEB
        return String("/");
#else
        char buffer[1024];
        ssize_t n = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        return String(StringView(buffer, n > 0 ? usize(n) : 0));
#endif
    }

    String TempDirectory()
    {
        const char* t = getenv("TMPDIR");
#if HX_PLATFORM_ANDROID
        return String(t ? t : "/data/local/tmp");
#else
        return String(t ? t : "/tmp");
#endif
    }
}
