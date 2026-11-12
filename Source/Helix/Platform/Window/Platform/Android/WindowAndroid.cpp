#include <Helix/Platform/Window/Android.h>
#include <Helix/Platform/Threading/Thread.h>
#include <Helix/Platform/Window/Window.h>

#include <android/native_window.h>

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

    void RunMainLoop(FrameFunction frame, void* user)
    {
        while (frame(user))
        {
        }
    }
}
