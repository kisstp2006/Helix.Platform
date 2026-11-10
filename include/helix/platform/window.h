#pragma once

#include <helix/core/containers/array.h>
#include <helix/core/strings/string.h>
#include <helix/platform/input.h>

namespace helix::platform
{
    struct WindowDesc
    {
        StringView title = "Helix";
        i32 width = 1280;
        i32 height = 720;
        bool resizable = true;
        bool visible = true;
        StringView canvasSelector = "#canvas";
    };

    struct NativeWindowHandle
    {
        void* window = nullptr;
        void* display = nullptr;
    };

    class Window
    {
    public:
        static Window* Create(const WindowDesc& desc);
        static void Destroy(Window* window);
        static bool IsSupported();

        virtual ~Window() = default;
        virtual void PollEvents(Array<Event>& out) = 0;
        virtual void SetTitle(StringView title) = 0;
        virtual void GetSize(i32& width, i32& height) const = 0;
        virtual void GetFramebufferSize(i32& width, i32& height) const = 0;
        virtual f32 GetContentScale() const = 0;
        virtual NativeWindowHandle GetNativeHandle() const = 0;
        virtual bool HasFocus() const = 0;
        bool CloseRequested() const { return closeRequested_; }

    protected:
        bool closeRequested_ = false;
    };

    using FrameFunction = bool (*)(void* user);

    // Native platforms loop until the frame function returns false, the web yields to the browser each frame.
    void RunMainLoop(FrameFunction frame, void* user);
}
