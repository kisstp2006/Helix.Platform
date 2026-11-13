#include <Helix/Platform/Window/Android.h>
#include <Helix/Core/Strings/Format.h>
#include <Helix/Platform/IO/FileSystem.h>
#include <Helix/Platform/Threading/Thread.h>
#include <Helix/Platform/Window/Window.h>

#include <Helix/Platform/Time/Time.h>

#include <android/asset_manager.h>
#include <android/native_window.h>

#include <atomic>

namespace helix::platform
{
    namespace
    {
        Mutex& StateMutex()
        {
            static Mutex m;
            return m;
        }

        ANativeWindow* nativeWindow = nullptr;
        f32 contentScale = 1.0f;
        bool hasFocus = true;
        Array<Event>* queue = nullptr;

        Array<Event>& Queue()
        {
            if (!queue)
                queue = New<Array<Event>>(HeapAllocator());
            return *queue;
        }

        class AndroidWindow final : public Window
        {
        public:
            void PollEvents(Array<Event>& out) override
            {
                ScopedLock lock(StateMutex());
                for (const Event& e : Queue())
                {
                    if (e.type == EventType::WindowClose || e.type == EventType::Quit)
                        closeRequested_ = true;
                    out.Push(e);
                }
                Queue().Clear();
            }

            void SetTitle(StringView) override {}

            void GetSize(i32& width, i32& height) const override
            {
                GetFramebufferSize(width, height);
                width = i32(f32(width) / contentScale);
                height = i32(f32(height) / contentScale);
            }

            void GetFramebufferSize(i32& width, i32& height) const override
            {
                ScopedLock lock(StateMutex());
                width = nativeWindow ? ANativeWindow_getWidth(nativeWindow) : 0;
                height = nativeWindow ? ANativeWindow_getHeight(nativeWindow) : 0;
            }

            f32 GetContentScale() const override { return contentScale; }
            NativeWindowHandle GetNativeHandle() const override { return {android::GetNativeWindow(), nullptr}; }
            bool HasFocus() const override { return hasFocus; }
            // The activity's surface always fills the screen.
            void SetSize(i32, i32) override {}
            void GetPosition(i32& x, i32& y) const override
            {
                x = 0;
                y = 0;
            }
            void SetPosition(i32, i32) override {}
            bool SetMode(WindowMode next, u32, const DisplayMode&) override { return next == WindowMode::FullScreen; }
            WindowMode GetMode() const override { return WindowMode::FullScreen; }
            u32 GetDisplay() const override { return 0; }
        };
    }

    namespace android
    {
        void SetNativeWindow(ANativeWindow* window)
        {
            ScopedLock lock(StateMutex());
            nativeWindow = window;
        }

        ANativeWindow* GetNativeWindow()
        {
            ScopedLock lock(StateMutex());
            return nativeWindow;
        }

        void PushEvent(const Event& event)
        {
            ScopedLock lock(StateMutex());
            Queue().Push(event);
        }

        void SetContentScale(f32 scale) { contentScale = scale > 0.0f ? scale : 1.0f; }

        void SetFocused(bool focused)
        {
            hasFocus = focused;
            Event e;
            e.type = focused ? EventType::WindowFocus : EventType::WindowBlur;
            PushEvent(e);
        }
    }

    bool Window::IsSupported() { return true; }

    namespace
    {
        JavaVM* javaVm = nullptr;
        jobject activityObject = nullptr;
        AAssetManager* assetManager = nullptr;
        String nativeLibraryFolder, filesFolder;
        u64 installedVersion = 0;
        thread_local JNIEnv* threadEnv = nullptr;
        std::atomic<ANativeWindow*> usedWindow{nullptr};

        JNIEnv* Env()
        {
            if (threadEnv)
                return threadEnv;
            JNIEnv* env = nullptr;
            if (javaVm && javaVm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK)
                return env;
            return nullptr;
        }

        // A void method of the activity; with an argument when `hasValue`.
        void CallActivity(const char* method, const char* signature, jvalue value, bool hasValue)
        {
            JNIEnv* env = Env();
            if (!env || !activityObject)
                return;
            jclass c = env->GetObjectClass(activityObject);
            jmethodID m = env->GetMethodID(c, method, signature);
            if (m)
            {
                if (hasValue)
                    env->CallVoidMethodA(activityObject, m, &value);
                else
                    env->CallVoidMethod(activityObject, m);
            }
            if (env->ExceptionCheck())
                env->ExceptionClear();
            env->DeleteLocalRef(c);
        }
    }

    void SetKeepScreenOn(bool keepOn)
    {
        jvalue v;
        v.z = keepOn ? JNI_TRUE : JNI_FALSE;
        CallActivity("setKeepScreenOn", "(Z)V", v, true);
    }

