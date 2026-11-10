#include <helix/core/log/log.h>
#include <helix/platform/thread.h>
#include <helix/platform/time.h>
#include <helix/platform/window.h>

using namespace helix;
using namespace helix::platform;

namespace
{
    LogChannel DemoLog{"Demo"};

    struct Demo
    {
        Window* window;
        Array<Event> events;
        Stopwatch clock;
        u64 frames = 0;
        f64 autoCloseSeconds = 0;
    };

    void Print(const Event& e)
    {
        switch (e.type)
        {
        case EventType::KeyDown:
        case EventType::KeyUp:
            HX_LOG_INFO(DemoLog, "{} {} mods={} repeat={}", e.type == EventType::KeyDown ? "key down" : "key up", KeyCodeName(e.key.key), e.modifiers, e.repeat);
            break;
        case EventType::Text:
            HX_LOG_INFO(DemoLog, "text U+{:04X}", e.text.codepoint);
            break;
        case EventType::MouseDown:
        case EventType::MouseUp:
            HX_LOG_INFO(DemoLog, "mouse {} button {} at {} {}", e.type == EventType::MouseDown ? "down" : "up", int(e.mouse.button), e.mouse.x, e.mouse.y);
            break;
        case EventType::MouseWheel:
            HX_LOG_INFO(DemoLog, "wheel {} {}", e.wheel.dx, e.wheel.dy);
            break;
        case EventType::WindowResize:
            HX_LOG_INFO(DemoLog, "resize {} x {}", e.resize.width, e.resize.height);
            break;
        case EventType::WindowFocus:
        case EventType::WindowBlur:
            HX_LOG_INFO(DemoLog, "{}", e.type == EventType::WindowFocus ? "focus" : "blur");
            break;
        case EventType::TouchBegin:
            HX_LOG_INFO(DemoLog, "touch {} at {} {}", e.touch.id, e.touch.x, e.touch.y);
            break;
        case EventType::WindowClose:
            HX_LOG_INFO(DemoLog, "close requested");
            break;
        default:
            break;
        }
    }

    bool Frame(void* user)
    {
        auto* demo = static_cast<Demo*>(user);
        demo->events.Clear();
        demo->window->PollEvents(demo->events);
        for (const Event& e : demo->events)
            Print(e);
        demo->frames++;
        if (demo->autoCloseSeconds > 0 && demo->clock.ElapsedSeconds() > demo->autoCloseSeconds)
            return false;
        if (demo->window->CloseRequested())
        {
            HX_LOG_INFO(DemoLog, "{} frames in {:.2} s", demo->frames, demo->clock.ElapsedSeconds());
            return false;
        }
        SleepSeconds(1.0 / 120.0);
        return true;
    }
}

int main(int argc, char** argv)
{
    static Demo demo;
    for (int i = 1; i < argc; i++)
        if (StringView(argv[i]) == "--auto-close")
            demo.autoCloseSeconds = 2.0;
    demo.window = Window::Create({.title = "Helix window demo", .width = 960, .height = 540});
    if (!demo.window)
    {
        HX_LOG_ERROR(DemoLog, "no window support on this system");
        return 1;
    }
    i32 w, h;
    demo.window->GetFramebufferSize(w, h);
    HX_LOG_INFO(DemoLog, "window {} x {} pixels, scale {}, {} cores", w, h, demo.window->GetContentScale(), LogicalCoreCount());
    RunMainLoop(&Frame, &demo);
#if !HX_PLATFORM_WEB
    Window::Destroy(demo.window);
#endif
    return 0;
}
