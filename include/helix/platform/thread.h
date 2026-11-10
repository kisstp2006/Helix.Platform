#pragma once

#include <helix/core/base/types.h>

#include <atomic>

#if defined(_M_X64) || defined(__x86_64__)
#include <emmintrin.h>
#endif

namespace helix::platform
{
    using ThreadFunction = void (*)(void* user);

    struct ThreadDesc
    {
        ThreadFunction function = nullptr;
        void* user = nullptr;
        const char* name = "helix";
        usize stackSize = 0;
    };

    class Thread
    {
    public:
        Thread() = default;
        ~Thread();
        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;

        bool Start(const ThreadDesc& desc);
        void Join();
        bool Joinable() const { return handle_ != 0; }

    private:
        uptr handle_ = 0;
        ThreadDesc desc_{};
    };

    bool ThreadsSupported();
    u64 CurrentThreadId();
    void SetCurrentThreadName(const char* name);
    u32 LogicalCoreCount();
    void YieldThread();

    inline void SpinPause()
    {
#if defined(_M_X64) || defined(__x86_64__)
        _mm_pause();
#elif defined(__aarch64__) || defined(_M_ARM64)
        __asm__ __volatile__("yield");
#endif
    }

    class Mutex
    {
    public:
        Mutex();
        ~Mutex();
        Mutex(const Mutex&) = delete;
        Mutex& operator=(const Mutex&) = delete;
        void Lock();
        bool TryLock();
        void Unlock();
        void* Native() { return storage_; }

    private:
        alignas(16) u8 storage_[64];
    };

    class ScopedLock
    {
    public:
        explicit ScopedLock(Mutex& m) : mutex_(m) { mutex_.Lock(); }
        ~ScopedLock() { mutex_.Unlock(); }
        ScopedLock(const ScopedLock&) = delete;
        ScopedLock& operator=(const ScopedLock&) = delete;

    private:
        Mutex& mutex_;
    };

    class ConditionVariable
    {
    public:
        ConditionVariable();
        ~ConditionVariable();
        ConditionVariable(const ConditionVariable&) = delete;
        ConditionVariable& operator=(const ConditionVariable&) = delete;
        void Wait(Mutex& mutex);
        bool WaitFor(Mutex& mutex, f64 seconds);
        void NotifyOne();
        void NotifyAll();

    private:
        alignas(16) u8 storage_[64];
    };

    class Semaphore
    {
    public:
        explicit Semaphore(u32 initial = 0);
        void Release(u32 count = 1);
        void Acquire();
        bool TryAcquire();

    private:
        std::atomic<i32> count_;
        std::atomic<u32> waiters_{0};
    };

    class SpinLock
    {
    public:
        void Lock()
        {
            while (flag_.exchange(true, std::memory_order_acquire))
                while (flag_.load(std::memory_order_relaxed))
                    SpinPause();
        }
        void Unlock() { flag_.store(false, std::memory_order_release); }

    private:
        std::atomic<bool> flag_{false};
    };
}
