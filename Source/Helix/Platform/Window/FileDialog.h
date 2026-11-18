#pragma once

#include <Helix/Core/Containers/Array.h>
#include <Helix/Core/Strings/String.h>

// The system's file dialogs: a file to open, a file to save, a folder. Windows has its shell dialogs; Linux runs zenity
// or kdialog when one of them is installed; Android and the Web have none. Each call blocks until the user answers.
namespace helix::platform
{
    class Window;

    struct FileDialogFilter
    {
        // What the dialog shows ("Scene"), and the extensions it takes, without dots, separated by ';' ("helix").
        String name;
        String extensions;
    };

    struct FileDialogDesc
    {
        String title;
        // The folder it starts in ("" for the system's choice), and a save dialog's file name.
        String directory;
        String defaultName;
        Array<FileDialogFilter> filters = Array<FileDialogFilter>();
        // The window it belongs to (null: the active one).
        Window* owner = nullptr;
    };

    // Whether this system shows file dialogs at all.
    bool HasFileDialogs();
    // The chosen path (forward slashes); "" when cancelled or when the system has no dialog.
    String OpenFileDialog(const FileDialogDesc& desc);
    // A save dialog adds the first filter's first extension to a name without one.
    String SaveFileDialog(const FileDialogDesc& desc);
    String OpenFolderDialog(const FileDialogDesc& desc);
}
