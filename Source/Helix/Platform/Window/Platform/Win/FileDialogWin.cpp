#include <Helix/Platform/Window/FileDialog.h>

#include <Helix/Platform/Window/Window.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shobjidl.h>

namespace helix::platform
{
    namespace
    {
        Array<wchar_t> Wide(StringView text)
        {
            Array<wchar_t> out;
            int n = MultiByteToWideChar(CP_UTF8, 0, text.Data(), int(text.Size()), nullptr, 0);
            out.Resize(usize(n) + 1);
            if (n > 0)
                MultiByteToWideChar(CP_UTF8, 0, text.Data(), int(text.Size()), out.Data(), n);
            out[usize(n)] = 0;
            return out;
        }

        String Narrow(const wchar_t* text)
        {
            int n = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
            if (n <= 1)
                return String();
            Array<char> buffer;
            buffer.Resize(usize(n));
            WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer.Data(), n, nullptr, nullptr);
            String out(StringView(buffer.Data(), usize(n - 1)));
            for (usize i = 0; i < out.Size(); i++)
                if (out[i] == '\\')
                    out[i] = '/';
            return out;
        }

        enum class Kind
        {
            Open,
            Save,
            Folder
        };

        String Show(const FileDialogDesc& desc, Kind kind)
        {
            // COM on this thread (already there, in either model, is fine).
            HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
            bool uninit = SUCCEEDED(init);
            String result;
            IFileDialog* dialog = nullptr;
            HRESULT hr = kind == Kind::Save
                             ? CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(reinterpret_cast<IFileSaveDialog**>(&dialog)))
                             : CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(reinterpret_cast<IFileOpenDialog**>(&dialog)));
            if (SUCCEEDED(hr) && dialog)
            {
                DWORD options = 0;
                dialog->GetOptions(&options);
                options |= FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR;
                if (kind == Kind::Folder)
                    options |= FOS_PICKFOLDERS;
                if (kind == Kind::Open)
                    options |= FOS_FILEMUSTEXIST;
                if (kind == Kind::Save)
                    options |= FOS_OVERWRITEPROMPT;
                dialog->SetOptions(options);
                if (!desc.title.Empty())
                    dialog->SetTitle(Wide(desc.title.View()).Data());
                // The filters: "Name" and "*.a;*.b".
                Array<Array<wchar_t>> names, specs;
                Array<COMDLG_FILTERSPEC> filters;
                if (kind != Kind::Folder)
                    for (const FileDialogFilter& f : desc.filters)
                    {
                        String spec;
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
                                if (!spec.Empty())
                                    spec.Append(';');
                                spec.Append(e == "*" ? StringView("*.*") : StringView("*."));
                                if (e != "*")
                                    spec.Append(e);
                            }
                            at = end + 1;
                        }
                        names.Push(Wide(f.name.View()));
                        specs.Push(Wide(spec.Empty() ? StringView("*.*") : spec.View()));
                    }
                for (usize i = 0; i < names.Size(); i++)
                    filters.Push(COMDLG_FILTERSPEC{names[i].Data(), specs[i].Data()});
                if (!filters.Empty())
                    dialog->SetFileTypes(UINT(filters.Size()), filters.Data());
                if (kind == Kind::Save && !desc.filters.Empty())
                {
                    StringView first = desc.filters[0].extensions.View();
                    usize semi = first.Find(';');
                    StringView ext = semi == StringView::npos ? first : first.Substr(0, semi);
                    if (!ext.Empty() && ext != "*")
                        dialog->SetDefaultExtension(Wide(ext).Data());
                }
                if (!desc.defaultName.Empty())
                    dialog->SetFileName(Wide(desc.defaultName.View()).Data());
                if (!desc.directory.Empty())
                {
                    String dir(desc.directory);
                    for (usize i = 0; i < dir.Size(); i++)
                        if (dir[i] == '/')
                            dir[i] = '\\';
                    IShellItem* folder = nullptr;
                    if (SUCCEEDED(SHCreateItemFromParsingName(Wide(dir.View()).Data(), nullptr, IID_PPV_ARGS(&folder))) && folder)
                    {
                        dialog->SetFolder(folder);
                        folder->Release();
                    }
                }
                HWND owner = desc.owner ? static_cast<HWND>(desc.owner->GetNativeHandle().window) : GetActiveWindow();
                if (SUCCEEDED(dialog->Show(owner)))
                {
                    IShellItem* item = nullptr;
                    if (SUCCEEDED(dialog->GetResult(&item)) && item)
                    {
                        PWSTR path = nullptr;
                        if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path)
                        {
                            result = Narrow(path);
                            CoTaskMemFree(path);
                        }
                        item->Release();
                    }
                }
                dialog->Release();
            }
            if (uninit)
                CoUninitialize();
            return result;
        }
    }

    bool HasFileDialogs() { return true; }
    String OpenFileDialog(const FileDialogDesc& desc) { return Show(desc, Kind::Open); }
    String SaveFileDialog(const FileDialogDesc& desc) { return Show(desc, Kind::Save); }
    String OpenFolderDialog(const FileDialogDesc& desc) { return Show(desc, Kind::Folder); }
}
