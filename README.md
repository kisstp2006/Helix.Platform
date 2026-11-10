# Helix.Platform

Operating system layer of the Helix engine: time, threads and synchronization, file system,
dynamic libraries, windows, input events and the main loop.

## Backends

| Platform | Window and input | Threads, time, files |
|---|---|---|
| Windows | Win32, per-monitor DPI v2, UTF-16 text input | Win32 threads, SRW locks, high resolution waitable timers |
| Linux | X11, libX11 is loaded at runtime | pthreads, `clock_gettime` |
| Android | `ANativeWindow` and input fed by the player Java glue through `helix/platform/android.h` | pthreads |
| Web | Canvas and DOM events through the Emscripten HTML5 API, `requestAnimationFrame` main loop | Single threaded unless built with pthreads |

## Layout

| Path | Content |
|---|---|
| `include/helix/platform/time.h` | `Ticks`, `Now`, `SleepSeconds`, `Stopwatch` |
| `include/helix/platform/thread.h` | `Thread`, `Mutex`, `ConditionVariable`, `Semaphore`, `SpinLock`, core count |
| `include/helix/platform/file_system.h` | `File`, whole file read and write, directories, listing, paths |
| `include/helix/platform/dynamic_library.h` | `DynamicLibrary` |
| `include/helix/platform/input.h` | `KeyCode` (script API values), mouse buttons, `Event` |
| `include/helix/platform/window.h` | `Window`, `RunMainLoop` |
| `src/platform/<platform>/` | Backends, `posix/` is shared by Linux, Android and the web |
| `examples/window_demo.cpp` | Opens a window and logs every input event |

## Example

```cpp
using namespace helix::platform;
Window* window = Window::Create({.title = "Helix", .width = 1280, .height = 720});
helix::Array<Event> events;
RunMainLoop([](void* user) {
    auto* w = static_cast<Window*>(user);
    helix::Array<Event> events;
    w->PollEvents(events);
    return !w->CloseRequested();
}, window);
```

## Build

```
git clone --recursive https://github.com/kisstp2006/Helix.Platform
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
```

Linux needs `libx11-dev` for the headers. Window tests are skipped when no display is available.

## License

GPL-3.0, see `LICENSE`.