    void SetDisplayOrientation(DisplayOrientation orientation)
    {
        jvalue v;
        v.i = jint(orientation);
        CallActivity("setOrientation", "(I)V", v, true);
    }

    namespace android
    {
        void SetActivity(JavaVM* vm, jobject activity)
        {
            JNIEnv* env = nullptr;
            JavaVM* machine = vm ? vm : javaVm;
            if (machine)
                machine->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
            if (activityObject && env)
                env->DeleteGlobalRef(activityObject);
            activityObject = activity && env ? env->NewGlobalRef(activity) : nullptr;
            javaVm = vm;
        }

        void SetAssetManager(AAssetManager* manager) { assetManager = manager; }

        void AttachThread()
        {
            if (javaVm && !threadEnv)
                javaVm->AttachCurrentThread(&threadEnv, nullptr);
        }

        void DetachThread()
        {
            if (javaVm && threadEnv)
            {
                javaVm->DetachCurrentThread();
                threadEnv = nullptr;
            }
        }

        void WaitForNativeWindowRelease(ANativeWindow* window)
        {
            for (int i = 0; i < 100 && window && usedWindow.load() == window; i++)
                SleepSeconds(0.01);
        }

        bool ReadAsset(StringView path, Array<u8>& out)
        {
            if (!assetManager)
                return false;
            String name(path);
            AAsset* asset = AAssetManager_open(assetManager, name.CStr(), AASSET_MODE_BUFFER);
            if (!asset)
                return false;
            off64_t size = AAsset_getLength64(asset);
            out.Resize(usize(size));
            bool ok = size == 0 || AAsset_read(asset, out.Data(), usize(size)) == int(size);
            AAsset_close(asset);
            return ok;
        }

        void SetApplicationFolders(StringView nativeLibraries, StringView files, u64 packageVersion)
        {
            nativeLibraryFolder = String(nativeLibraries);
            filesFolder = String(files);
            installedVersion = packageVersion;
        }

        StringView NativeLibraryDirectory() { return nativeLibraryFolder; }
        StringView FilesDirectory() { return filesFolder; }

        bool ListAssets(StringView folder, Array<String>& out)
        {
            out.Clear();
            if (!assetManager)
                return false;
            String name(folder);
            AAssetDir* dir = AAssetManager_openDir(assetManager, name.CStr());
            if (!dir)
                return false;
            while (const char* file = AAssetDir_getNextFileName(dir))
                out.Push(String(StringView(file)));
            AAssetDir_close(dir);
            return !out.Empty();
        }

        bool ExtractAssets(StringView folder, StringView destination)
        {
            String stamp = JoinPath(destination, ".helix-extracted");
            String version = Format("{}", installedVersion);
            Array<u8> existing;
            if (installedVersion != 0 && ReadFile(stamp, existing) && StringView(reinterpret_cast<const char*>(existing.Data()), existing.Size()) == version.View())
                return true;
            Array<String> files;
            ListAssets(folder, files);
            CreateDirectories(destination);
            Array<u8> bytes;
            for (const String& file : files)
            {
                if (!ReadAsset(JoinPath(folder, file), bytes) || !WriteFile(JoinPath(destination, file), Span<const u8>(bytes.Data(), bytes.Size())))
                    return false;
            }
            return WriteFile(stamp, Span<const u8>(reinterpret_cast<const u8*>(version.Data()), version.Size()));
        }

        void AcknowledgeNativeWindow(ANativeWindow* window) { usedWindow.store(window); }

        void FinishActivity() { CallActivity("finishFromNative", "()V", jvalue{}, false); }
    }

    // One display: the surface (density from the content scale, 160 dpi at scale 1).
    void GetDisplays(Array<DisplayInfo>& out)
    {
        out.Clear();
        DisplayInfo d;
        d.name = String("Android display");
        {
            ScopedLock lock(StateMutex());
            d.width = nativeWindow ? ANativeWindow_getWidth(nativeWindow) : 0;
            d.height = nativeWindow ? ANativeWindow_getHeight(nativeWindow) : 0;
        }
        d.workWidth = d.width;
        d.workHeight = d.height;
        d.dpi = 160.0f * contentScale;
        d.primary = true;
        out.Push(Move(d));
    }

    void GetDisplayModes(u32 display, Array<DisplayMode>& out)
    {
        out.Clear();
        Array<DisplayInfo> displays;
        GetDisplays(displays);
        if (display < displays.Size())
            out.Push({displays[display].width, displays[display].height, 0, 1});
    }

    Window* Window::Create(const WindowDesc&) { return New<AndroidWindow>(HeapAllocator()); }

    void Window::Destroy(Window* window) { Delete(HeapAllocator(), static_cast<AndroidWindow*>(window)); }

    void RunMainLoop(FrameFunction frame, void* user, bool)
    {
        while (frame(user))
        {
        }
    }
}
