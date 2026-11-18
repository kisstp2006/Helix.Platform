#include <Helix/Platform/Window/FileDialog.h>

#include <Helix/Core/Strings/Format.h>

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// The dialogs of zenity (GTK) or kdialog (KDE), run as processes: the path comes on their standard output.
namespace helix::platform
{
    namespace
    {
        // A program of the PATH ("" when none).
        String Find(StringView name)
        {
            const char* path = getenv("PATH");
            if (!path)
                return String();
            StringView all(path);
            usize at = 0;
            while (at <= all.Size())
            {
                usize end = all.Find(':', at);
                if (end == StringView::npos)
                    end = all.Size();
                String full = Format("{}/{}", all.Substr(at, end - at), name);
                if (end > at && access(full.Data(), X_OK) == 0)
                    return full;
                at = end + 1;
            }
            return String();
        }

        enum class Kind
        {
            Open,
            Save,
            Folder
        };

        String Patterns(const FileDialogFilter& f)
        {
            String out;
            usize at = 0;
            StringView exts = f.extensions.View();
            while (at <= exts.Size())
            {
                usize end = exts.Find(';', at);
                if (end == StringView::npos)
                    end = exts.Size();
                StringView e = exts.Substr(at, end - at).Trimmed();
                if (!e.Empty())
                {
                    if (!out.Empty())
                        out.Append(' ');
                    out.Append(e == "*" ? StringView("*") : StringView("*."));
                    if (e != "*")
                        out.Append(e);
                }
                at = end + 1;
            }
            return out;
        }

        // Runs the program; its first line of output when it ends with 0.
        String Run(const String& program, const Array<String>& args)
        {
            int fds[2];
            if (pipe(fds) != 0)
                return String();
            pid_t pid = fork();
            if (pid == 0)
            {
                dup2(fds[1], STDOUT_FILENO);
                close(fds[0]);
                close(fds[1]);
                Array<char*> argv;
                argv.Push(const_cast<char*>(program.Data()));
                for (const String& a : args)
                    argv.Push(const_cast<char*>(a.Data()));
                argv.Push(nullptr);
                execv(program.Data(), argv.Data());
                _exit(127);
            }
            close(fds[1]);
            String out;
            char buffer[512];
            ssize_t n;
            while ((n = read(fds[0], buffer, sizeof(buffer))) > 0)
                out.Append(StringView(buffer, usize(n)));
            close(fds[0]);
            int status = 0;
            if (pid < 0 || waitpid(pid, &status, 0) < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
                return String();
            usize newline = out.View().Find('\n');
            return newline == StringView::npos ? out : String(out.View().Substr(0, newline));
        }

        String Show(const FileDialogDesc& desc, Kind kind)
        {
            String start = desc.directory;
            if (!desc.defaultName.Empty())
                start = start.Empty() ? desc.defaultName : Format("{}/{}", desc.directory, desc.defaultName);
            else if (!start.Empty())
                start.Append('/');
            Array<String> args;
            String zenity = Find("zenity");
            if (!zenity.Empty())
            {
                args.Push(String("--file-selection"));
                if (!desc.title.Empty())
                    args.Push(Format("--title={}", desc.title));
                if (kind == Kind::Save)
                {
                    args.Push(String("--save"));
                    args.Push(String("--confirm-overwrite"));
                }
                if (kind == Kind::Folder)
                    args.Push(String("--directory"));
                if (!start.Empty())
                    args.Push(Format("--filename={}", start));
                if (kind != Kind::Folder)
                    for (const FileDialogFilter& f : desc.filters)
                        args.Push(Format("--file-filter={} | {}", f.name, Patterns(f)));
                return Run(zenity, args);
            }
            String kdialog = Find("kdialog");
            if (kdialog.Empty())
                return String();
            args.Push(String(kind == Kind::Open ? "--getopenfilename" : kind == Kind::Save ? "--getsavefilename" : "--getexistingdirectory"));
            args.Push(start.Empty() ? String(".") : start);
            if (kind != Kind::Folder && !desc.filters.Empty())
            {
                String filter;
                for (const FileDialogFilter& f : desc.filters)
                {
                    if (!filter.Empty())
                        filter.Append('\n');
                    filter.Append(Format("{}|{}", Patterns(f), f.name));
                }
                args.Push(filter);
            }
            if (!desc.title.Empty())
            {
                args.Push(String("--title"));
                args.Push(desc.title);
            }
            return Run(kdialog, args);
        }

        String WithExtension(String path, const FileDialogDesc& desc)
        {
            if (path.Empty() || desc.filters.Empty())
                return path;
            usize slash = path.View().FindLast('/');
            StringView name = slash == StringView::npos ? path.View() : path.View().Substr(slash + 1);
            if (name.Find('.') != StringView::npos)
                return path;
            StringView first = desc.filters[0].extensions.View();
            usize semi = first.Find(';');
            StringView ext = semi == StringView::npos ? first : first.Substr(0, semi);
            if (!ext.Empty() && ext != "*")
                path.Append(Format(".{}", ext));
            return path;
        }
    }

    bool HasFileDialogs() { return !Find("zenity").Empty() || !Find("kdialog").Empty(); }
    String OpenFileDialog(const FileDialogDesc& desc) { return Show(desc, Kind::Open); }
    String SaveFileDialog(const FileDialogDesc& desc) { return WithExtension(Show(desc, Kind::Save), desc); }
    String OpenFolderDialog(const FileDialogDesc& desc) { return Show(desc, Kind::Folder); }
}
