#include <helix/platform/thread.h>
#include <helix/platform/time.h>
#include <helix/core/base/assert.h>

#include <windows.h>

namespace helix::platform
{
    static_assert(sizeof(SRWLOCK) <= 64 && sizeof(CONDITION_VARIABLE) <= 64);

    namespace
    {
        DWORD WINAPI ThreadEntry(LPVOID param)
        {
            auto* desc = static_cast<ThreadDesc*>(param);
            SetCurrentThreadName(desc->name);
            desc->function(desc->user);
            return 0;
        }

        LARGE_INTEGER Frequency()
        {
            static LARGE_INTEGER f = [] {
                LARGE_INTEGER v;
                QueryPerformanceFrequency(&v);
                return v;
            }();
            return f;
        }
    }

    Thread::~Thread() { HX_ASSERT_MSG(!Joinable(), "thread destroyed without Join"); }

    bool Thread::Start(const ThreadDesc& desc)
    {
        HX_ASSERT(!Joinable() && desc.function);
        desc_ = desc;
        HANDLE h = CreateThread(nullptr, desc.stackSize, &ThreadEntry, &desc_, 0, nullptr);
        handle_ = reinterpret_cast<uptr>(h);
        return h != nullptr;
    }

    void Thread::Join()
    {
        if (!handle_)
            return;
        HANDLE h = reinterpret_cast<HANDLE>(handle_);
        WaitForSingleObject(h, INFINITE);
        CloseHandle(h);
        handle_ = 0;
    }

    bool ThreadsSupported() { return true; }
    u64 CurrentThreadId() { return GetCurrentThreadId(); }

    void SetCurrentThreadName(const char* name)
    {
        wchar_t wide[64];
        int n = MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, 64);
        if (n > 0)
            SetThreadDescription(GetCurrentThread(), wide);
    }

    u32 LogicalCoreCount()
    {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        return info.dwNumberOfProcessors;
    }

    void YieldThread() { SwitchToThread(); }

    Mutex::Mutex() { InitializeSRWLock(reinterpret_cast<SRWLOCK*>(storage_)); }
    Mutex::~Mutex() = default;
    void Mutex::Lock() { AcquireSRWLockExclusive(reinterpret_cast<SRWLOCK*>(storage_)); }
    bool Mutex::TryLock() { return TryAcquireSRWLockExclusive(reinterpret_cast<SRWLOCK*>(storage_)) != 0; }
    void Mutex::Unlock() { ReleaseSRWLockExclusive(reinterpret_cast<SRWLOCK*>(storage_)); }

    ConditionVariable::ConditionVariable() { InitializeConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(storage_)); }
    ConditionVariable::~ConditionVariable() = default;

    void ConditionVariable::Wait(Mutex& mutex)
    {
        SleepConditionVariableSRW(reinterpret_cast<CONDITION_VARIABLE*>(storage_), static_cast<SRWLOCK*>(mutex.Native()), INFINITE, 0);
    }

    bool ConditionVariable::WaitFor(Mutex& mutex, f64 seconds)
    {
        DWORD ms = DWORD(seconds * 1000.0);
        return SleepConditionVariableSRW(reinterpret_cast<CONDITION_VARIABLE*>(storage_), static_cast<SRWLOCK*>(mutex.Native()), ms, 0) != 0;
    }

    void ConditionVariable::NotifyOne() { WakeConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(storage_)); }
    void ConditionVariable::NotifyAll() { WakeAllConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(storage_)); }

    u64 Ticks()
    {
        LARGE_INTEGER v;
        QueryPerformanceCounter(&v);
        return u64(v.QuadPart);
    }

    u64 TicksPerSecond() { return u64(Frequency().QuadPart); }

    void SleepSeconds(f64 seconds)
    {
        if (seconds <= 0.0)
            return;
        static thread_local HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        if (timer)
        {
            LARGE_INTEGER due;
            due.QuadPart = -LONGLONG(seconds * 1e7);
            if (SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
            {
                WaitForSingleObject(timer, INFINITE);
                return;
            }
        }
        Sleep(DWORD(seconds * 1000.0));
    }
}
