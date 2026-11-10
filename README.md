# Helix.Platform

Operating system layer of the Helix engine: time, threads and synchronization, file system,
dynamic libraries, windows, input events and the main loop.

## Backends

| Platform | Window and input | Threads, time, files |
|---|---|---|
| Windows | Win32, per-monitor DPI v2, UTF-16 text input | Win32 threads, SRW locks, high resolution waitable timers |
| Linux | X11, libX11 is loaded at runtime | pthreads, `clock_gettime` |
| Android | `ANativeWindow` and input fed by the player Java glue through `Helix/Platform/Window/Android.h` | pthreads |
| Web | Canvas and DOM events through the Emscripten HTML5 API, `requestAnimationFrame` main loop | Single threaded unless built with pthreads |

## Layout

| Path | Content |
|---|---|
| `Source/Helix/Platform/Time/Time.h` | `Ticks`, `Now`, `SleepSeconds`, `Stopwatch` |
| `Source/Helix/Platform/Threading/Thread.h` | `Thread`, `Mutex`, `ConditionVariable`, `Semaphore`, `SpinLock`, core count |
| `Source/Helix/Platform/IO/FileSystem.h` | `File`, whole file read and write, directories, listing, paths |
| `Source/Helix/Platform/System/DynamicLibrary.h` | `DynamicLibrary` |
| `Source/Helix/Platform/Window/Input.h` | `KeyCode` (script API values), mouse buttons, `Event` |
| `Source/Helix/Platform/Window/Window.h` | `Window`, `RunMainLoop` |
| `.../<Topic>/Platform/<Os>/` | Backends (`Win`, `Linux`, `Android`, `Web`), `Posix` is shared by Linux, Android and the web |
| `Examples/WindowDemo/WindowDemo.cpp` | Opens a window and logs every input event |

## Example

```cpp
using namespace helix::platform;
Window* window = Window::Create({.title = "Helix", .width = 1280, .height = 720});
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
