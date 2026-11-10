#include "file/file_internal.h"

namespace helix::platform
{
    namespace
    {
        bool IsSeparator(char c) { return c == '/' || c == '\\'; }

        usize LastSeparator(StringView path)
        {
            for (usize i = path.Size(); i-- > 0;)
                if (IsSeparator(path[i]))
                    return i;
            return StringView::npos;
        }
    }

    String JoinPath(StringView a, StringView b)
    {
        String out(a);
        if (!a.Empty() && !b.Empty() && !IsSeparator(a[a.Size() - 1]))
            out.Append('/');
        while (!b.Empty() && !a.Empty() && IsSeparator(b[0]))
            b = b.Substr(1);
        out.Append(b);
        return out;
    }

    StringView GetFileName(StringView path)
    {
        usize s = LastSeparator(path);
        return s == StringView::npos ? path : path.Substr(s + 1);
    }

    StringView GetExtension(StringView path)
    {
        StringView name = GetFileName(path);
        usize dot = name.FindLast('.');
        return dot == StringView::npos || dot == 0 ? StringView() : name.Substr(dot);
    }

    StringView GetDirectoryName(StringView path)
    {
        usize s = LastSeparator(path);
        return s == StringView::npos ? StringView() : path.Substr(0, s);
    }

    File& File::operator=(File&& other) noexcept
    {
        if (this != &other)
        {
            Close();
            handle_ = other.handle_;
            other.handle_ = -1;
        }
        return *this;
    }

    bool ReadFile(StringView path, Array<u8>& out)
    {
        File f;
        if (!f.Open(path, FileMode::Read))
            return false;
        i64 size = f.Size();
        if (size < 0)
            return false;
        out.Resize(usize(size));
        return f.Read(out.Data(), usize(size)) == usize(size);
    }

    bool WriteFile(StringView path, Span<const u8> data)
    {
        File f;
        if (!f.Open(path, FileMode::Write))
            return false;
        return f.Write(data.Data(), data.Size()) == data.Size();
    }

    bool CreateDirectories(StringView path)
    {
        if (path.Empty() || DirectoryExists(path))
            return true;
        StringView parent = GetDirectoryName(path);
        if (!parent.Empty() && parent != path && !CreateDirectories(parent))
            return false;
        return CreateSingleDirectory(path) || DirectoryExists(path);
    }
}
