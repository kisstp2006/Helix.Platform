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

    Window* Window::Create(const WindowDesc&) { return New<AndroidWindow>(HeapAllocator()); }

    void Window::Destroy(Window* window) { Delete(HeapAllocator(), static_cast<AndroidWindow*>(window)); }

    void RunMainLoop(FrameFunction frame, void* user)
    {
        while (frame(user))
        {
        }
    }
}
