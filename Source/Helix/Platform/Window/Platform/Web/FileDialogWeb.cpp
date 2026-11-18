#include <Helix/Platform/Window/FileDialog.h>

// No file dialogs here: every call answers as if cancelled.
namespace helix::platform
{
    bool HasFileDialogs() { return false; }
    String OpenFileDialog(const FileDialogDesc&) { return String(); }
    String SaveFileDialog(const FileDialogDesc&) { return String(); }
    String OpenFolderDialog(const FileDialogDesc&) { return String(); }
}
