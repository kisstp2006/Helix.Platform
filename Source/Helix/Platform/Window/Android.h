#pragma once

#include <Helix/Core/Containers/Array.h>
#include <Helix/Core/Strings/String.h>
#include <Helix/Platform/Window/Input.h>

#include <jni.h>

struct ANativeWindow;
struct AAssetManager;

// Android: the activity of Helix.Platform (Android/ in this repo: Java and its JNI glue) feeds the native window, input
// and focus, and runs the application's HelixMain on a thread of its own. The application reads its packaged files
// through the APK's assets and tells which surface its graphics device uses, so the activity can let a surface go.
namespace helix::platform::android
{
    // Called from the activity's JNI glue.
    void SetNativeWindow(ANativeWindow* window);
    ANativeWindow* GetNativeWindow();
    void PushEvent(const Event& event);
    void SetContentScale(f32 scale);
    void SetFocused(bool focused);
    // The activity (a global reference is taken; null clears it) and the virtual machine.
    void SetActivity(JavaVM* vm, jobject activity);
    void SetAssetManager(AAssetManager* manager);
    // The folder the APK's native libraries are installed to, the application's private files and the installed
    // version of the package (its last update time).
    void SetApplicationFolders(StringView nativeLibraries, StringView files, u64 packageVersion);
    // The calling thread attaches to the virtual machine (and detaches); the glue's application thread does.
    void AttachThread();
    void DetachThread();
    // Blocks the UI thread until the application's device no longer uses the window (a second at most).
    void WaitForNativeWindowRelease(ANativeWindow* window);

    // Called by the application.
    // A file of the APK's assets by its path there; false when it is not packaged.
    bool ReadAsset(StringView path, Array<u8>& out);
    // The names of the files of an asset folder (its folders are not listed); false when it has none.
    bool ListAssets(StringView folder, Array<String>& out);
    // The files of an asset folder copied into a folder, again only when the package was installed or updated since
    // (a stamp file in the folder); false when a file could not be written.
    bool ExtractAssets(StringView folder, StringView destination);
    StringView NativeLibraryDirectory();
    StringView FilesDirectory();
    // Each frame: the window the graphics device uses now (null for none).
    void AcknowledgeNativeWindow(ANativeWindow* window);
    // Ends the activity (the application's run is over).
    void FinishActivity();
}

// The application's entry point on Android, run on the activity's application thread with the intent's arguments.
int HelixMain(int argc, char** argv);
