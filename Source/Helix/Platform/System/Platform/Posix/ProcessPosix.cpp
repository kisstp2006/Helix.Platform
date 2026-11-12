#include <Helix/Platform/System/Process.h>

#include <Helix/Platform/IO/FileSystem.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

namespace helix::platform
{
#if HX_PLATFORM_LINUX
    // A lock file in the temporary folder; a lock of another open of the file (another process, or this one) fails.
    bool SingleInstance::Acquire(StringView name)
    {
        Release();
        String file("helix-");
        for (usize i = 0; i < name.Size(); i++)
            file.Append(name[i] == '/' ? '_' : name[i]);
        file.Append(".lock");
        String path = JoinPath(TempDirectory(), file);
        int fd = open(path.CStr(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
        if (fd < 0)
            return true;
        if (flock(fd, LOCK_EX | LOCK_NB) != 0)
        {
            close(fd);
            return false;
        }
        handle_ = uptr(fd) + 1;
        return true;
    }

    void SingleInstance::Release()
    {
        if (!handle_)
            return;
        int fd = int(handle_ - 1);
        flock(fd, LOCK_UN);
        close(fd);
        handle_ = 0;
    }

    // TracerPid in /proc/self/status is not zero while a debugger is attached.
    bool IsDebuggerAttached()
    {
        Array<u8> status;
        if (!ReadFile("/proc/self/status", status))
            return false;
        status.Push(0);
        const char* tracer = std::strstr(reinterpret_cast<const char*>(status.Data()), "TracerPid:");
        return tracer && std::atoi(tracer + 10) != 0;
    }

    void WaitForNativeDebugger()
    {
        if (IsDebuggerAttached())
            return;
        std::fprintf(stderr, "Attach a native debugger to process %d.\n", int(getpid()));
        while (!IsDebuggerAttached())
            usleep(100000);
    }
#else
    // Android and the Web run one instance by nature; whether a debugger is attached is not known there.
    bool SingleInstance::Acquire(StringView) { return true; }
    void SingleInstance::Release() { handle_ = 0; }
    bool IsDebuggerAttached() { return false; }
    void WaitForNativeDebugger() {}
#endif
}
